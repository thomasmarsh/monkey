#pragma once 

#include "state.h"
#include "util.h"

#include <vector>
#include <map>

#include <cassert>

inline std::string to_string(const Move &m, const Player &player) {
    return fmt::format("{{ {} {{ {} }} {} {} }}",
                       to_string(m.action),
                       m.card == Move::null
                           ? std::string("null")
                           : to_string(Card::Get(player.hand.at(m.card))),
                       m.arg == Move::null
                           ? std::string("null")
                           : std::to_string(m.arg),
                       m.next
                           ? to_string(*m.next, player)
                           : std::string("null"));
}

struct Moves {
    std::vector<Move> moves;
    std::vector<Move::Ptr> allocated;
    const State &state;
    const Player &player;
    Player::Aggregate exposed;

    Moves(const State &s)
    : state(s)
    , player(s.current())
    , exposed(player.id, s.players)
    {
        TRACE();
        moves.reserve(16);
        findMoves();
    }

    ~Moves() {
        TRACE();
        for (auto p : allocated) {
            delete p;
        }
    }

    void add(const Move&& m) {
        moves.emplace_back(m);
    }

    void add(Action a,
             size_t i=Move::null,
             size_t arg=Move::null,
             Move::Ptr next=nullptr)
    {
        TRACE();
        assert(i <= 0xFF);
        assert(arg <= 0xFF);
        moves.emplace_back(Move {a, uint8_t(i), uint8_t(arg), next});
    }

    Move::Ptr alloc(Action a,
                    size_t i=Move::null,
                    size_t arg=Move::null,
                    Move::Ptr next=nullptr)
    {
        TRACE();
        assert(i <= 0xFF);
        assert(arg <= 0xFF);
        if (allocated.capacity() < 16) { allocated.reserve(16); }
        auto *p = new Move {a, uint8_t(i), uint8_t(arg), next};
        allocated.push_back(p);
        return p;
    }


    std::shared_ptr<Move> findNextAction(size_t, const Card &card) {
        TRACE();
        WARN("unhandled");
        return nullptr;
    }

    void findActionMoves(size_t i, const Card &card) {
        TRACE();
        size_t count = 0;
        switch (card.arg_type) {
        case ArgType::NONE:
            if (card.type != CHARACTER && card.type != WRENCH) {
                ERROR("expected CHARACTER, got {} {}",
                      to_string(card.type),
                      to_string(card));
            }
            add(card.action, i);
            break;
        case ArgType::RECV_STYLE:
            assert(card.type == STYLE);
            if (!state.challenge.no_styles) {
                maskedMoves(player.visible.recv_style ^ player.visible.double_style, i, card);
                maskedDoubleMoves(player.visible.double_style, i, card);
            }
            break;
        case ArgType::RECV_WEAPON:
            assert(card.type == WEAPON);
            if (!state.challenge.no_weapons) {
                maskedMoves(player.visible.recv_weapon, i, card);
            }
            break;
        case ArgType::EXPOSED_CHAR:
            count = maskedMoves(exposed.exposed_char, i, card);
            if (count == 0 && card.type == CHARACTER) { add(Action::NONE, i); }
            break;
        case ArgType::EXPOSED_STYLE:
            count = maskedMoves(exposed.exposed_style, i, card);
            if (count == 0 && card.type == CHARACTER) { add(Action::NONE, i); }
            break;
        case ArgType::EXPOSED_WEAPON:
            count = maskedMoves(exposed.exposed_weapon, i, card);
            if (count == 0 && card.type == CHARACTER) { add(Action::NONE, i); }
            break;
        case ArgType::VISIBLE_CHAR_OR_HOLD:
            visibleOrHoldMoves(i, card);
            break;
        case ArgType::OPPONENT:
            opponentMoves(i, card);
            break;
        case ArgType::OPPONENT_HAND:
            opponentHandMoves(i, card);
            break;
        case ArgType::HAND:
            handMoves(i, card);
            break;
        case ArgType::DRAW_PILE:
            drawPileMoves(i, card);
            break;
        }
    }

    void findCardMoves(size_t i, const Card &card) {
        TRACE();
        switch (card.type) {
        case CHARACTER: findActionMoves(i, card); break;
        case STYLE:     findActionMoves(i, card); break;
        case WEAPON:    findActionMoves(i, card); break;
        case WRENCH:    findActionMoves(i, card); break;
        default:
            ERROR("unhandled: {}", to_string(card));
            break;
        }
    }

    void addConcede() { TRACE(); add(Move::Concede()); }
    void addPass()    { TRACE(); add(Move::Pass()); }

