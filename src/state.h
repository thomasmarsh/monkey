#pragma once

#include "deck.h"
#include "challenge.h"
#include "player.h"
#include "move.h"
#include "util.h"

struct State {
    Deck::Ptr            deck;
    std::vector<CardRef> events;
    std::vector<Player>  players;
    Challenge            challenge;

    State(size_t num_players)
    : deck(std::make_shared<Deck>())
    , challenge(num_players)
    {
        TRACE();
        for (size_t i=0; i < num_players; ++i) {
            players.emplace_back(Player(i));
        }
    }

    State(const State &rhs)
    : deck(rhs.deck->clone())
    , events(rhs.events)
    , players(rhs.players)
    , challenge(rhs.challenge)
    {
        TRACE();
        assert(*deck == *rhs.deck);
        assert(events == rhs.events);
    }

    static std::shared_ptr<State> New(const State &rhs) {
        return std::make_shared<State>(rhs);
    }

    void init() {
        TRACE();
        deck->populate();
        deck->shuffle();
        deck->print();
        deal();
        drawEvent();
    }

#ifndef NO_DEBUG
    void debug() const {
        DLOG("EVENTS");
        for (const auto c : events) {
            DLOG("    {}", to_string(Card::Get(c)));
        }
        for (int i=0; i < players.size(); ++i) {
            DLOG("PLAYER {}", i);
            players[i].debug();
        }
        challenge.debug();
        DLOG("gameOver() = {}", gameOver());
    }
#endif

    // State-wide card count
    size_t size() const {
        TRACE();
        size_t count = 0;
        for (const auto &p : players) {
            count += p.size();
        }
        count += events.size() + deck->size();
        return count;
    }

    void validateCards() const {
        TRACE();
        assert(size() == NUM_CARDS);
    }

#pragma mark - State Management

    void checkGameOver() {
        TRACE();
        challenge.round.game_over = deck->draw.events.empty();
    }

    bool gameOver() const {
        TRACE();
        return challenge.round.game_over;
    }

    void step() {
        TRACE();
        assert(!challenge.round.game_over);
        challenge.step();
        if (challenge.finished()) {
            score();
            checkGameOver();
        }
    }

    bool roundFinished() { return challenge.round.finished(); }
    bool challengeFinished() { return challenge.finished(); }

    void resetRound() { challenge.round.reset(); }

    // Idempotently check whether we need to reset something and possibly do so.
    void checkReset() {
        if (!challenge.round.game_over) {
            if (challengeFinished()) {
                discardVisible();
                challenge.reset();
                deal();
                drawEvent();
            } else if (roundFinished()) {
                resetRound();
            }
        }
    }

    const Player &current() const { return players[challenge.round.current]; }
          Player &current()       { return players[challenge.round.current]; }

    void score() {
        size_t count = 0;
        size_t best = 0;
        int high = 0;
        for (int i = 0; i < players.size(); ++i) {
            // Ignoring conceded players
            if (!challenge.round.conceded[i]) {
                auto value = players[i].visible.played_value;
                if (value > high) {
                    count = 0;
                    best = i;
                    high = value;
                } else if (value == high) {
                    ++count;
                }
            }
        }
        if (count > 0) {
            LOG("tie - no points for anyone");
        } else {
            auto points = players[best].visible.played_points;
            LOG("player {} is the winner with {} points", best, points);
            players[best].score += points;
        }
    }

    float getResult(size_t i) const {
        TRACE();
        // Note: It may be useful to emphasize highest score even if losing:
        //     return (float) players[i]->score / 200.;
        size_t best = 0;
        int high = 0;
        size_t count = 0;
        for (size_t j=0; j < players.size(); ++j) {
            if (players[j].score >= high) {
                if (players[j].score == high) {
                    ++count;
                } else {
                    count = 0;
                }
                high = players[j].score;
                best = j;
            }
        }
        if (count == 0 && i == best) {
            return 1;
        }
        return 0;
    }

    void printScore() {
        auto s = std::to_string(players[0].score);
        for (size_t i=1; i < players.size(); ++i) {
            s += "/" + std::to_string(players[i].score);
        }
        LOG("Score: {}", s);
    }

