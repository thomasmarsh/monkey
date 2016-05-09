#pragma once

#include "cards.h"
#include "rand.h"

#include <memory>
#include <set>

struct DeckCardSet {
    Cards events;
    Cards characters;
    Cards skills;

    void clear() {
        events.clear();
        characters.clear();
        skills.clear();
    };

    size_t size() const {
        return events.size() + characters.size() + skills.size();
    }

    bool empty() const {
        return events.empty() && characters.empty() && skills.empty();
    }

    bool operator != (const DeckCardSet &rhs) const {
        return events     != rhs.events     ||
               characters != rhs.characters ||
               skills     != rhs.skills;
    }

    void shuffle() {
        Shuffle(events);
        Shuffle(characters);
        Shuffle(skills);
    }

    void sort() {
        Sort(events);
        Sort(characters);
        Sort(skills);
    }
};

struct Deck {
    using Ptr = std::shared_ptr<Deck>;

    DeckCardSet draw;
    DeckCardSet discard;

    Deck() {
        populate();
        shuffle();
    }

    Deck::Ptr clone() const {
        auto d = std::make_shared<Deck>();
        d->draw = draw;
        d->discard = discard;
        return d;
    }

    void replaceDiscards() {
        MoveCards(discard.events,     draw.events);
        MoveCards(discard.characters, draw.characters);
        MoveCards(discard.skills,     draw.skills);
    }

    void shuffle() {
        LOG("<shuffle deck>");
        replaceDiscards();
        draw.shuffle();
        assert(size() == NUM_CARDS);
    }

    void shuffleDiscard(Cards &to, Cards &from) {
        // We draw from the end of the vector, so in order to append cards, we
        // must first reverse the remaining items, append, then reverse again.
        std::reverse(std::begin(to), std::end(to));
        Shuffle(from);
        MoveCards(from, to);
        std::reverse(std::begin(to), std::end(to));
    }

    void shuffleCharacterDiscards() {
        LOG("<shuffle character discards>");
        shuffleDiscard(draw.characters, discard.characters);
    }

    void shuffleSkillDiscards() {
        LOG("<shuffle skill discards>");
        shuffleDiscard(draw.skills,     discard.skills);
    }

    void sort()  { draw.sort();  discard.sort(); }
    void clear() { draw.clear(); discard.clear(); }
    size_t size() const { return draw.size() + discard.size(); }

    bool operator != (const Deck &rhs) const {
        return draw    != rhs.draw ||
               discard != rhs.discard;
    }

    bool operator == (const Deck &rhs) const {
        return !(*this != rhs);
    }

    void insert(const Card &c, DeckCardSet &set) {
        switch (c.type) {
            case CHARACTER:
                set.characters.push_back(c.id);
                break;
            case EVENT:
                set.events.push_back(c.id);
                break;
            case WEAPON:
            case STYLE:
            case WRENCH:
                set.skills.push_back(c.id);
                break;
        }
    }

    void populate() {
        clear();

        for (int i=0; i < NUM_CARDS; ++i) {
            const auto &card = Card::Get(i);
            assert(i == card.id);
            insert(card, draw);
        }
    }

    void discardCard(const Card &card) {
        insert(card, discard);
    }

    void dealCharacters(size_t n, Cards &dest) {
        TRACE();
        for (int i=0; i < n; ++i) {
            dest.push_back(drawCharacter());
        }
    }

    void dealSkills(size_t n, Cards &dest) {
        TRACE();
        for (int i=0; i < n; ++i) {
            dest.push_back(drawSkill());
        }
    }

    CardRef drawFromPile(Cards &pile) {
        assert(!pile.empty());
        auto card = pile.back();
        pile.pop_back();
        return card;
    }

    CardRef drawEvent() {
        assert(!draw.events.empty());
        return DrawCard(draw.events);
    }

    CardRef drawCharacter() {
        if (draw.characters.empty()) {
            shuffleCharacterDiscards();
        }
        assert(!draw.characters.empty());
        return DrawCard(draw.characters);
    }

    CardRef drawSkill() {
        if (draw.skills.empty()) {
            shuffleSkillDiscards();
        }
        assert(!draw.skills.empty());
        return DrawCard(draw.skills);
    }

    void print() const {
        auto print = [](const char* title, auto pile) {
            LOG("{} ({}):", title, pile.size());
            auto r = pile;
            reverse(begin(r), end(r));
            for (const auto card : r) {
                LOG(" - {}", to_string(Card::Get(card)));
            }
        };

        LOG("==== DECK ====");
        LOG("");
        print("events", draw.events);
        print("characters", draw.characters);
        print("skills", draw.skills);
        print_size();
    }

    bool isConsistent() const {
        std::set<CardRef> cset;
        auto d = clone();
        d->shuffle();
        for (auto c : d->draw.events)     { cset.insert(c); }
        for (auto c : d->draw.characters) { cset.insert(c); }
        for (auto c : d->draw.skills)     { cset.insert(c); }
        return cset.size() == NUM_CARDS;
    }

    void print_size() const {
        LOG("total: draw={}/{}/{} discard={}/{}/{} = {}",
            draw.events.size(),
            draw.characters.size(),
            draw.skills.size(),
            discard.events.size(),
            discard.characters.size(),
            discard.skills.size(),
            size());
    }
};
