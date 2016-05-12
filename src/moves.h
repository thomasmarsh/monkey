#pragma once 

#include "state.h"
#include "util.h"

#include <vector>
#include <map>

#include <cassert>

struct Moves {
    std::vector<Move> moves;
    const State &state;
    const Player &player;
    Player::Aggregate exposed;
    bool played_double_style;

    explicit Moves(const State &s, bool find_moves=true)
    : state(s)
    , player(s.current())
    , exposed(player.id, s.players)
    , played_double_style(false)
    {
        TRACE();
        if (find_moves) {
            moves.reserve(16);
            findMoves();
        }
    }

    // Find all moves for card i in hand.
    explicit Moves(const State &s, uint8_t i)
    : Moves(s, false)
    {
        TRACE();
        moves.reserve(16);
        checkCard(i, player.hand.at(i));
    }

    void add(const Move&& move) {
        moves.emplace_back(move);
    }

    void add(const Move::Step&& first, const Move::Step&& second=Move::Step()) {
        add({first, second});
    }

    void findCardMoves(uint8_t i, const Card &card) {
        TRACE();

        auto initial_count = moves.size();

        switch (card.arg_type) {
        case ArgType::NONE:
            if (card.type != CHARACTER) {
                ERROR("expected CHARACTER, got {} {}",
                      to_string(card.type),
                      to_string(card));
            }
            break;
        case ArgType::RECV_STYLE:
            assert(card.type == STYLE);
            if (!state.challenge.no_styles) {
                // Consider taking out the XOR here to play both single and double moves on a
                // character. See the related NOTE in maskedDoubleMoves.
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
        case ArgType::VISIBLE:              add(card.action); break;
        case ArgType::EXPOSED_CHAR:         maskedMoves(exposed.exposed_char, i, card); break;
        case ArgType::EXPOSED_STYLE:        maskedMoves(exposed.exposed_style, i, card); break;
        case ArgType::EXPOSED_WEAPON:       maskedMoves(exposed.exposed_weapon, i, card); break;
        case ArgType::VISIBLE_CHAR_OR_HOLD: visibleOrHoldMoves(i, card); break;
        case ArgType::OPPONENT:             opponentMoves(i, card); break;
        case ArgType::OPPONENT_HAND:        opponentHandMoves(i, card); break;
        case ArgType::HAND:                 handMoves(i, card); break;
        case ArgType::DRAW_PILE:            drawPileMoves(i, card); break;
        }

        if (card.type == CHARACTER && (moves.size() - initial_count) == 0) {
            add({Action::NONE, card.id, i});
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

    void maskedMoves(uint64_t mask, uint8_t i, const Card &card) {
        TRACE();
        if (!mask) { return; }
        for (uint8_t c : EachBit(mask)) {
            add({ card.action, card.id, i, c});
        }
    }

    std::vector<uint8_t> findStylesFromIndex(uint8_t i, const Card &card) {
        auto nc  = player.hand.characters.size();
        auto &skills = player.hand.skills;

        std::vector<uint8_t> matches;
        matches.reserve(skills.size());

        // The current index in the skills portion of the hand is `i - nc`. We forward to that
        // position + 1, so we are at the next card in the hand.
        auto ns = skills.size();
        for (uint8_t j=(i-nc)+1; j < ns; ++j) {
            auto &c = Card::Get(skills[j]);
            if (c.type == STYLE) {
                // We need to add back `nc` to restore it from a skill- to a hand relative index.
                matches.push_back(j + nc);
            }
        }
        return matches;
    }

    void maskedDoubleMoves(uint64_t mask, uint8_t i, const Card &card) {
        TRACE();

        if (!mask) {
            // No visible cards allow double moves.
            return;
        }

        auto styles = findStylesFromIndex(i, card);

        // No matches found means we only have one more card of this type in the hand after the
        // start position.
        if (styles.empty()) {
            // NOTE: if taking of the XOR in the arg_type dispatch, then remove this search here.

            if (!played_double_style) {
                maskedMoves(mask, i, card);
            }
            return;
        }

        played_double_style = true;

        // Finally, iterate over all the cards in hand that accept double styles.
        for (uint8_t c : EachBit(mask)) {
            // For each other card in hand
            for (auto j : styles) {
                assert(j > i);
                add({{card.action, card.id, j, c},
                     {card.action, card.id, i, c}});
            }
        }
    }

    void visibleOrHoldMoves(uint8_t i, const Card &card) {
        TRACE();
        uint8_t x = 0;
        auto n = player.visible.num_characters;
        assert(n == player.visible.characters.size());
        for (; x < n; ++x) {
            add({card.action, card.id, i, x});
        }
        // The last move (x > num chars) indicates a hold.
        add({card.action, card.id, i, x});
    }

    void opponentMoves(uint8_t i, const Card &card) {
        TRACE();
        for (uint8_t p=0; p < state.players.size(); ++p) {
            if (p != player.id) {
                add({card.action, card.id, i, p});
            }
        }
    }

    void opponentHandMoves(uint8_t i, const Card &card) {
        TRACE();
        for (uint8_t p=0; p < state.players.size(); ++p) {
            if (p != player.id) {
                if (!state.players[p].hand.empty()) {
                    add({card.action, card.id, i, p});
                }
            }
        }
    }

    void handMoves(uint8_t i, const Card &card) {
        TRACE();
        if (card.action == Action::PLAY_CHARACTER) {
            ScopedLogLevel l(LogContext::Level::warn);
            charHandMoves(i, card);
        } else {
            allHandMoves(i, card);
        }
    }

    void addDoubleChar(State &clone, uint8_t i, const Card &c1, uint8_t j, CardRef c) {
        Moves m2(clone, j);
        for (const auto &m : m2.moves) {
            assert(m.second.isNull());
            add({{Action::NONE, c1.id, i, Move::null}, m.first});
        }
    }

    void charHandMoves(uint8_t i, const Card &card) {
        auto clone = state;
        clone.processStep(Move::Step(Action::NONE, card.id, i));
        for (uint8_t j=0; j < player.hand.characters.size(); ++j) {
            auto c = player.hand.characters[j];
            if (j < i) {
                addDoubleChar(clone, i, card, j, c);
            } else if (j > i) {
                addDoubleChar(clone, i, card, j-1, c);
            }
        }
    }

    void allHandMoves(uint8_t i, const Card &card) {
        TRACE();
        for (uint8_t c=0; c < player.hand.size(); ++c) {
            // Need to be careful about the current card.
            if (c < i) {
                add({card.action, card.id, i, c});
            } else if (c > i) {
                add({card.action, card.id, i, uint8_t(c-1)});
            }
        }
    }

    void drawPileMoves(uint8_t i, const Card &card) {
        TRACE();
        // 0 = characters, 1 = skills
        add({card.action, card.id, i, 0});
        add({card.action, card.id, i, 1});
    }

    void discardTwo(uint8_t i) {
        TRACE();
        if (player.hand.empty()) {
            ERROR("unexpected empty hand");
        }
        if (player.hand.size() == 1) {
            add({Action::DISCARD_ONE, CardRef(-1), Move::null, 0});
            return;
        }

        for (uint8_t i=0; i < player.hand.size()-1; ++i) {
            for (uint8_t j=i+1; j < player.hand.size(); ++j) {
                add({{Action::DISCARD_ONE, CardRef(-1), Move::null, j},
                     {Action::DISCARD_ONE, CardRef(-1), Move::null, i}});
            }
        }
    }

    void firstCharacterMove() {
        TRACE();
        uint8_t i = 0;
        for (const auto c : player.hand.characters) {
            findCardMoves(i, Card::Get(c));
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

    void checkCard(uint8_t i, CardRef c) {
        const auto &card = Card::Get(c);
        if (canPlayCard(card)) {
            findCardMoves(i, card);
        }
    }

    void iterateCards(uint8_t &i, const Cards &cards) {
        for (const auto c : cards) {
            checkCard(i, c);
            ++i;
        }
    }

    void iterateHand() {
        uint8_t i = 0;
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

    void print() {
        LOG("Moves:");
        for (auto &m : moves) {
            LOG("    {}", to_string(m));
        }
    }
};