    void discardVisible() {
        TRACE();
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
        TRACE();
        return Player::Aggregate(challenge.round.current, players);
    }

    void deal() {
        TRACE();
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
            deck->dealSkills(TARGET_S-std::min(TARGET_S, ns), p.hand.skills);
            for (auto i=ns; i < p.hand.skills.size(); ++i) {
                logDraw(p.id, p.hand.skills[i]);
            }
        }
    }

    // Randomize the hidden cards with respect to current player.
    void randomizeHiddenState() {
        TRACE();
        auto hidden = std::make_shared<Deck>();

        const auto i = current().id;

        // Copy all unseen cards (draw pile) from the main to the hidden deck.
        CopyCards(deck->draw.characters, hidden->draw.characters);
        CopyCards(deck->draw.skills,     hidden->draw.skills);

        // Copy all cards from players to hidden deck.
        for (auto &p : players) {
            if (p.id != i) {
                CopyCards(p.hand.characters, hidden->draw.characters);
                CopyCards(p.hand.skills,     hidden->draw.skills);
            }
        }

        // Shuffle cards. Now we can redistribute from the deck
        hidden->draw.shuffle();

        // Shuffle events directly since we didn't copy them.
        Shuffle(deck->draw.events);

        // Distribute cards to the players
        for (auto &p : players) {
            if (p.id != i) {
                auto &hand = p.hand;
                // Remember the size of the hand.
                auto nc = hand.characters.size();
                auto nr = hand.skills.size();

                // Now it is safe to clear the hand and redistribute.
                hand.reset();
                for (int j=0; j < nc; ++j) {
                    auto c = hidden->drawCharacter();
                    hand.characters.push_back(c);
                }
                for (int j=0; j < nr; ++j) {
                    auto c = hidden->drawSkill();
                    hand.insert(c);
                }
            }
        }

        // Move the remaining deck
        deck->draw.characters = std::move(hidden->draw.characters);
        deck->draw.skills  = std::move(hidden->draw.skills);
    }

