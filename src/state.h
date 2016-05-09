#pragma once

#include "deck.h"
#include "challenge.h"
#include "player.h"
#include "move.h"

struct State {
    Deck::Ptr            deck;
    std::vector<CardRef> events;
    std::vector<Player>  players;
    Challenge            challenge;
    uint8_t              challenge_num:4;

    State(size_t num_players)
    : deck(std::make_shared<Deck>())
    , challenge(num_players)
    {
        deck->print();
        for (size_t i=0; i < num_players; ++i) {
            players.emplace_back(Player(i));
        }
        deal();
        drawEvent();
    }

    void debug() const {
        for (int i=0; i < players.size(); ++i) {
            DLOG("PLAYER {}", i);
            players[i].debug();
        }
    }

    // State-wide card count
    size_t size() const {
        size_t count = 0;
        for (const auto &p : players) {
            count += p.size();
        }
        count += events.size() + deck->size();
        return count;
    }

    void validateCards() const {
        assert(size() == NUM_CARDS);
    }

#pragma mark - State Management

    void checkGameOver() {
        challenge.round.game_over = deck->draw.events.empty();
    }

    bool gameOver() const {
        return challenge.round.game_over;
    }

    void step() {
        assert(!challenge.round.game_over);
        challenge.step();
        if (challenge.finished()) {
            checkGameOver();
        }
    }

    void reset() {
        if (!challenge.round.game_over) {
            discardVisible();
            challenge.reset();
            validateCards();
            deal();
            drawEvent();
        }
    }

    const Player &current() const { return players[challenge.round.current]; }
          Player &current()       { return players[challenge.round.current]; }

    void discardVisible() {
        for (auto &p : players) {
            for (const auto &c : p.visible.characters) {
                for (auto s : c.styles) { deck->discardCard(Card::Get(s)); }
                for (auto w : c.weapons) { deck->discardCard(Card::Get(w)); }
                deck->discardCard(Card::Get(c.card));
            }
            p.visible.reset();
        }
    }

    auto exposed() const {
        return Player::Aggregate(challenge.round.current, players);
    }

    void deal() {
        constexpr size_t TARGET_C = 4;
        constexpr size_t TARGET_S = 6;

        for (auto &p : players) {
            p.affinity = Affinity::NONE;

            auto nc = p.hand.characters.size();
            deck->dealCharacters(TARGET_C-std::min(TARGET_C, nc), p.hand.characters);
            for (auto i=nc; i < p.hand.characters.size(); ++i) {
                logDraw(p.id, p.hand.characters[i]);
            }

            auto ns = p.hand.skills.size();
            deck->dealCharacters(TARGET_S-std::min(TARGET_S, ns), p.hand.skills);
            for (auto i=ns; i < p.hand.skills.size(); ++i) {
                logDraw(p.id, p.hand.skills[i]);
            }
        }
    }

    void drawEvent() {
        auto c = deck->drawEvent();
        events.push_back(c);
        const auto &card = Card::Get(c);
        LOG("<event:{}>", to_string(card));
        handleEvent(card);
    }

#pragma mark - Event Handling

    void handleEvent(const Card &card) {
        switch (card.action) {
        case Action::E_NO_STYLES:       challenge.no_styles  = true; break;
        case Action::E_NO_WEAPONS:      challenge.no_weapons = true; break;
        case Action::E_CHAR_BONUS:      challenge.char_bonus = true; break;
        case Action::E_INVERT_VALUE:    playersInvertValue();  break;
        case Action::E_DISCARD_TWO:     playersDiscardTwo();
        case Action::E_DRAW_ONE_CHAR:   playersDrawOneCharacter(); break;
        case Action::E_DRAW_TWO_SKILLS: playersDrawTwoSkills(); break;
        case Action::E_RANDOM_STEAL:    playersRandomSteal(); break;

        default:
            ERROR("unhandled card: {}", to_string(card));
        }
    }

    void logDraw(size_t i, CardRef c) {
        LOG("<player {}:draw {}>", i, to_string(Card::Get(c)));
        if (Card::Get(c).id == 0) {
            ;
        }
    }

    void playersInvertValue() {
        for (auto &p : players) {
            p.visible.invert_value = true;
        }
    }

    void playersDiscardTwo() {
        for (auto &p : players) {
            p.discard_two = true;
        }
    }

    void playersDrawOneCharacter() {
        for (auto &p : players) {
            auto c = deck->drawCharacter();
            logDraw(p.id, c);
            p.hand.insert(c);
        }
    }


    void playersDrawTwoSkills() {
        for (auto &p : players) {
            for (int j=0; j < 2; ++j) {
                auto c = deck->drawSkill();
                logDraw(p.id, c);
                p.hand.insert(c);
            }
        }
    }

