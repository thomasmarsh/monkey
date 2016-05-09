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
    bool                 game_over:1;

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
            LOG("PLAYER {}", i);
            players[i].debug();
        }
    }

    bool checkGameOver() {
        return game_over = deck->draw.events.empty();
    }

    void step() {
        challenge.step();
        if (challenge.round.finished()) {
            checkGameOver();
        }
    }

    void reset() {
        if (!game_over) {
            discardVisible();
            challenge.reset();
            validateCards();
            deal();
            drawEvent();
        }
    }

    const Player &current() const { return players[challenge.round.current]; }
          Player &current()       { return players[challenge.round.current]; }

    // State-wide card count
    size_t size() const {
        size_t count = 0;
        for (const auto &p : players)  {
            count += p.size();
        }
        count += events.size() + deck->size();
        return count;
    }

    void validateCards() const {
        assert(size() == NUM_CARDS);
    }

    void discardVisible() {
        for (auto &p : players) {
            for (const auto c : p.hand.characters) { deck->discardCard(Card::Get(c)); }
            for (const auto c : p.hand.skills) { deck->discardCard(Card::Get(c)); }
            p.hand.reset();
            for (const auto &c : p.visible.characters) {
                for (auto s : c.styles) { deck->discardCard(Card::Get(s)); }
                for (auto w : c.weapons) { deck->discardCard(Card::Get(w)); }
                deck->discardCard(Card::Get(c.card));
            }
            p.visible.reset();
        }
    }

    auto exposed() const {
        return Player::Aggregate::Exposed(challenge.round.current, players);
    }

    void dealPortion(size_t target, Cards &source, size_t dest_size, Player &p)
    {
        auto n = target-std::min(target, dest_size);

        for (int i=0; i < n; ++i) {
            auto c = DrawCard(source);
            logDraw(p.id, c);
            p.hand.insert(c);
        }
    }

    void deal() {
        for (auto &p : players) {
            p.affinity = Affinity::NONE;
            dealPortion(4, deck->draw.characters, p.hand.characters.size(), p);
            dealPortion(6, deck->draw.skills,     p.hand.skills.size(),     p);
        }
    }

    void drawEvent() {
        auto c = deck->drawEvent();
        events.push_back(c);
        auto &card = Card::Get(c);
        LOG("<event:{}>", to_string(card));
        handleEvent(card);
    }

    void handleEvent(const Card &card) {
        switch (card.action) {
        case Action::E_NO_STYLES:       challenge.flags.no_styles  = true; break;
        case Action::E_NO_WEAPONS:      challenge.flags.no_weapons = true; break;
        case Action::E_CHAR_BONUS:      challenge.flags.char_bonus = true; break;
        case Action::E_INVERT_VALUE:    playersInvertValue();  break;
        case Action::E_DISCARD_TWO:     playersDiscardTwo();
        case Action::E_DRAW_ONE_CHAR:   playersDrawOneCharacter(); break;
        case Action::E_DRAW_TWO_SKILLS: playersDrawTwoSkills(); break;
        case Action::E_RANDOM_STEAL:    playersRandomSteal(); break;

        default:
            ERROR("unhandled card: {}", to_string(card));
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

    void logDraw(size_t i, CardRef c) {
        LOG("<player {}:draw {}>", i, to_string(Card::Get(c)));
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

    void discardOne(const Move &move) {
        auto &hand = current().hand;
        assert(move.arg < hand.size());
        auto c = hand.draw(move.arg);
        auto &card = Card::Get(c);
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
        default:
            WARN("unhandled action: {}", to_string(move.action));
            break;
        }
    }

    void pass() {
        LOG("<player {}:pass>", current().id);
        challenge.round.pass();
    }

    void concede() {
        LOG("<player {}:concede>", current().id);
        challenge.round.concede();
    }

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

    void perform(const Move &move) {
        if (move.card != Move::null) {
            auto c = current().hand.draw(move.card);
            auto &card = Card::Get(c);
            handleAction(move);
            playCard(move, card);
        } else {
            handleAction(move);
        }
        current().visible.print();
        step();
    }
};