#pragma mark - Event Handling

    void drawEvent() {
        TRACE();
        auto c = deck->drawEvent();
        events.push_back(c);
        const auto &card = Card::Get(c);
        LOG("<event:{}>", to_string(card));
        handleEvent(card);
    }

    void handleEvent(const Card &card) {
        TRACE();
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
    }

    void playersInvertValue() {
        TRACE();
        for (auto &p : players) {
            p.visible.invert_value = true;
        }
    }

    void playersDiscardTwo() {
        TRACE();
        for (auto &p : players) {
            p.discard_two = true;
        }
    }

    void playersDrawOneCharacter() {
        TRACE();
        for (auto &p : players) {
            auto c = deck->drawCharacter();
            logDraw(p.id, c);
            p.hand.insert(c);
        }
    }


    void playersDrawTwoSkills() {
        TRACE();
        for (auto &p : players) {
            for (int j=0; j < 2; ++j) {
                auto c = deck->drawSkill();
                logDraw(p.id, c);
                p.hand.insert(c);
            }
        }
    }

    void playersRandomSteal() {
        TRACE();
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
        TRACE();
        LOG("<player {}:pass>", current().id);
        challenge.round.pass();
    }

    void concede() {
        TRACE();
        LOG("<player {}:concede>", current().id);
        challenge.round.concede();
    }

    void discardOne(const Move::Step &step) {
        TRACE();
        auto &hand = current().hand;
        assert(step.arg < hand.size());
        auto c = hand.draw(step.arg);
        const auto &card = Card::Get(c);
        LOG("<player {}:discard {}>", current().id, to_string(card));
        deck->discardCard(card);

        if (step.index == Move::null) {
            current().discard_two = false;
        }
    }

    void drawCard(const Move::Step &step) {
        TRACE();
        CardRef c;
        switch (step.arg) {
        case 0: c = deck->drawCharacter(); break;
        case 1: c = deck->drawSkill(); break;
        default: ERROR("unhandled");
        }
        logDraw(current().id, c);
        current().hand.insert(c);
    }

    void stealCard(const Move::Step &step) {
        TRACE();
        auto &other = players[step.arg];
        auto c = other.hand.drawRandom();
        LOG("<player {}:steal {} {}>",
            current().id, other.id, to_string(Card::Get(c)));
        current().hand.insert(c);
    }

    Player &playerArg(uint8_t arg) { return players[arg >> 4]; }
    uint8_t indexArg(uint8_t arg) { return arg & 0xF; }

    void knockoutChar(size_t i) {
        TRACE();
        auto c = playerArg(i).visible.removeCharacter(indexArg(i));
        deck->discardCard(Card::Get(c));
    }

    void knockoutStyle(size_t i) {
        TRACE();
        auto c = playerArg(i).visible.removeStyle(indexArg(i));
        deck->discardCard(Card::Get(c));
    }

    void knockoutWeapon(size_t i) {
        TRACE();
        auto c = playerArg(i).visible.removeWeapon(indexArg(i));
        deck->discardCard(Card::Get(c));
    }

    void tradeHand(const Move::Step &step) {
        TRACE();
        std::swap(current().hand, players[step.arg].hand);
    }

    void handleAction(const Move::Step &step) {
        TRACE();
        switch (step.action) {
        case Action::NONE:
        case Action::PLAY_WEAPON:
        case Action::PLAY_STYLE:
            // Will be handled by playCard()
            assert(step.index != Move::null);
            break;
        case Action::PASS:               pass(); break;
        case Action::CONCEDE:            concede(); break;
        case Action::DISCARD_ONE:        discardOne(step); break;
        case Action::DRAW_CARD:          drawCard(step); break;
        case Action::STEAL_CARD:         stealCard(step); break;
        case Action::KNOCKOUT_CHARACTER: knockoutChar(step.arg); break;
        case Action::KNOCKOUT_STYLE:     knockoutStyle(step.arg); break;
        case Action::KNOCKOUT_WEAPON:    knockoutWeapon(step.arg); break;
        case Action::TRADE_HAND:         tradeHand(step); break;
        case Action::CLEAR_FIELD:        discardVisible(); break;

        case Action::DISARM_CHARACTER:
        case Action::PLAY_WEAPON_RETAIN:
        case Action::CAPTURE_WEAPON:
        case Action::PLAY_CHARACTER:
            //WARN("unimplemented action: {}", to_string(step.action));
            break;

        case Action::E_DRAW_TWO_SKILLS:
        case Action::E_NO_STYLES:
        case Action::E_DRAW_ONE_CHAR:
        case Action::E_NO_WEAPONS:
        case Action::E_RANDOM_STEAL:
        case Action::E_INVERT_VALUE:
        case Action::E_DISCARD_TWO:
        case Action::E_CHAR_BONUS:
            ERROR("received event: {}", to_string(step.action));
            break;
        }
    }

#pragma mark - Play Move

    void playCard(const Move::Step &step, const Card &card) {
        TRACE();
        auto &p = current();
        switch (card.type) {
        case CHARACTER:
            p.placeCharacter(card);
            break;
        case STYLE:
            p.placeStyle(card, step.arg);
            break;
        case WEAPON:
            p.placeWeapon(card, step.arg);
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

    void processStep(const Move::Step &step) {
        TRACE();
        assert(!step.isNull());
        if (step.index != Move::null) {
            auto &p = current();

            LOG("<player {}:play {}>", p.id, to_string(step));
            assert(step.index < p.hand.size());
            assert(step.card < NUM_CARDS);

            auto c = p.hand.draw(step.index);
#if 0
            if (step.card != c) {
                auto &a = Card::Get(step.card);
                auto &b = Card::Get(c);
                LOG("STEP:");
                a.debug();
                LOG("HAND");
                b.debug();
            }
#endif
            assert(step.card == c);

            playCard(step, Card::Get(step.card));
            handleAction(step);
        } else {
            assert(step.card == CardRef(-1));
            handleAction(step);
        }
    }

    void perform(const Move &move) {
        TRACE();
        // TODO: assert !round_finished, !gameOver, etc.
        assert(!move.isNull());
        processStep(move.first);
        if (!move.second.isNull()) {
            processStep(move.second);
        }

        step();
    }
};