    void playersRandomSteal() {
        // The chosen card from each player's hand.
        auto np = players.size();
        std::vector<CardRef> stolen;
        stolen.reserve(np);

        // Some players may not have a card, so track whether found one.
        Bitset<uint8_t> hasCard;

        // Choose a card from each player.
        size_t i = 0;
        for (auto &p : players) {
            if (!p.hand.empty()) {
                stolen.push_back(p.hand.drawRandom());
                hasCard.set(i);
            } else {
                stolen.push_back(0);
            }
            ++i;
        }

        // Transfer each card (if available) to the player to the left.
        for (size_t i=0; i < np; ++i) {
            if (hasCard[i]) {
                size_t left = (i+1) % np;
                auto &recipient = players[left];
                LOG("<transfer_card:{}->{} {}>",
                    i,
                    left,
                    to_string(Card::Get(stolen[i])));

                recipient.hand.insert(stolen[i]);
            }
        }
    }

#pragma mark - Card Actions

    void pass() {
        LOG("<player {}:pass>", current().id);
        challenge.round.pass();
    }

    void concede() {
        LOG("<player {}:concede>", current().id);
        challenge.round.concede();
    }

    void discardOne(const Move &move) {
        auto &hand = current().hand;
        assert(move.arg < hand.size());
        auto c = hand.draw(move.arg);
        const auto &card = Card::Get(c);
        LOG("<player {}:discard {}>", current().id, to_string(card));
        deck->discardCard(card);

        if (move.card == Move::null) {
            current().discard_two = false;
        }
    }

    void drawCard(const Move &move) {
        CardRef c;
        switch (move.arg) {
        case 0: c = deck->drawCharacter(); break;
        case 1: c = deck->drawSkill(); break;
        default: ERROR("unhandled");
        }
        logDraw(current().id, c);
        current().hand.insert(c);
    }

    void stealCard(const Move &move) {
        auto &other = players[move.arg];
        auto c = other.hand.drawRandom();
        LOG("<player {}:steal {} {}>",
            current().id, other.id, to_string(Card::Get(c)));
        current().hand.insert(c);
    }

    void knockoutChar(size_t i) {
        auto c = players[i >> 4].visible.removeCharacter(i & 0xF);
        deck->discardCard(Card::Get(c));
    }

    void knockoutStyle(size_t i) {
        auto c = players[i >> 4].visible.removeStyle(i & 0xF);
        deck->discardCard(Card::Get(c));
    }

    void knockoutWeapon(size_t i) {
        auto c = players[i >> 4].visible.removeWeapon(i & 0xF);
        deck->discardCard(Card::Get(c));
    }

    void tradeHand(const Move &move) {
        std::swap(current().hand, players[move.arg].hand);
    }

    void handleAction(const Move &move) {
        switch (move.action) {
        case Action::NONE:
        case Action::PLAY_WEAPON:
        case Action::PLAY_STYLE:
            // Will be handled by playCard()
            assert(move.card != Move::null);
            break;
        case Action::PASS:               pass(); break;
        case Action::CONCEDE:            concede(); break;
        case Action::DISCARD_ONE:        discardOne(move); break;
        case Action::DRAW_CARD:          drawCard(move); break;
        case Action::STEAL_CARD:         stealCard(move); break;
        case Action::KNOCKOUT_CHARACTER: knockoutChar(move.arg); break;
        case Action::KNOCKOUT_STYLE:     knockoutStyle(move.arg); break;
        case Action::KNOCKOUT_WEAPON:    knockoutWeapon(move.arg); break;
        case Action::TRADE_HAND:         tradeHand(move); break;
        case Action::CLEAR_FIELD:        discardVisible(); break;

        case Action::DISARM_CHARACTER:
        case Action::PLAY_WEAPON_RETAIN:
        case Action::PLAY_DOUBLESTYLE:
        case Action::CAPTURE_WEAPON:
        case Action::PLAY_CHARACTER:
            WARN("unimplemented action: {}", to_string(move.action));
            break;

        case Action::E_DRAW_TWO_SKILLS:
        case Action::E_NO_STYLES:
        case Action::E_DRAW_ONE_CHAR:
        case Action::E_NO_WEAPONS:
        case Action::E_RANDOM_STEAL:
        case Action::E_INVERT_VALUE:
        case Action::E_DISCARD_TWO:
        case Action::E_CHAR_BONUS:
            ERROR("received event: {}", to_string(move.action));
            break;
        }
    }

#pragma mark - Play Move

    void playCard(const Move &move, const Card &card) {
        auto &p = current();
        LOG("<player {}:play {}>", p.id, to_string(card));
        switch (card.type) {
        case CHARACTER:
            p.placeCharacter(card);
            break;
        case STYLE:
            p.placeStyle(card, move.arg);
            break;
        case WEAPON:
            p.placeWeapon(card, move.arg);
            break;
        case WRENCH:
            deck->discardCard(card);
            break;
        default:
            WARN("unhandled: {}", to_string(card));
            deck->discardCard(card);
            break;
        }
    }

    void processMove(const Move &move) {
        if (move.card != Move::null) {
            assert(move.card < NUM_CARDS);
            auto c = current().hand.draw(move.card);
            const auto &card = Card::Get(c);
            handleAction(move);
            playCard(move, card);
        } else {
            handleAction(move);
        }
    }

    void perform(const Move *move) {
        assert(move);
        while (move) {
            processMove(*move);
            move = move->next;
        }

        step();
    }
};
