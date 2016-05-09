#pragma once 

#include "bits.h"
#include "cards.h"

struct PlayerVisible {
    struct Character {
        CardRef card;
        Cards styles;
        Cards weapons;
        bool immune;

        Character(CardRef c) : card(c), immune(false) {}

        bool empty() const {
            return styles.empty() && weapons.empty();
        }

        size_t size() const {
            return 1 + styles.size() + weapons.size();
        }

        void debug() const {
            LOG("    Character: {}", to_string(Card::Get(card)));
            LOG("        immune = {}", immune);
            for (const auto c : styles) {
                LOG("        {}", to_string(Card::Get(c)));
            }
            for (const auto c : weapons) {
                LOG("        {}", to_string(Card::Get(c)));
            }
        }
    };

    size_t                 num_characters:4;
    bool                   invert_value:1;
    int                    played_value;
    int                    played_points;
    std::vector<Character> characters;
    Bitset<uint16_t>       recv_style;
    Bitset<uint16_t>       recv_weapon;
    Bitset<uint16_t>       exposed_char;
    Bitset<uint16_t>       exposed_style;
    Bitset<uint16_t>       exposed_weapon;

    void debug() const {
        LOG("Visible:");
        LOG("    num_characters = {}", num_characters);
        LOG("    invert_value   = {}", invert_value);
        LOG("    played_value   = {}", played_value);
        LOG("    played_points  = {}", played_value);
        LOG("    recv_style     = {:016b}", recv_style);
        LOG("    recv_weapon    = {:016b}", recv_weapon);
        LOG("    exposed_char   = {:016b}", exposed_char);
        LOG("    exposed_style  = {:016b}", exposed_style);
        LOG("    exposed_weapon = {:016b}", exposed_weapon);
        for (const auto &c : characters) {
            c.debug();
        }
    }

    PlayerVisible()
    : num_characters(0)
    , played_value(0)
    , played_points(0)
    {
    }

    void reset() {
        characters.clear();
        num_characters = 0;
        invert_value   = false;
        played_value   = 0;
        played_points  = 0;
        recv_style     = 0;
        recv_weapon    = 0;
        exposed_char   = 0;
        exposed_style  = 0;
        exposed_weapon = 0;
    }

    void placeCharacter(const Card &card) {
        characters.emplace_back(Character {card.id});

        auto i = num_characters;
        ++num_characters;

        if (card.special == Special::IMMUNE) {
            characters[i].immune = true;
        }

        // TODO: sometimes a mismatch here
        assert(num_characters == characters.size());

        assert(num_characters <= sizeof(exposed_char)*8);
        exposeChar(i);

        if (invert_value) {
            played_points += card.inverted_value;
            played_value += card.inverted_value;
        } else {
            played_points += card.face_value;
            played_value += card.face_value;
        }

        if (card.special != Special::NO_STYLES) {
            recv_style.set(i);
        }
        if (card.special != Special::NO_WEAPONS) {
            recv_weapon.set(i);
        }
    }

    void exposeStyle(size_t i) {
        if (!characters[i].immune) {
            exposed_char.clear(i);
            exposed_style.set(i);
        }
    }

    void exposeWeapon(size_t i) {
        if (!characters[i].immune) {
            exposed_char.clear(i);
            exposed_weapon.set(i);
        }
    }

    void placeStyle(const Card &card, size_t i) {
        characters[i].styles.push_back(card.id);
        exposeStyle(i);
    }

    void placeWeapon(const Card &card, size_t i) {
        characters[i].weapons.push_back(card.id);
        exposeWeapon(i);
    }

    void unexposeStyle(size_t i) {
        assert(exposed_style[i]);
        exposed_style.clear(i);
    }

    void unexposeWeapon(size_t i) {
        assert(exposed_weapon[i]);
        exposed_weapon.clear(i);
    }

    void exposeChar(size_t i) {
        if (!characters[i].immune) {
            assert(!exposed_char[i]);
            exposed_char.set(i);
        }
    }

    CardRef removeCharacter(size_t i) {
        assert(characters[i].empty());
        --num_characters;
        auto c = characters[i].card;
        auto &card = Card::Get(c);
        characters.erase(characters.begin()+i);

        if (invert_value) {
            played_value -= card.inverted_value;
            played_points -= card.inverted_value;
        } else {
            played_value -= card.face_value;
            played_points -= card.face_value;
        }

        recv_style.drop(i);
        recv_weapon.drop(i);
        exposed_char.drop(i);
        exposed_style.drop(i);
        exposed_weapon.drop(i);
        return c;
    }

    CardRef removeStyle(size_t i) {
        assert(!characters[i].styles.empty());
        auto card = DrawCard(characters[i].styles);

        if (characters[i].styles.empty()) {
            unexposeStyle(i);
        }
        if (characters[i].empty()) {
            exposeChar(i);
        }
        return card;
    }

    CardRef removeWeapon(size_t i) {
        assert(!characters[i].weapons.empty());
        auto card = DrawCard(characters[i].weapons);

        if (characters[i].weapons.empty()) {
            unexposeWeapon(i);
        }
        if (characters[i].empty()) {
            exposeChar(i);
        }
        return card;
    }

    size_t size() const {
        size_t count = 0;
        for (const auto &c : characters) {
            count += c.size();
        }
        return count;
    }

    bool empty() const {
        return characters.empty();
    }

    void print() const {
        if (!empty()) {
            for (const auto &c : characters) {
                LOG("   - {}", to_string(Card::Get(c.card)));
                for (const auto &set : { c.weapons, c.styles }) {
                    for (auto r : set) {
                        LOG("     - {}", to_string(Card::Get(r)));
                    }
                }
            }
        }
    }
};
