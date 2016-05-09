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
                           : to_string(Card::Get(player.hand.cards[m.card])),
                       m.arg == Move::null
                           ? std::string("null")
                           : std::to_string(m.arg),
                       m.next
                           ? to_string(*m.next, player)
                           : std::string("null"));
}


struct Moves {
    std::vector<Move> moves;
    const State &state;
    uint64_t exposed_char;
    uint64_t exposed_style;
    uint64_t exposed_weapon;

    Moves(const State &s) : state(s) {
        findMoves();
    }

    std::shared_ptr<Move> findNextAction(size_t, const Card &card) {
        WARN("unhandled");
        return nullptr;
    }

    void findActionMoves(size_t i, const Card &card) {
        switch (card.arg_type) {
        case ArgType::NONE:
            if (card.type != CHARACTER && card.type != WRENCH) {
                ERROR("expected CHARACTER, got {} {}",
                      to_string(card.type),
                      to_string(card));
            }
            add(Move(card.action, i));
            break;
        case ArgType::RECV_STYLE:
            if (!state.challenge.flags.no_styles) {
                maskedMoves(state.current().visible.recv_style, i, card);
            }
            break;
        case ArgType::RECV_WEAPON:
            if (!state.challenge.flags.no_weapons) {
                maskedMoves(state.current().visible.recv_weapon, i, card);
            }
            break;
        case ArgType::EXPOSED_CHAR:
            maskedMoves(exposed_char, i, card);
            break;
        case ArgType::EXPOSED_STYLE:
            maskedMoves(exposed_style, i, card);
            break;
        case ArgType::EXPOSED_WEAPON:
            maskedMoves(exposed_weapon, i, card);
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

    void add(const Move&& move) {
        moves.emplace_back(move);
    }

    void addConcede() { add(Move::Concede()); }
    void addPass()    { add(Move::Pass()); }

    bool firstMove() {
        return state.current().visible.characters.empty();
    }

    bool discardFlag() {
        return state.current().discard_two;
    }

    bool noCharactersInHand() {
        return state.current().hand.num_characters == 0;
    }

    bool canPlayCard(const Card &card) {
        if (firstMove() && card.type != CHARACTER) {
            return false;
        }

        if (state.current().affinity == Affinity::NONE ||
            card.affinity == Affinity::NONE) {
            return true;
        }

        return state.current().affinity == card.affinity;
    }

    void maskedMoves(uint64_t mask, size_t i, const Card &card) {
        for (size_t c : EachBit(mask)) {
            add(Move(card.action, i, c));
        }
    }

    void visibleOrHoldMoves(size_t i, const Card &card) {
        size_t x = 0;
        auto n = state.current().visible.num_characters;
        for (; x < n; ++x) {
            add(Move(card.action, i, x));
        }
        // The last move (x > num chars) indicates a hold.
        add(Move(card.action, i, x));
    }

    void opponentMoves(size_t i, const Card &card) {
        for (size_t p=0; p < state.players.size(); ++p) {
            if (p != state.current().id) {
                add(Move(card.action, i, p));
            }
        }
    }

    void opponentHandMoves(size_t i, const Card &card) {
        for (size_t p=0; p < state.players.size(); ++p) {
            if (p != state.current().id) {
                if (!state.players[p].hand.empty()) {
                    add(Move(card.action, i, p));
                }
            }
        }
    }

    void handMoves(size_t i, const Card &card) {
        const auto &p = state.current();
        for (size_t c=0; c < p.hand.size(); ++c) {
            // Need to be careful about the current card.
            if (c < i) {
                add(Move(card.action, i, c));
            } else if (c > i) {
                add(Move(card.action, i, c-1));
            }
        }
    }

    void drawPileMoves(size_t i, const Card &card) {
        // 0 = characters, 1 = skills
        add(Move(card.action, i, 0));
        add(Move(card.action, i, 1));
    }

    void discardTwo(size_t i) {
        const auto &p = state.current();
        if (p.hand.empty()) {
            ERROR("unexpected empty hand");
        }
        WARN("unhandled double discard - just discarding one");
        for (size_t c=0; c < p.hand.size(); ++c) {
            // Need to be careful about the current card.
            if (c < i) {
                add(Move(Action::DISCARD_ONE, Move::null, c));
            } else if (c > i) {
                add(Move(Action::DISCARD_ONE, Move::null, c-1));
            }
        }
#if 0
            auto indices = DoubleIndices(p.hand.size());
            for (const auto &e : indices) {
                add(
                        Move(Action::DISCARD_ONE, e[0], Move::null,
                             std::make_shared<Move>(Action::DISCARD_ONE, e[1])));
            }
#endif
    }

    void firstCharacterMove() {
        auto &cards = state.current().hand.cards;
        for (int i=0; i < cards.size(); ++i) {
            auto &card = Card::Get(cards[i]);
            if (card.type == CHARACTER) {
                findActionMoves(i, card);
            }
        }
    }

    void handleFirstMove() {
        if (noCharactersInHand()) {
            addConcede();
        } else if (discardFlag()) {
            discardTwo(Move::null);
        } else {
            firstCharacterMove();
        }
    }

    void findMoves() {
        std::tie(exposed_char,
                 exposed_style,
                 exposed_weapon) = state.exposed();

        moves.clear();

        if (firstMove()) {
            handleFirstMove();
            return;
        }

        auto &cards = state.current().hand.cards;
        for (int i=0; i < cards.size(); ++i) {
            auto &card = Card::Get(cards[i]);
            LOG("consider: {}", to_string(card));
            if (canPlayCard(card)) {
                findCardMoves(i, card);
            }
        }

        add(Move::Pass());
        add(Move::Concede());
    }
};

extern void test_moves();