    bool firstMove() {
        TRACE();
        return player.visible.characters.empty();
    }

    bool discardFlag() {
        TRACE();
        return player.discard_two;
    }

    bool noCharactersInHand() {
        TRACE();
        return player.hand.characters.empty();
    }

    bool canPlayCard(const Card &card) {
        TRACE();
        if (firstMove() && card.type != CHARACTER) {
            return false;
        }

        if (player.affinity == Affinity::NONE ||
            card.affinity == Affinity::NONE) {
            return true;
        }

        return player.affinity == card.affinity;
    }

    size_t maskedMoves(uint64_t mask, size_t i, const Card &card) {
        TRACE();
        if (!mask) {
            return 0;
        }
        size_t count = 0;
        for (size_t c : EachBit(mask)) {
            add(card.action, i, c);
            ++count;
        }
        return count;
    }

    size_t maskedDoubleMoves(uint64_t mask, size_t i, const Card &card) {
        TRACE();

        if (!mask) {
            return 0;
        }

        auto n = Bitset<uint64_t>(mask).count();
        if (n == 1) {
            add(card.action, i, Bitset<uint64_t>(mask).index(0));
        }

        std::vector<size_t> indices(n);
        for (size_t c : EachBit(mask)) {
            indices.push_back(c);
        }

        size_t count = 0;
        for (size_t j=0; j < (n-1); ++j) {
            for (size_t k=j+1; k < n; ++k) {
                add(card.action, i, k,
                    alloc(card.action, i, j));
                ++count;
            }
        }
        return count;
    }

    size_t visibleOrHoldMoves(size_t i, const Card &card) {
        TRACE();
        size_t count = 0;
        size_t x = 0;
        auto n = player.visible.num_characters;
        for (; x < n; ++x) {
            add(card.action, i, x);
            ++count;
        }
        // The last move (x > num chars) indicates a hold.
        add(card.action, i, x);
        return count+1;
    }

    void opponentMoves(size_t i, const Card &card) {
        TRACE();
        for (size_t p=0; p < state.players.size(); ++p) {
            if (p != player.id) {
                add(card.action, i, p);
            }
        }
    }

    void opponentHandMoves(size_t i, const Card &card) {
        TRACE();
        for (size_t p=0; p < state.players.size(); ++p) {
            if (p != player.id) {
                if (!state.players[p].hand.empty()) {
                    add(card.action, i, p);
                }
            }
        }
    }

    void handMoves(size_t i, const Card &card) {
        TRACE();
        for (size_t c=0; c < player.hand.size(); ++c) {
            // Need to be careful about the current card.
            if (c < i) {
                add(card.action, i, c);
            } else if (c > i) {
                add(card.action, i, c-1);
            }
        }
    }

    void drawPileMoves(size_t i, const Card &card) {
        TRACE();
        // 0 = characters, 1 = skills
        add(card.action, i, 0);
        add(card.action, i, 1);
    }

    void discardTwo(size_t i) {
        TRACE();
        if (player.hand.empty()) {
            ERROR("unexpected empty hand");
        }
        if (player.hand.size() == 1) {
            add(Action::DISCARD_ONE, Move::null, 0);
            return;
        }

        for (size_t i=0; i < player.hand.size()-1; ++i) {
            for (size_t j=i+1; j < player.hand.size(); ++j) {
                add(Action::DISCARD_ONE, Move::null, j,
                    alloc(Action::DISCARD_ONE, Move::null, i));
            }
        }
    }

    void firstCharacterMove() {
        TRACE();
        size_t i = 0;
        for (const auto c : player.hand.characters) {
            findActionMoves(i, Card::Get(c));
            ++i;
        }
    }

    void handleFirstMove() {
        TRACE();
        if (noCharactersInHand()) {
            addConcede();
        } else if (discardFlag()) {
            discardTwo(Move::null);
        } else {
            firstCharacterMove();
        }
    }

    void checkCard(size_t i, CardRef c) {
        const auto &card = Card::Get(c);
        if (canPlayCard(card)) {
            findCardMoves(i, card);
        }
    }

    void iterateCards(size_t &i, const Cards &cards) {
        for (const auto c : cards) {
            checkCard(i, c);
            ++i;
        }
    }

    void iterateHand() {
        size_t i = 0;
        const auto &hand = player.hand;
        iterateCards(i, hand.characters);
        iterateCards(i, hand.skills);
    }

    void findMoves() {
        TRACE();
        assert(moves.empty());

        if (firstMove()) {
            handleFirstMove();
            return;
        }

        iterateHand();

        add(Move::Pass());
        add(Move::Concede());
    }
};
