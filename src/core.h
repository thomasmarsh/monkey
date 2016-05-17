#pragma once

#include <string>

extern void Initialize();

enum class Affinity : uint8_t {
    NONE,
    MONK,
    CLAN,
};

enum CardType : uint8_t {
    CHARACTER,
    STYLE,
    WEAPON,
    WRENCH,
    EVENT,
};

enum class Special : uint8_t {
    NONE,
    IMMUNE,
    DOUBLE_STYLES,
    STYLE_PLUS_ONE,
    TWO_WEAPONS,
    DOUBLE_WEAPON_VALUE,
    IGNORE_AFFINITY,
    NO_STYLES,
    NO_WEAPONS
};

enum class Action : uint8_t {
    // action               // arg_type
    NONE,                   // -
    PASS,                   // -
    CONCEDE,                // -
    CLEAR_FIELD,            // -
    PLAY_CHARACTER,         // -
    STEAL_CARD,             // OPPONENT_HAND
    TRADE_HAND,             // OPPONENT
    DISCARD_ONE,            // HAND
    DRAW_CARD,              // DRAW_PILE
    PLAY_WEAPON,            // VISIBLE_CHAR
    PLAY_STYLE,             // VISIBLE_CHAR
    DISARM_CHARACTER,       // OPPONENT_EMPTY_CHAR
    KNOCKOUT_CHARACTER,     // OPPONENT_EMPTY_CHAR
    KNOCKOUT_STYLE,         // OPPONENT_STYLE_CHAR
    KNOCKOUT_WEAPON,        // OPPONENT_WEAPON_CHAR
    PLAY_WEAPON_RETAIN,     // VISIBLE_CHAR_OR_HOLD
    CAPTURE_WEAPON,         // OPPONENT_WEAPON_CHAR

    // events
    E_DRAW_TWO_SKILLS,
    E_NO_STYLES,
    E_DRAW_ONE_CHAR,
    E_NO_WEAPONS,
    E_RANDOM_STEAL,
    E_DISCARD_TWO,
    E_CHAR_BONUS,
    E_INVERT_VALUE,
};

enum class ArgType : uint8_t {
    NONE,                 // n/a
    VISIBLE,              // all visible cards (used by CLEAR_FIELD)
    RECV_STYLE,           // a visible character accepting styles
    RECV_WEAPON,          // a visible character accepting weapons
    EXPOSED_CHAR,         // an opponent's empty character
    EXPOSED_STYLE,        // an opponent's exposed style
    EXPOSED_WEAPON,       // an opponent's exposed weapon
    VISIBLE_CHAR_OR_HOLD, // a visible character, or hold in hand
    OPPONENT,             // a given opponent
    OPPONENT_HAND,        // opponent's hand (i.e., only if they have cards)
    HAND,                 // a card in hand
    DRAW_PILE,            // a draw pile (characters or skills)
};


inline std::string to_string_short(Affinity a) {
    switch (a) {
    case Affinity::NONE: return "*";
    case Affinity::MONK: return "M";
    case Affinity::CLAN: return "C";
    }
}

inline std::string to_string(Affinity a) {
    switch (a) {
    case Affinity::NONE: return "NONE";
    case Affinity::MONK: return "MONK";
    case Affinity::CLAN: return "CLAN";
    }
}

inline std::string to_string(Special s) {
    switch (s) {
    case Special::NONE:                return "NONE";
    case Special::IMMUNE:              return "IMMUNE";
    case Special::DOUBLE_STYLES:       return "DOUBLE_STYLES";
    case Special::STYLE_PLUS_ONE:      return "STYLE_PLUS_ONE";
    case Special::TWO_WEAPONS:         return "TWO_WEAPONS";
    case Special::DOUBLE_WEAPON_VALUE: return "DOUBLE_WEAPON_VALUE";
    case Special::IGNORE_AFFINITY:     return "IGNORE_AFFINITY";
    case Special::NO_STYLES:           return "NO_STYLES";
    case Special::NO_WEAPONS:          return "NO_WEAPONS";
    }
};

inline std::string to_string_short(CardType t) {
    switch (t) {
    case CHARACTER: return "C";
    case STYLE:     return "S";
    case WEAPON:    return "N";
    case WRENCH:    return "W";
    case EVENT:     return "E";
    }
}

inline std::string to_string(CardType t) {
    switch (t) {
    case CHARACTER: return "CHARACTER";
    case STYLE:     return "STYLE";
    case WEAPON:    return "WEAPON";
    case WRENCH:    return "WRENCH";
    case EVENT:     return "EVENT";
    }
}

inline std::string to_string(Action action) {
    switch (action) {
	case Action::NONE:               return "NONE";
	case Action::PASS:               return "PASS";
	case Action::CONCEDE:            return "CONCEDE";
	case Action::CLEAR_FIELD:        return "CLEAR_FIELD";
	case Action::STEAL_CARD:         return "STEAL_CARD";
	case Action::TRADE_HAND:         return "TRADE_HAND";
	case Action::DISCARD_ONE:        return "DISCARD_ONE";
	case Action::DRAW_CARD:          return "DRAW_CARD";
	case Action::PLAY_WEAPON:        return "PLAY_WEAPON";
	case Action::PLAY_STYLE:         return "PLAY_STYLE";
	case Action::DISARM_CHARACTER:   return "DISARM_CHARACTER";
	case Action::KNOCKOUT_CHARACTER: return "KNOCKOUT_CHARACTER";
	case Action::KNOCKOUT_STYLE:     return "KNOCKOUT_STYLE";
	case Action::KNOCKOUT_WEAPON:    return "KNOCKOUT_WEAPON";
	case Action::PLAY_WEAPON_RETAIN: return "PLAY_WEAPON_RETAIN";
	case Action::CAPTURE_WEAPON:     return "CAPTURE_WEAPON";
	case Action::PLAY_CHARACTER:     return "PLAY_CHARACTER";
    case Action::E_DRAW_TWO_SKILLS:  return "E_DRAW_TWO_SKILLS";
    case Action::E_NO_STYLES:        return "E_NO_STYLES";
    case Action::E_DRAW_ONE_CHAR:    return "E_DRAW_ONE_CHAR";
    case Action::E_NO_WEAPONS:       return "E_NO_WEAPONS";
    case Action::E_RANDOM_STEAL:     return "E_RANDOM_STEAL";
    case Action::E_DISCARD_TWO:      return "E_DISCARD_TWO";
    case Action::E_CHAR_BONUS:       return "E_CHAR_BONUS";
    case Action::E_INVERT_VALUE:     return "E_INVERT_VALUE";
    }
}

inline std::string to_string(ArgType a) {
    switch (a) {
    case ArgType::NONE:                 return "NONE";
    case ArgType::VISIBLE:              return "VISIBLE";
    case ArgType::RECV_STYLE:           return "RECV_STYLE";
    case ArgType::RECV_WEAPON:          return "RECV_WEAPON";
    case ArgType::EXPOSED_CHAR:         return "EXPOSED_CHAR";
    case ArgType::EXPOSED_STYLE:        return "EXPOSED_STYLE";
    case ArgType::EXPOSED_WEAPON:       return "EXPOSED_WEAPON";
    case ArgType::VISIBLE_CHAR_OR_HOLD: return "VISIBLE_CHAR_OR_HOLD";
    case ArgType::OPPONENT:             return "OPPONENT";
    case ArgType::OPPONENT_HAND:        return "OPPONENT_HAND";
    case ArgType::HAND:                 return "HAND";
    case ArgType::DRAW_PILE:            return "DRAW_PILE";
    }
};
