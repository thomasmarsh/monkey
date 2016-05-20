#pragma once

#include "bits.h"
#include "cards.h"

struct PlayerVisible {
    struct Character {
        CardRef card;
        Cards   styles;
        Cards   weapons;
        bool    immune:1;
        bool    two_weapons:1;
        bool    disarmed:1;

        Character(CardRef c)
        : card(c)
        , immune(false)
        , two_weapons(false)
        {}

        bool empty() const {
            return styles.empty() && weapons.empty();
        }

        size_t size() const {
            return 1 + styles.size() + weapons.size();
        }

#ifndef NO_DEBUG
        void debug() const {
            DLOG("    Character: {}", to_string(Card::Get(card)));
            DLOG("        immune      = {}", immune);
            DLOG("        two_weapons = {}", two_weapons);
            for (const auto c : styles) {
                DLOG("        {}", to_string(Card::Get(c)));
            }
            for (const auto c : weapons) {
                DLOG("        {}", to_string(Card::Get(c)));
            }
        }
#endif
    } __attribute__ ((__packed__));

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
    Bitset<uint16_t>       double_style;

    PlayerVisible()
    : num_characters(0)
    , invert_value(false)
    , played_value(0)
    , played_points(0)
    {
    }

#ifndef NO_DEBUG
    void debug() const {
        DLOG("Visible:");
        DLOG("    num_characters = {}", num_characters);
        DLOG("    invert_value   = {}", invert_value);
        DLOG("    played_value   = {}", played_value);
        DLOG("    played_points  = {}", played_value);
        DLOG("    recv_style     = {:016b}", recv_style);
        DLOG("    recv_weapon    = {:016b}", recv_weapon);
        DLOG("    exposed_char   = {:016b}", exposed_char);
        DLOG("    exposed_style  = {:016b}", exposed_style);
        DLOG("    exposed_weapon = {:016b}", exposed_weapon);
        DLOG("    double_style   = {:016b}", double_style);
        for (const auto &c : characters) {
            c.debug();
        }
    }
#endif

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
        double_style   = 0;
    }

    void disarm(size_t i) {
        characters[i].disarmed = true;
        recv_weapon.clear(i);
        recv_style.clear(i);
    }

    void dropColumn(size_t i) {
        recv_style.drop(i);
        recv_weapon.drop(i);
        exposed_char.drop(i);
        exposed_style.drop(i);
        exposed_weapon.drop(i);
        double_style.drop(i);
    }

    void setSpecial(size_t i, Special s) {
        bool want_styles  = true;
        bool want_weapons = true;

        switch (s) {
        case Special::NO_STYLES:     want_styles               = false; break;
        case Special::NO_WEAPONS:    want_weapons              = false; break;
        case Special::IMMUNE:        characters[i].immune      = true;  break;
        case Special::TWO_WEAPONS:   characters[i].two_weapons = true;  break;
        case Special::DOUBLE_STYLES: double_style.set(i); break;
        default:
            break;
        }

        if (want_styles)  { recv_style.set(i); }
        if (want_weapons) { recv_weapon.set(i); }
    }

    void addValue(const Card &card) {
        if (invert_value) {
            if (card.type == CHARACTER) {
                played_points += card.inverted_value;
            }
            played_value  += card.inverted_value;
        } else {
            if (card.type == CHARACTER) {
                played_points += card.face_value;
            }
            played_value  += card.face_value;
        }
    }

    void reduceValue(const Card &card) {
        if (invert_value) {
            played_value  -= card.inverted_value;
            if (card.type == CHARACTER) {
                played_points -= card.inverted_value;
            }
        } else {
            played_value  -= card.face_value;
            if (card.type == CHARACTER) {
                played_points -= card.face_value;
            }
        }
    }

    void placeCharacter(const Card &card) {
        characters.emplace_back(Character {card.id});

        auto i = num_characters;
        ++num_characters;

        assert(num_characters == characters.size());
        assert(num_characters <= sizeof(exposed_char)*8);
        assert(num_characters <= 10);

        setSpecial(i, card.special);
        exposeChar(i);
        addValue(card);
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
        assert(i < characters.size());
        characters[i].styles.push_back(card.id);
        exposeStyle(i);
        addValue(card);
    }

    void placeWeapon(const Card &card, size_t i) {
        assert(i < characters.size());
        characters[i].weapons.push_back(card.id);
        exposeWeapon(i);
        addValue(card);
        updateRecvWeapons(i);
    }

    void updateRecvWeapons(size_t i) {
        auto &c = characters[i];
        size_t allowed = c.two_weapons ? 2 : 1;
        if (c.weapons.size() >= allowed) {
            recv_weapon.clear(i);
        } else {
            recv_weapon.set(i);
        }
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
        assert(i < characters.size());
        assert(characters[i].empty());
        --num_characters;
        auto c = characters[i].card;
        auto &card = Card::Get(c);
        characters.erase(characters.begin()+i);

        reduceValue(card);
        dropColumn(i);

        return c;
    }

    CardRef removeStyle(size_t i) {
        assert(i < characters.size());
        assert(!characters[i].styles.empty());
        auto card = DrawCard(characters[i].styles);

        if (characters[i].styles.empty()) {
            unexposeStyle(i);
        }
        if (characters[i].empty()) {
            exposeChar(i);
        }
        reduceValue(Card::Get(card));
        return card;
    }

    CardRef removeWeapon(size_t i) {
        assert(i < characters.size());
        assert(!characters[i].weapons.empty());
        auto card = DrawCard(characters[i].weapons);

        if (characters[i].weapons.empty()) {
            unexposeWeapon(i);
        }
        if (characters[i].empty()) {
            exposeChar(i);
        }
        updateRecvWeapons(i);
        reduceValue(Card::Get(card));
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
#ifndef NO_LOGGING
        if (!empty()) {
            for (const auto &c : characters) {
                LOG("   - {}", to_string(Card::Get(c.card)));
                for (const auto &set : { c.weapons, c.styles }) {
                    for (auto r : set) {
                        LOG("     - {}", to_string(Card::Get(r)));
                    }
                }
            }
        } else {
            LOG("    <empty>");
        }
#endif// NO_LOGGING
    }
} __attribute__ ((__packed__));
