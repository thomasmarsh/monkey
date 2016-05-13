#include <SFML/Graphics.hpp>
#include "json11.hpp"

#include <cassert>
#include <cstdio>
#include <iostream>
#include <fstream>
#include <streambuf>

#include <chrono>
#include <thread>
#include <algorithm>
#include <random>
#include <map>
#include <string>
#include <vector>
#include <tuple>
#include <atomic>

using namespace std;
using namespace json11;

//#define LOGGING
//#define DISPLAY_UI

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunused-variable"

constexpr size_t MC_LEN   = 70;
constexpr size_t NSAMPLES = 10000;

constexpr size_t MCTS_LEN = 1000;
constexpr float EXPLORATION = 10;

#pragma clang diagnostic pop

// ---------------------------------------------------------------------------

static atomic<int> gDisableLog(0);

#ifdef LOGGING
#define LOG(...) { if (!gDisableLog) { printf("%04d: ", __LINE__); printf(__VA_ARGS__); fflush(stdout); } }
#else
#define LOG(...)
#endif

struct LogLock {
    LogLock()  { ++gDisableLog; }
    ~LogLock() { assert(gDisableLog > 0); --gDisableLog; assert(gDisableLog >= 0); }
};

#define ERROR(msg) \
    do { \
        fprintf(stderr, "\n%4d: ERROR: %s\n", __LINE__, msg); \
        fflush(stderr); \
        abort(); \
    } while (0)

static int gTraceLevel = 0;

struct Tracer {
    const char *f;
    int line;

    Tracer(const char *_f, int l) : f(_f), line(l) {
        printf("%*cENTER: %s:%d\n", gTraceLevel*4, ' ', f, line);
        ++gTraceLevel;
    }

    ~Tracer() {
        --gTraceLevel;
        printf("%*cLEAVE: %s:%d\n", gTraceLevel*4, ' ', f, line);
    }
};

//#define TRACE() Tracer __trace(__PRETTY_FUNCTION__, __LINE__)
#define TRACE()

// ---------------------------------------------------------------------------

enum Affinity {
    NONE,
    MONK,
    CLAN,
};

static string AffinityStr(Affinity a) {
    switch (a) {
    case NONE: return "*";
    case MONK: return "M";
    case CLAN: return "C";
    }
}

static string AffinityLongStr(Affinity a) {
    switch (a) {
    case NONE: return "NONE";
    case MONK: return "MONK";
    case CLAN: return "CLAN";
    }
}

static Affinity ParseAffinity(const string &a) {
    if      (a == "NONE" || a == "*") { return NONE; }
    else if (a == "MONK" || a == "M") { return MONK; }
    else if (a == "CLAN" || a == "C") { return CLAN; }

    LOG("unexpected affinity string: %s\n", a.c_str());
    abort();
}

enum CardType {
    CHARACTER,
    STYLE,
    WEAPON,
    WRENCH,
    EVENT,
};

static string CardTypeStr(CardType t) {
    switch (t) {
    case CHARACTER: return "C";
    case STYLE:     return "S";
    case WEAPON:    return "N";
    case WRENCH:    return "W";
    case EVENT:     return "E";
    }
}

static string CardTypeLongStr(CardType t) {
    switch (t) {
    case CHARACTER: return "CHARACTER";
    case STYLE:     return "STYLE";
    case WEAPON:    return "WEAPON";
    case WRENCH:    return "WRENCH";
    case EVENT:     return "EVENT";
    }
}

static CardType ParseCardType(const string &t) {
    if      (t == "CHARACTER" || t == "C") { return CHARACTER; }
    else if (t == "STYLE"     || t == "S") { return STYLE; }
    else if (t == "WEAPON"    || t == "N") { return WEAPON; }
    else if (t == "WRENCH"    || t == "W") { return WRENCH; }
    else if (t == "EVENT"     || t == "E") { return EVENT; }

    LOG("unexpected CardType string: %s\n", t.c_str());
    abort();
}

enum CardAction {
    NULL_ACTION,
    E_CHAR_VALUE_FLIP,
    E_NO_STYLES,
    E_DRAW_ONE_CHAR,
    E_DRAW_TWO_SKILLS,
    E_NO_WEAPONS,
    E_DISCARD_TWO,
    E_RANDOM_STEAL,
    E_CHAR_POINT_INCREASE,
    C_IMMUNE,
    C_IGNORE_AFFINITY,
    C_INCREASE_WEAPON_LIMIT,
    C_INCREASE_STYLE_RATE,
    C_KNOCKOUT_CHARACTER,
    C_KNOCKOUT_STYLE,
    C_KNOCKOUT_WEAPON,
    C_CAPTURE_WEAPON,
    C_BUDDY_CHAR,
    C_DOUBLE_WEAPON_VALUE,
    C_STYLE_POINTS,
    C_NO_STYLES,
    C_NO_WEAPONS,
    C_DRAW_CARD,
    C_THIEF,
    C_DISCARD,
    C_TRADE,
    W_NULLIFY_STYLE,
    W_CLEAR_FIELD,
    W_DISARM_PLAYER,
};

static string CardActionStr(CardAction action) {
    switch (action) {
    case NULL_ACTION:             return "NULL_ACTION";
    case E_CHAR_VALUE_FLIP:       return "E_CHAR_VALUE_FLIP";
    case E_NO_STYLES:             return "E_NO_STYLES";
    case E_DRAW_ONE_CHAR:         return "E_DRAW_ONE_CHAR";
    case E_DRAW_TWO_SKILLS:       return "E_DRAW_TWO_SKILLS";
    case E_NO_WEAPONS:            return "E_NO_WEAPONS";
    case E_DISCARD_TWO:           return "E_DISCARD_TWO";
    case E_RANDOM_STEAL:          return "E_RANDOM_STEAL";
    case E_CHAR_POINT_INCREASE:   return "E_CHAR_POINT_INCREASE";
    case C_IMMUNE:                return "C_IMMUNE";
    case C_IGNORE_AFFINITY:       return "C_IGNORE_AFFINITY";
    case C_INCREASE_WEAPON_LIMIT: return "C_INCREASE_WEAPON_LIMIT";
    case C_INCREASE_STYLE_RATE:   return "C_INCREASE_STYLE_RATE";
    case C_KNOCKOUT_CHARACTER:    return "C_KNOCKOUT_CHARACTER";
    case C_KNOCKOUT_STYLE:        return "C_KNOCKOUT_STYLE";
    case C_KNOCKOUT_WEAPON:       return "C_KNOCKOUT_WEAPON";
    case C_CAPTURE_WEAPON:        return "C_CAPTURE_WEAPON";
    case C_BUDDY_CHAR:            return "C_BUDDY_CHAR";
    case C_DOUBLE_WEAPON_VALUE:   return "C_DOUBLE_WEAPON_VALUE";
    case C_STYLE_POINTS:          return "C_STYLE_POINTS";
    case C_NO_STYLES:             return "C_NO_STYLES";
    case C_NO_WEAPONS:            return "C_NO_WEAPONS";
    case C_DRAW_CARD:             return "C_DRAW_CARD";
    case C_THIEF:                 return "C_THIEF";
    case C_DISCARD:               return "C_DISCARD";
    case C_TRADE:                 return "C_TRADE";
    case W_NULLIFY_STYLE:         return "W_NULLIFY_STYLE";
    case W_CLEAR_FIELD:           return "W_CLEAR_FIELD";
    case W_DISARM_PLAYER:         return "W_DISARM_PLAYER";
    }
}

CardAction ParseCardAction(const string &a) {
    if      (a == "NULL_ACTION")             { return NULL_ACTION; }
    else if (a == "E_CHAR_VALUE_FLIP")       { return E_CHAR_VALUE_FLIP; }
    else if (a == "E_NO_STYLES")             { return E_NO_STYLES; }
    else if (a == "E_DRAW_ONE_CHAR")         { return E_DRAW_ONE_CHAR; }
    else if (a == "E_DRAW_TWO_SKILLS")       { return E_DRAW_TWO_SKILLS; }
    else if (a == "E_NO_WEAPONS")            { return E_NO_WEAPONS; }
    else if (a == "E_DISCARD_TWO")           { return E_DISCARD_TWO; }
    else if (a == "E_RANDOM_STEAL")          { return E_RANDOM_STEAL; }
    else if (a == "E_CHAR_POINT_INCREASE")   { return E_CHAR_POINT_INCREASE; }
    else if (a == "C_IMMUNE")                { return C_IMMUNE; }
    else if (a == "C_IGNORE_AFFINITY")       { return C_IGNORE_AFFINITY; }
    else if (a == "C_INCREASE_WEAPON_LIMIT") { return C_INCREASE_WEAPON_LIMIT; }
    else if (a == "C_INCREASE_STYLE_RATE")   { return C_INCREASE_STYLE_RATE; }
    else if (a == "C_KNOCKOUT_CHARACTER")    { return C_KNOCKOUT_CHARACTER; }
    else if (a == "C_KNOCKOUT_STYLE")        { return C_KNOCKOUT_STYLE; }
    else if (a == "C_KNOCKOUT_WEAPON")       { return C_KNOCKOUT_WEAPON; }
    else if (a == "C_CAPTURE_WEAPON")        { return C_CAPTURE_WEAPON; }
    else if (a == "C_BUDDY_CHAR")            { return C_BUDDY_CHAR; }
    else if (a == "C_DOUBLE_WEAPON_VALUE")   { return C_DOUBLE_WEAPON_VALUE; }
    else if (a == "C_STYLE_POINTS")          { return C_STYLE_POINTS; }
    else if (a == "C_NO_STYLES")             { return C_NO_STYLES; }
    else if (a == "C_NO_WEAPONS")            { return C_NO_WEAPONS; }
    else if (a == "C_DRAW_CARD")             { return C_DRAW_CARD; }
    else if (a == "C_THIEF")                 { return C_THIEF; }
    else if (a == "C_DISCARD")               { return C_DISCARD; }
    else if (a == "C_TRADE")                 { return C_TRADE; }
    else if (a == "W_NULLIFY_STYLE")         { return W_NULLIFY_STYLE; }
    else if (a == "W_CLEAR_FIELD")           { return W_CLEAR_FIELD; }
    else if (a == "W_DISARM_PLAYER")         { return W_DISARM_PLAYER; }

    LOG("unexpected CardAction string: %s\n", a.c_str());
    abort();
}

static map<CardAction,string> CARD_ACTION_DESC;

static string CardActionDescription(CardAction a) {
    auto it = CARD_ACTION_DESC.find(a);
    assert(it != CARD_ACTION_DESC.end());
    return it->second;
}

struct CardTableEntry {
    CardType type;
    int id;
    int prototype;
    int quantity;
    int value;
    bool isUber;
    Affinity affinity;
    CardAction action;
    string label;

    string repr() const {
        constexpr size_t SIZE = 80;
        char buf[SIZE];
        snprintf(buf, SIZE, "[%02X %s%d%s%s \"%s\"]",
                 id,
                 CardTypeStr(type).c_str(),
                 value,
                 AffinityStr(affinity).c_str(),
                 isUber ? "U" : " ",
                 label.c_str());
        return buf;
    }

    Json to_json() const {
        return Json::object {
            { "type", CardTypeLongStr(type) },
            { "quantity", quantity },
            { "value", value },
            { "isUber", isUber },
            { "affinity", AffinityLongStr(affinity) },
            { "action", CardActionStr(action) },
            { "description", CardActionDescription(action) },
        };
    }
};

static string LoadFile(const string &fname) {
    ifstream t(fname);
    string str;

    if (!t.is_open()) {
        return str;
    }

    t.seekg(0, ios::end);   
    str.reserve(t.tellg());
    t.seekg(0, ios::beg);

    str.assign(istreambuf_iterator<char>(t),
               istreambuf_iterator<char>());
    return str;
}

static string LoadCardsJson() {
    auto jsonStr = LoadFile("cards.json");
    if (jsonStr.empty()) {
        jsonStr = LoadFile("../cards.json");
    }
    assert(!jsonStr.empty());
    return jsonStr;
}

void ValidateCardJson(const Json &json) {
    assert(json.is_object());
    string error;
    bool has_shape = json.has_shape({
        { "action",      Json::STRING },
        { "affinity",    Json::STRING },
        { "description", Json::STRING },
        { "isUber",      Json::BOOL },
        { "quantity",    Json::NUMBER },
        { "type",        Json::STRING },
        { "value",       Json::NUMBER },
        { "label",       Json::STRING },
    }, error);

    if (!error.empty()) {
        LOG("has_shape error: %s\n", error.c_str());
    }
    assert(has_shape);
}

using CardTable = vector<CardTableEntry>;

static CardTable CARD_TABLE_PROTO;

static void ParseCard(const Json &json) {
    ValidateCardJson(json);

    CardTableEntry proto;
    proto.action   = ParseCardAction(json["action"].string_value());
    proto.affinity = ParseAffinity(json["affinity"].string_value());
    proto.isUber   = json["isUber"].bool_value();
    proto.quantity = json["quantity"].int_value();
    proto.type     = ParseCardType(json["type"].string_value());
    proto.value    = json["value"].int_value();
    proto.label    = json["label"].string_value();
    CARD_TABLE_PROTO.push_back(proto);

    CARD_ACTION_DESC[proto.action] = json["description"].string_value();
}

static void ParseJson(const Json &json) {
    assert(json.is_array());

    auto cards = json.array_items();
    assert(!cards.empty());

    CARD_TABLE_PROTO.clear();
    CARD_TABLE_PROTO.reserve(cards.size());

    for (const auto &card : cards) {
        ParseCard(card);
    }
}

static void LoadCardTableProto() {
    string error;

    auto json = Json::parse(LoadCardsJson(), error);
    if (!error.empty()) {
        LOG("JSON parse error: %s\n", error.c_str());
        abort();
    }

    ParseJson(json);
}

constexpr size_t NUM_CARDS = 198;

vector<CardTableEntry> CARD_TABLE;

static void ExpandCardTable() {
    TRACE();
    assert(CARD_TABLE.empty());
    CARD_TABLE.reserve(NUM_CARDS);
    int id = 0;
    int prototype = 0;
    for (auto &e : CARD_TABLE_PROTO) {
        for (size_t i=0; i < e.quantity; ++i) {
            auto copy = e;
            copy.id = id;
            copy.prototype = prototype;
            CARD_TABLE.push_back(copy);
            ++id;
        }
        CARD_TABLE_PROTO[prototype].prototype = prototype;
        ++prototype;
    }
    if (CARD_TABLE.size() != NUM_CARDS) {
        LOG("CARD_TABLE.size() = %zu\n", CARD_TABLE.size());
    }
    assert(CARD_TABLE.size() == NUM_CARDS);
}

static void LoadCards() {
    LoadCardTableProto();
    ExpandCardTable();
}

// ---------------------------------------------------------------------------

using Cards = vector<size_t>;

// Copies all of cards from source to end of target.
static void CopyCards(const Cards &source, Cards &target) {
    TRACE();
    target.insert(end(target), begin(source), end(source));
}

// Moves all of cards from source to end of target.
static void MoveCards(Cards &source, Cards &target) {
    TRACE();
    CopyCards(source, target);
    source.clear();
}

static size_t DrawCard(Cards &set, size_t i) {
    assert(i < set.size());
    auto it = begin(set)+i;
    auto card = *it;
    set.erase(it);
    return card;
};


// ---------------------------------------------------------------------------

constexpr size_t ARC4RANDOM_MAX = 0x100000000L;

struct ARC4RNG {
    typedef size_t result_type;
    constexpr static size_t max() { return ARC4RANDOM_MAX; }
    constexpr static size_t min() { return 0; }
    size_t operator()() { return arc4random(); }
} Arc4RNG;

template <typename T>
static void Sort(T &v) {
    ::sort(begin(v), end(v));
}

template <typename T, typename F>
static void Sort(T &v, F f) {
    ::sort(begin(v), end(v), f);
}

template <typename T>
static void Shuffle(T &v) {
    std::shuffle(begin(v), end(v), Arc4RNG);
}

template <typename I>
static void Shuffle(I begin, I end) {
    std::shuffle(begin, end, Arc4RNG);
}

static size_t urand(size_t n) {
    assert(n < ARC4RANDOM_MAX);
    return arc4random_uniform((unsigned int) n);
}

static float RandFloat() {
    return (float)arc4random() / ARC4RANDOM_MAX;
}

template <typename I>
static I Choice(I begin, I end) {
    return begin + urand(distance(begin, end));
}

template <typename T, typename V>
static bool Contains(const T &c, const V &v) {
    return find(begin(c), end(c), v) != end(c);
}

static string EscapeQuotes(const string &before) {
    string after;
    after.reserve(before.length() + 4);

    for (string::size_type i = 0; i < before.length(); ++i) {
        switch (before[i]) {
            case '"':
            case '\\': after += '\\';
            default:   after += before[i];
        }
    }

    return after;
}

// ---------------------------------------------------------------------------

struct Move {
    enum Action {
        NULL_ACTION,
        PLAY_CHARACTER,
        PLAY_STYLE,
        PLAY_WEAPON,
        PLAY_WRENCH,
        KNOCKOUT_CHARACTER,
        KNOCKOUT_STYLE,
        KNOCKOUT_WEAPON,
        CAPTURE_WEAPON,
        BUDDY_CHAR,
        THIEF_STEAL,
        DISCARD_ONE,
        TRADE_HAND,
        DRAW_CARD,
        PASS,
        CONCEDE,
    };

    Action action;
    size_t index;

    static Move NullMove() { return Move { NULL_ACTION, 0 }; }
    static bool IsNullMove(const Move &m) { return m.action == NULL_ACTION; }

    bool operator == (const Move &rhs) const {
        return action == rhs.action && index == rhs.index;
    }

    string repr() const {
        string r = "{";
        switch (action) {
        case NULL_ACTION:        r += "NULL_ACTION"; break;
        case PLAY_CHARACTER:     r += "PLAY_CHARACTER"; break;
        case PLAY_STYLE:         r += "PLAY_STYLE"; break;
        case PLAY_WEAPON:        r += "PLAY_WEAPON"; break;
        case PLAY_WRENCH:        r += "PLAY_WRENCH"; break;
        case PASS:               r += "PASS"; break;
        case CONCEDE:            r += "CONCEDE"; break;
        case KNOCKOUT_CHARACTER: r += "KNOCKOUT_CHARACTER"; break;
        case KNOCKOUT_STYLE:     r += "KNOCKOUT_STYLE"; break;
        case KNOCKOUT_WEAPON:    r += "KNOCKOUT_WEAPON"; break;
        case CAPTURE_WEAPON:     r += "CAPTURE_WEAPON"; break;
        case BUDDY_CHAR:         r += "BUDDY_CHAR"; break;
        case THIEF_STEAL:        r += "THIEF_STEAL"; break;
        case DISCARD_ONE:        r += "DISCARD_ONE"; break;
        case TRADE_HAND:         r += "TRADE_HAND"; break;
        case DRAW_CARD:          r += "DRAW_CARD"; break;
        }
        r += ":" + to_string(index) + "}";
        return r;
    }

    bool operator < (const Move &rhs) const {
        if (action != rhs.action) {
            return (int) action < (int) rhs.action;
        }
        return index < rhs.index;
    }
};

using MoveList = vector<Move>;

// ---------------------------------------------------------------------------

using IndexList = vector<size_t>;
using PermutationList = vector<pair<size_t, IndexList>>;

static void Permute(PermutationList &result, IndexList combo)
{
    auto mx = *max_element(begin(combo), end(combo));
    do {
        result.push_back({mx, combo});
    } while (next_permutation(begin(combo), end(combo)));
}

static void Combo(PermutationList &result, IndexList &c, int n, int offset, int k) {
    if (k == 0) {
        Permute(result, c);
    } else {
        for (int i=offset; i <= n-k; ++i) {
            c.push_back(i);
            Combo(result, c, n, i+1, k-1);
            c.pop_back();
        }
    }
}

static void Sort(PermutationList &r) {
    sort(begin(r), end(r),
         [](const auto &a, const auto &b) {
             if (a.first != b.first) {
                 return a.first < b.first;
             }
             auto &x = a.second;
             auto &y = b.second;
             for (int i=0; i < x.size(); ++i) {
                if (x[i] != y[i]) {
                    return x[i] < y[i];
                }
             }
             return false;
         });
}

static PermutationList BuildPermutations(size_t n, size_t k) {
    auto result = PermutationList();
    auto combo = IndexList();

    Combo(result, combo, n, 0, k);
    Sort(result);

    return result;
}

// ---------------------------------------------------------------------------

constexpr size_t MAX_DOUBLE_MOVES = 15;

static auto DOUBLE_MOVES = BuildPermutations(MAX_DOUBLE_MOVES, 2);

static size_t NumDoubleMoves(size_t n) {
    // This is the same as the binomial coefficient * 2.

    assert(n < MAX_DOUBLE_MOVES);
    size_t i = 0;
    while (true) {
        auto &entry = DOUBLE_MOVES[i];
        if (entry.first >= n) {
            return i;
        }
        ++i;
    }
    ERROR("n too high");
}

static vector<IndexList> DoubleMoves(size_t n) {
    assert(n < MAX_DOUBLE_MOVES);
    auto moves = vector<IndexList>();
    size_t i=0;
    while (true) {
        auto &entry = DOUBLE_MOVES[i];
        if (entry.first >= n) {
            return moves;
        }
        moves.push_back(entry.second);
        ++i;
    }
    ERROR("n too high");
}

// ---------------------------------------------------------------------------

struct ChallengeFlags {
    bool no_styles:1;
    bool no_weapons:1;
    bool char_value_flip:1;
    bool char_point_increase:1;

    ChallengeFlags() { reset(); }

    void reset() {
        no_styles           = false;
        no_weapons          = false;
        char_value_flip     = false;
        char_point_increase = false;
    }

    string repr() {
        char buf[80];
        snprintf(buf, 80, "{"
                 "no_styles=%d "
                 "no_weapons=%d "
                 "char_flip=%d "
                 "char_point=%d}",
                 no_styles,
                 no_weapons,
                 char_value_flip,
                 char_point_increase);
        return buf;
    }
};

struct PlayerFlags {
    Affinity affinity;
    bool ignore_affinity;

    PlayerFlags() { reset(); }

    void reset() {
        TRACE();
        affinity = NONE;
        ignore_affinity = false;
    }
};

// ---------------------------------------------------------------------------

struct Stats {
    struct Count {
        int wins;
        int losses;

        Count() : wins(0), losses(0) {}
    };

    map<Affinity, Count> detail;

    void addWin(Affinity a) {
        detail[a].wins++;
    }

    void addLoss(Affinity a) {
        detail[a].losses++;
    }
};

// ---------------------------------------------------------------------------

static size_t AdjustedValue(size_t c, const ChallengeFlags &flags) {
    TRACE();
    auto &card = CARD_TABLE[c];
    size_t v = 0;
    if (flags.char_value_flip) {
        switch (card.affinity) {
        case NONE: v =   card.value; break;
        case CLAN: v = 5-card.value; break;
        case MONK: v = 7-card.value; break;
        }
    } else {
        v = card.value + flags.char_point_increase;
    }
    return v;
}

// ---------------------------------------------------------------------------

struct PlayerVisible {
    struct Character {
        size_t character;
        Cards styles;
        Cards weapons;

        size_t size()          const { return 1+styles.size()+weapons.size(); }
        bool empty()           const { return styles.empty() && weapons.empty(); }

        CardAction action()    const { return CARD_TABLE[character].action; }

        bool isImmune()        const { return action() == C_IMMUNE; }
        bool canReceiveStyle() const { return action() != C_NO_STYLES; }
        bool canUseWeapons()   const { return action() != C_NO_WEAPONS; }
        bool hasDoubleStyles() const { return action() == C_INCREASE_STYLE_RATE; }
        bool canDoubleWeapon() const { return action() == C_INCREASE_WEAPON_LIMIT; }

        bool canReceiveWeapon() const {
            return canUseWeapons() &&
                     (weapons.empty() || (canDoubleWeapon() && weapons.size() == 1));
        }
    };

    vector<Character> characters;
    Cards wrenches;

    void insertCharacter(size_t c) {
        TRACE();
        assert(CARD_TABLE[c].type == CHARACTER);
        characters.push_back(Character {c, {}, {}});
    }

    void insertCard(size_t card, size_t character) {
        TRACE();
        for (auto &e : characters) {
            if (e.character == character) {
                switch (CARD_TABLE[card].type) {
                case STYLE:  e.styles.push_back(card);  return;
                case WEAPON: e.weapons.push_back(card); return;
                case WRENCH:
                case CHARACTER:
                case EVENT:
                     ERROR("unexpected");
                     break;
                }
            }
        }
        ERROR("character not found");
    }

    size_t freeStylePositions() const {
        TRACE();
        return accumulate(begin(characters),
                          end(characters),
                          0,
                          [](size_t n, const auto &c) { return n+c.canReceiveStyle(); });
    }

    size_t freeWeaponPositions() const {
        TRACE();
        return accumulate(begin(characters),
                          end(characters),
                          0,
                          [](size_t n, const auto &c) { return n+c.canReceiveWeapon(); });
    }

    bool canReceiveWeapon() const {
        TRACE();
        for (const auto &c : characters) {
            if (c.canReceiveWeapon()) {
                return true;
            }
        }
        return false;
    }

    bool canReceiveStyle() const {
        TRACE();
        for (const auto &c : characters) {
            if (c.canReceiveStyle()) {
                return true;
            }
        }
        return false;
    }

    bool hasDoubleStyles() const {
        for (const auto &c : characters) {
            if (c.hasDoubleStyles()) {
                return true;
            }
        }
        return false;
    }

    size_t styleMoveCount(size_t n) const {
        size_t count = n * freeStylePositions();
        for (const auto &c : characters) {
            if (c.hasDoubleStyles()) {
                count += NumDoubleMoves(n);
            }
        }
        return count;
    }

    size_t size() const {
        TRACE();
        return wrenches.size()
            + accumulate(begin(characters),
                         end(characters),
                         0,
                         [](size_t n, const auto &c) { return n+c.size(); });
    }

    bool empty() const { return characters.empty() && wrenches.empty(); }

    int value(const ChallengeFlags &flags) const {
        TRACE();
        int v = 0;
        for (const auto &c : characters) {
            v += AdjustedValue(c.character, flags);
            for (auto r : c.styles) {
                v += CARD_TABLE[r].value;
            }
            v += (CARD_TABLE[c.character].action == C_STYLE_POINTS) * c.styles.size();

            int weapon_multiple = CARD_TABLE[c.character].action == C_DOUBLE_WEAPON_VALUE ? 2 : 1;
            for (auto r : c.weapons) {
                v += CARD_TABLE[r].value * weapon_multiple;
            }
        }
        return v;
    }

    int value() const {
        TRACE();
        static ChallengeFlags nullFlags;
        return value(nullFlags);
    }

    int characterValue(const ChallengeFlags &flags) const {
        TRACE();
        return accumulate(begin(characters),
                          end(characters),
                          0,
                          [&flags](int n, const auto &c) {
                              return n+AdjustedValue(c.character, flags);
                          });
    }
};

struct PlayerHand {
    Cards characters;
    Cards styles;
    Cards weapons;
    Cards wrenches;

    PlayerHand() {
        TRACE();
        characters.reserve(8);
        styles.reserve(6);
        weapons.reserve(6);
        wrenches.reserve(2);
    }

    void insertCard(size_t c) {
        TRACE();
        switch (CARD_TABLE[c].type) {
        case STYLE:     styles.push_back(c);     break;
        case WEAPON:    weapons.push_back(c);    break;
        case WRENCH:    wrenches.push_back(c);   break;
        case CHARACTER: characters.push_back(c); break;
        case EVENT:
             ERROR("received event in insertCard");
             break;
        }
    }

    pair<size_t,CardType> drawIndexed(size_t i) {
        TRACE();
        auto cmax = characters.size();
        auto count = cmax;
        count += styles.size();   auto smax = count;
        count += wrenches.size(); auto wmax = count;
        count += weapons.size();  auto tmax = count;

        assert(i < tmax);

        if      (i >= wmax) { i -= wmax; return { DrawCard(weapons,  i), WEAPON }; }
        else if (i >= smax) { i -= smax; return { DrawCard(wrenches, i), WRENCH }; }
        else if (i >= cmax) { i -= cmax; return { DrawCard(styles,   i), STYLE }; }
        assert(i < cmax);
        return {DrawCard(characters, i), CHARACTER};
    }

    pair<size_t,CardType> randomCard() const {
        TRACE();
        auto cmax = characters.size();
        auto count = cmax;
        count += styles.size();   auto smax = count;
        count += wrenches.size(); auto wmax = count;
        count += weapons.size();  auto tmax = count;

        size_t i = urand(tmax);
        if      (i >= wmax) { i -= wmax; return {i, WEAPON}; }
        else if (i >= smax) { i -= smax; return {i, WRENCH}; }
        else if (i >= cmax) { i -= cmax; return {i, STYLE}; }
        assert(i < cmax);
        return {i, CHARACTER};
    }

    Cards &cardsForType(CardType t) {
        switch (t) {
        case CHARACTER: return characters;
        case STYLE:     return styles;
        case WEAPON:    return weapons;
        case WRENCH:    return wrenches;
        case EVENT:
            break;
        }
        ERROR("bad card request");
    }

    pair<size_t,CardType> drawRandomCard() {
        TRACE();
        auto r = randomCard();
        return { DrawCard(cardsForType(r.second), r.first), r.second };
    }

    void clear() {
        TRACE();
        characters.clear();
        styles.clear();
        weapons.clear();
        wrenches.clear();
    }

    size_t size()  const { return characters.size() + skillsSize(); }
    bool   empty() const { return characters.empty() && skillsEmpty(); }

    size_t skillsSize() const {
        TRACE();
        return styles.size() + weapons.size() + wrenches.size();
    }

    bool skillsEmpty() const {
        return styles.empty() && weapons.empty() && wrenches.empty();
    }

    int skillValue() const {
        TRACE();
        int v = 0;
        for (const auto & set : { styles, weapons, wrenches }) {
            for (const auto c : set) {
                v += CARD_TABLE[c].value;
            }
        }
        return v;
    }

    int value() const {
        TRACE();
        static ChallengeFlags nullFlags;
        return skillValue() + characterValue(nullFlags);
    }

    int value(const ChallengeFlags &flags) const {
        TRACE();
        return skillValue() + characterValue(flags);
    }


    int characterValue(const ChallengeFlags &flags) const {
        TRACE();
        return accumulate(begin(characters),
                          end(characters),
                          0,
                          [&flags](size_t v, const auto &c) {
                              return v+AdjustedValue(c, flags);
                          });
    }
};

// ---------------------------------------------------------------------------

struct GameState;

struct MoveCount {
    const GameState *state;
    size_t count;
    size_t wmax;
    size_t smax;
    size_t cmax;
    size_t imax;

    bool can_concede;
    size_t vc_empty;
    size_t hc_empty;

    MoveList additional;

    explicit MoveCount(const GameState *state);
    void countMoves();
    void buildAdditional();
    Move move(size_t i);
    void print();
};

// ---------------------------------------------------------------------------

struct Player {
    using Ptr = shared_ptr<Player>;

    // These cards are hidden from view.
    PlayerHand hand;
    // These cards are on the table face-up.
    PlayerVisible visible;
    MoveList additional;
    PlayerFlags flags;
    Stats stats;
    int score;
    size_t id;

    Player() : score(0), id(-1) {}

    virtual string name() const = 0;
    virtual Move move(shared_ptr<GameState> gs) = 0; 

    template <typename T>
    Ptr clone() const {
        TRACE();
        auto p = make_shared<T>();
        p->hand       = hand;
        p->visible    = visible;
        p->additional = additional;
        p->flags      = flags;
        p->stats      = stats;
        p->score      = score;
        p->id         = id;
        return p;
    }

    size_t size() const { TRACE(); return hand.size() + visible.size(); }
    size_t empty() const { TRACE(); return hand.empty() && visible.empty(); }

    bool isAvailable(size_t i) const {
        TRACE();
        const auto &card = CARD_TABLE[hand.characters[i]];
        return (flags.affinity == NONE ||
                card.affinity == NONE ||
                card.affinity == flags.affinity);
    }

    void addAction(Move::Action a) { additional.emplace_back(Move {a, 0}); }

    size_t availableCharacters() const {
        TRACE();
        if (flags.affinity == NONE || flags.ignore_affinity) {
            return hand.characters.size();
        }

        size_t count = 0;
        for (int i=0; i < hand.characters.size(); ++i) {
            if (isAvailable(i)) {
                ++count;
            }
        }
        return count;
    }

    size_t cardForStyleIndex(size_t index) const {
        TRACE();
        assert(!visible.empty());
        auto &pile = hand.styles;

        auto ci = index / pile.size();
        assert(ci < visible.characters.size());
        auto ri = index % pile.size();
        assert(ri < pile.size());

        return pile[ri];
    }

    Cards cardsForStyle(size_t index) const {
        size_t basicPlays = hand.styles.size() * visible.freeStylePositions();
        if (index < basicPlays) {
            return {cardForStyleIndex(index)};
        } else {
            return cardsForDoublestyle(index - basicPlays);
        }
    }

    pair<size_t, Cards> findDoubleStylePlay(size_t index) const {
        auto moves = DoubleMoves(hand.styles.size());

        for (size_t i=0; i < visible.characters.size(); ++i) {
            auto &c = visible.characters[i];
            if (c.hasDoubleStyles()) {
                if (index < moves.size()) {
                    auto found = moves[index];
                    Cards r;
                    for (auto j : found) {
                        r.push_back(hand.styles[j]);
                    }
                    return {i, r};
                }
                assert(index >= moves.size());
                index -= moves.size();
            }
        }
        ERROR("bad move index");
    }

    Cards cardsForDoublestyle(size_t index) const {
        return findDoubleStylePlay(index).second;
    }

    void playDoubleStyles(size_t index) {
        size_t ci;
        Cards cards;
        tie(ci, cards) = findDoubleStylePlay(index);
        for (auto s : cards) {
            LOG("<player %zu:play_card %s>\n", id, CARD_TABLE[s].repr().c_str());
            visible.characters[ci].styles.push_back(s);
            auto it = find(begin(hand.styles), end(hand.styles), s);
            assert(it != end(hand.styles));
            hand.styles.erase(it);
        }
    }

    void playStyle(size_t index) {
        size_t basicPlays = hand.styles.size() * visible.freeStylePositions();
        if (index < basicPlays) {
            playSingleStyle(index);
        } else {
            playDoubleStyles(index - basicPlays);
        }
    }

    void playSingleStyle(size_t index) {
        TRACE();
        assert(!visible.empty());

        auto &pile = hand.styles;

        size_t ci = index / pile.size();
        size_t ri = index % pile.size();

        assert(ri < pile.size());
        auto c = pile[ri];
        auto j=0;
        auto found = false;
        for (auto i=0; i < visible.characters.size(); ++i) {
            if (visible.characters[i].canReceiveStyle()) {
                if (j == ci) {
                    visible.characters[i].styles.push_back(c);
                    found = true;
                    break;
                }
                ++j;
            }
        }
        assert(found);
        pile.erase(begin(pile)+ri);
        LOG("<player %zu:play_card %s>\n", id, CARD_TABLE[c].repr().c_str());
    }

    size_t cardForWeaponIndex(size_t index) const {
        TRACE();
        assert(!visible.empty());
        auto &pile = hand.weapons;

        size_t ri = index % pile.size();
        assert(ri < pile.size());
        return pile[ri];
    }

    void playWeapon(size_t index) {
        TRACE();
        assert(!visible.empty());
        auto &pile = hand.weapons;

        size_t ci = index / pile.size();
        size_t ri = index % pile.size();

        assert(ri < pile.size());
        auto c = pile[ri];
        auto j=0;
        auto found = false;
        for (auto i=0; i < visible.characters.size(); ++i) {
            if (visible.characters[i].canReceiveWeapon()) {
                if (j == ci) {
                    visible.characters[i].weapons.push_back(c);
                    found = true;
                    break;
                }
                ++j;
            }
        }
        assert(found);
        pile.erase(begin(pile)+ri);
        LOG("<player %zu:play_card %s>\n", id, CARD_TABLE[c].repr().c_str());
    }

    size_t cardIndexFromCharacterIndex(size_t index) const {
        TRACE();
        if (flags.affinity == NONE || flags.ignore_affinity) {
            return index;
        }
        size_t count = 0;
        for (int i = 0; i < hand.characters.size(); ++i) {
            if (isAvailable(i)) {
                if (count == index) {
                    return i;
                }
                ++count;
            }
        }
        ERROR("index not found in cardIndexFromCharacterIndex");
        return 0;
    }

    size_t playCharacter(size_t index) {
        TRACE();
        size_t i = cardIndexFromCharacterIndex(index);
        assert(i < hand.characters.size());
        size_t c = hand.characters[i];
        assert(CARD_TABLE[c].type == CHARACTER);
        visible.insertCharacter(c);
        hand.characters.erase(begin(hand.characters)+i);
        auto ca = CARD_TABLE[c].affinity;
        if (ca != NONE) {
            flags.affinity = ca;
        }
        LOG("<player %zu:play_card %s>\n", id, CARD_TABLE[c].repr().c_str());
        return c;
    }

    size_t playCard(Cards &source, Cards &target, size_t i) {
        TRACE();
        assert(!source.empty());
        assert(i < source.size());
        auto card = source[i];
        LOG("<player %zu:play_card %s>\n", id, CARD_TABLE[card].repr().c_str());

        target.push_back(source[i]);
        auto s1 = source.size();
        source.erase(begin(source)+i);
        auto s2 = source.size();
        assert(s1 > s2);
        return card;
    }

    size_t cardForMove(const Move &m) const {
        TRACE();
        switch (m.action) {
        case Move::PLAY_CHARACTER:
            if (flags.affinity == NONE) {
                return hand.characters[m.index];
            } else {
                auto c = cardIndexFromCharacterIndex(m.index);
                return hand.characters[c];
            }
        case Move::PLAY_STYLE:
            return cardForStyleIndex(m.index);
        case Move::PLAY_WEAPON:
            return cardForWeaponIndex(m.index);
        case Move::PLAY_WRENCH:
            return hand.wrenches[m.index];
        case Move::PASS:
        case Move::CONCEDE:
        case Move::NULL_ACTION:
        case Move::KNOCKOUT_CHARACTER:
        case Move::KNOCKOUT_STYLE:
        case Move::KNOCKOUT_WEAPON:
        case Move::CAPTURE_WEAPON:
        case Move::BUDDY_CHAR:
        case Move::THIEF_STEAL:
        case Move::DISCARD_ONE:
        case Move::TRADE_HAND:
        case Move::DRAW_CARD:
            ERROR("unexpected move");
        }
        return 0;
    }

    int characterValue(size_t c, const ChallengeFlags &flags) const {
        TRACE();
        if (flags.char_value_flip) {
            auto &card = CARD_TABLE[c];
            switch (card.affinity) {
            case NONE: return   card.value; break;
            case CLAN: return 5-card.value; break;
            case MONK: return 7-card.value; break;
            }
        }
        return characterValue(flags.char_point_increase);
    }

    int characterValue(size_t c, int modifier=0) const {
        TRACE();
        return CARD_TABLE[c].value + modifier;
    }

    int valueWithDoubleStyles(const Move &m, const ChallengeFlags &flags) const {
        size_t basicPlays = hand.styles.size() * visible.freeStylePositions();
        if (m.index < basicPlays) {
            return CARD_TABLE[cardForMove(m)].value;
        }
        auto dmoves = DoubleMoves(hand.styles.size());
        int i = m.index - basicPlays;

        for (auto &c : visible.characters) {
            if (c.hasDoubleStyles()) {
                if (i < dmoves.size()) {
                    auto v = 0;
                    for (auto i : dmoves[i]) {
                        auto s = hand.styles[i];
                        v += CARD_TABLE[s].value;
                    }
                    return v;
                }
                i -= dmoves.size();
                assert(i >= 0);
            }
        }
        ERROR("bad move index");
    }

    int valueForMove(const Move &m, const ChallengeFlags &flags) const {
        TRACE();
        switch (m.action) {
        case Move::PLAY_CHARACTER:
            return characterValue(cardForMove(m));
        case Move::PLAY_STYLE:
            if (visible.hasDoubleStyles()) {
                return valueWithDoubleStyles(m, flags);
            }
        case Move::PLAY_WEAPON:
        case Move::PLAY_WRENCH:
            return CARD_TABLE[cardForMove(m)].value;
        case Move::PASS:
        case Move::CONCEDE:
        case Move::NULL_ACTION:
        case Move::KNOCKOUT_CHARACTER:
        case Move::KNOCKOUT_STYLE:
        case Move::KNOCKOUT_WEAPON:
        case Move::CAPTURE_WEAPON:
        case Move::BUDDY_CHAR:
        case Move::THIEF_STEAL:
        case Move::DISCARD_ONE:
        case Move::TRADE_HAND:
        case Move::DRAW_CARD:
            return 0;
        }
    }

    string reprForMove(const Move &move) const {
        TRACE();
        switch (move.action) {
        case Move::PLAY_STYLE:
            if (visible.hasDoubleStyles()) {
                auto s = string();
                auto cards = cardsForStyle(move.index);
                for (auto card : cards) {
                    s += CARD_TABLE[card].repr() + "+";
                }
                assert(!s.empty());
                s.pop_back();
                return s;
            }
        case Move::PLAY_CHARACTER:
        case Move::PLAY_WEAPON:
        case Move::PLAY_WRENCH:
            return CARD_TABLE[cardForMove(move)].repr();
        case Move::PASS:               return "<pass>";
        case Move::CONCEDE:            return "<concede>";
        case Move::KNOCKOUT_CHARACTER: return "<knockout character>";
        case Move::KNOCKOUT_STYLE:     return "<knockout style>";
        case Move::KNOCKOUT_WEAPON:      return "<knockout weapon>";
        case Move::CAPTURE_WEAPON:       return "<capture weapon>";
        case Move::BUDDY_CHAR:         return "<buddy character>";
        case Move::THIEF_STEAL:        return "<steal card>";
        case Move::DISCARD_ONE:        return "<discard 1>";
        case Move::TRADE_HAND:         return "<trade hand>";
        case Move::DRAW_CARD:          return "<draw card>";
        case Move::NULL_ACTION:
            ERROR("unexpected null action");
        }
        return "<error>";
    }

    void print_hand() const {
#ifdef LOGGING
        TRACE();
        if (!hand.empty()) {
            LOG(" - hand (v=%d):\n", hand.value());
            if (hand.empty()) {
                LOG("   - <empty>\n");
            } else {
                int i=0;
                for (const auto &c : hand.characters) {
                    LOG("   - %s (%s)\n", CARD_TABLE[c].repr().c_str(),
                        isAvailable(i) ? "available" : "unavailable");
                    ++i;
                }
                for (const auto &s : { hand.styles, hand.weapons, hand.wrenches }) {
                    for (const auto &c : s) {
                        LOG("   - %s\n", CARD_TABLE[c].repr().c_str());
                    }
                }
            }
        }
#endif
    }

    void print_visible() const {
#ifdef LOGGING
        TRACE();
        if (gDisableLog > 0) {
            return;
        }
        if (!visible.empty()) {
            LOG(" - player %zu (v=%d):\n", id, visible.value());
            for (const auto &c : visible.characters) {
                LOG("   - %s\n", CARD_TABLE[c.character].repr().c_str());
                for (const auto &set : { c.weapons, c.styles }) {
                    for (auto r : set) {
                        LOG("     - %s\n", CARD_TABLE[r].repr().c_str());
                    }
                }
            }
            for (auto r : visible.wrenches) {
                LOG("   - %s\n", CARD_TABLE[r].repr().c_str());
            }
        }
#endif
    }

    void print() const {
#ifdef LOGGING
        TRACE();
        LOG("player %zu (v=%d):\n", id, value());
        print_hand();
        print_visible();
#endif
    }

    int value() const { TRACE(); return hand.value() + visible.value(); }
    int value(ChallengeFlags &flags) const { TRACE(); return visible.value(flags); }
};

// ---------------------------------------------------------------------------

struct DeckCardSet {
    Cards events;
    Cards characters;
    Cards skills;

    void clear() {
        TRACE();
        events.clear();
        characters.clear();
        skills.clear();
    };

    size_t size() const {
        TRACE();
        return events.size() + characters.size() + skills.size();
    }

    void shuffle() {
        TRACE();
        Shuffle(events);
        Shuffle(characters);
        Shuffle(skills);
    }

    void sort() {
        TRACE();
        Sort(events);
        Sort(characters);
        Sort(skills);
    }
};

struct Deck {
    using Ptr = shared_ptr<Deck>;

    DeckCardSet draw;
    DeckCardSet discard;

    Deck() {}

    void initialize() {
        TRACE();
        populate();
        shuffle();
    }

    Deck::Ptr clone() const {
        TRACE();
        auto d = make_shared<Deck>();
        d->draw = draw;
        d->discard = discard;
        return d;
    }

    void replaceDiscards() {
        TRACE();
        MoveCards(discard.events,     draw.events);
        MoveCards(discard.characters, draw.characters);
        MoveCards(discard.skills,  draw.skills);
    }

    void shuffle() {
        TRACE();
        LOG("<shuffle deck>\n");
        replaceDiscards();
        draw.shuffle();
        assert(size() == NUM_CARDS);
    }

    void sort()  { TRACE(); draw.sort();  discard.sort(); }
    void clear() { TRACE(); draw.clear(); discard.clear(); }
    size_t size() const { TRACE(); return draw.size() + discard.size(); }

    void populate() {
        TRACE();
        clear();

        for (int i=0; i < CARD_TABLE.size(); ++i) {
            assert(i == CARD_TABLE[i].id);
            switch (CARD_TABLE[i].type) {
                case CHARACTER:
                    draw.characters.push_back(i);
                    break;
                case EVENT:
                    draw.events.push_back(i);
                    break;
                case WEAPON:
                case STYLE:
                case WRENCH:
                    draw.skills.push_back(i);
                    break;
            }
        }
    }

    size_t drawFromPile(Cards &pile) {
        TRACE();
        auto card = pile.back();
        pile.pop_back();
        return card;
    }

    size_t drawEvent()     { TRACE(); return drawFromPile(draw.events); }
    size_t drawCharacter() { TRACE(); return drawFromPile(draw.characters); }
    size_t drawSkill()  { TRACE(); return drawFromPile(draw.skills); }

    void print() const {
#ifdef LOGGING
        TRACE();
        auto print = [](const char* title, auto pile) {
            LOG("%s (%zu):\n", title, pile.size());
            auto r = pile;
            reverse(begin(r), end(r));
            for (const auto card : r) {
                LOG(" - %s\n", CARD_TABLE[card].repr().c_str());
            }
        };

        LOG("==== DECK ====\n");
        LOG("\n");
        print("events", draw.events);
        print("characters", draw.characters);
        print("skills", draw.skills);
        print_size();
#endif
    }

    void print_size() const {
        LOG("total: draw=%zu/%zu/%zu discard=%zu/%zu/%zu = %zu\n",
            draw.events.size(),
            draw.characters.size(),
            draw.skills.size(),
            discard.events.size(),
            discard.characters.size(),
            discard.skills.size(),
            size());
    }
};

// ---------------------------------------------------------------------------

struct GameState : enable_shared_from_this<GameState> {
    using Ptr = shared_ptr<GameState>;
    using wPtr = weak_ptr<GameState>;

    Ptr self() { return shared_from_this(); }

    struct Round {
        size_t removed:4;
        size_t num_opponents:4;
        size_t pass_target:4;
        size_t i; // index into state.opponents
        GameState::wPtr _state;
        bool round_finished:1;
        bool challenge_finished:1;

        Round(GameState::Ptr gs) : _state(gs) { TRACE(); }

        string repr() {
            TRACE();
            constexpr size_t BUFSZ = 100;
            char buf[BUFSZ];
            snprintf(buf, BUFSZ, "{"
                     "removed=%zu "
                     "num_opponents=%zu "
                     "pass_target=%zu "
                     "i=%zu "
                     "round_finished=%d "
                     "challenge_finished=%d}",
                     removed,
                     num_opponents,
                     pass_target,
                     i,
                     round_finished,
                     challenge_finished);
            return buf;
        }

        void reset() {
            TRACE();
            auto state = _state.lock();
            assert(state);

            removed            = 0;
            num_opponents      = state->opponents.size();
            pass_target        = num_opponents;
            i                  = 0;
            round_finished     = false;
            challenge_finished = false;
        }

        void step(std::function<void()> f=nullptr) {
            TRACE();
            auto state = _state.lock();
            assert(state);
            auto p = state->opponents[i];
            auto move = state->dispatchPlay(p);
            checkState(p, move);
            if (f) { f(); }
        }

        void checkState(size_t p, Move &move) {
            TRACE();
            auto state = _state.lock();
            assert(state);
            assert(state->opponents.size() >= 1);
            switch (move.action) {
            case Move::CONCEDE:
                state->conceded.push_back(p);
                assert(i < state->opponents.size());
                state->opponents.erase(begin(state->opponents)+i);
                ++removed;
                assert(pass_target > 0);
                --pass_target;
                // don't increment i
                break;
            case Move::PASS:
                assert(pass_target > 0);
                --pass_target;
                // fall through
            case Move::NULL_ACTION:
                if (!state->players[p]->additional.empty()) {
                    state->players[p]->additional.erase(begin(state->players[p]->additional));
                }
            default:
                if (state->players[p]->additional.empty()) {
                    TRACE();
                    ++i;
                } else {
                    TRACE();
                    ++pass_target;
                }

                break;
            }

            if (removed == num_opponents-1) {
                LOG("only one player standing - challenge over\n");
                round_finished = true;
                challenge_finished = true;
            } else if (i >= (num_opponents-removed)) {
                // all players have moved
                round_finished = true;
                if (pass_target == 0) {
                    LOG("all players passed\n");
                    challenge_finished = true;
                }
            }
        }
    };

    ChallengeFlags flags;
    size_t num_players;
    size_t challenge_num;
    size_t current_challenger;
    Round round;
    size_t round_num;
    vector<size_t> opponents;
    size_t current_opponent;
    vector<size_t> conceded;
    Deck::Ptr deck;
    vector<Player::Ptr> players;
    vector<size_t> events;
    bool game_over;

    explicit GameState(Deck::Ptr d, vector<Player::Ptr> p)
    : num_players(p.size())
    , round(nullptr)
    , deck(d)
    , players(p)
    , game_over(false)
    {
        TRACE();
        assert(num_players >= 2);
    }

    void init() {
        TRACE();
        for (int i=0; i < num_players; ++i) {
            players[i]->id = i;
        }
        opponents.reserve(num_players);
        conceded.reserve(num_players);
        reset();
        round._state = self();
    }

    string repr() {
        TRACE();
        constexpr size_t BUFSZ=1000;
        char buf[BUFSZ];
        snprintf(buf, BUFSZ, "{"
                 "num_players=%zu "
                 "challenge_num=%zu "
                 "current_challenger=%zu "
                 "round_num=%zu "
                 "flags=%s "
                 "round=%s}",
                 num_players,
                 challenge_num,
                 current_challenger,
                 round_num,
                 flags.repr().c_str(),
                 round.repr().c_str());
        return buf;
    }

    template <typename T>
    Ptr clone() const {
        TRACE();
        vector<Player::Ptr> pv;
        for (const auto & p : players) {
            pv.emplace_back(p->clone<T>());
        }

        auto g = make_shared<GameState>(deck->clone(), pv);
        g->flags              = flags;
        g->num_players        = num_players;
        g->challenge_num      = challenge_num;
        g->current_challenger = current_challenger;
        g->round              = round;
        g->round_num          = round_num;
        g->opponents          = opponents;
        g->current_opponent   = current_opponent;
        g->conceded           = conceded;
        g->round._state       = g;
        g->events             = events;
        g->game_over          = game_over;
        return g;
    }

    void validateSize() {
        TRACE();
        auto c = 0;
        for (const auto &p : players) {
            c += p->size();
        }
        c += events.size()+deck->size();
        assert(c == NUM_CARDS);
    }

    Player *currentPlayer() const {
        TRACE();
        if (round.i >= opponents.size() || opponents[round.i] >= players.size()) {
            return nullptr;
        }
        return players[opponents[round.i]].get();
    }

    float getResult(size_t i) {
        TRACE();
        // Note: It may be useful to emphasize highest score even if losing:
        //     return (float) players[i]->score / 200.;
        size_t best = 0;
        int high = 0;
        size_t count = 0;
        for (size_t j=0; j < players.size(); ++j) {
            if (players[j]->score >= high) {
                if (players[j]->score == high) {
                    ++count;
                } else {
                    count = 0;
                }
                high = players[j]->score;
                best = j;
            }
        }
        if (count == 0 && i == best) {
            return 1;
        }
        return 0;
    }

    bool gameOver() {
        TRACE();
        game_over = deck->draw.events.empty();
        return game_over;
    }

    void reset() {
        TRACE();
        flags.reset();
        challenge_num = 0;
        // We need set a random first player.
        current_challenger = urand(num_players);
        current_opponent = 0;
        for (auto &p : players) {
            p->score = 0;
        }
    }

    void shuffleDiscard(Cards &draw, Cards &discard) {
        // We draw from the end of the vector, so in order to append cards, we
        // must first reverse the remaining items, append, then reverse again.
        reverse(begin(draw), end(draw));
        Shuffle(discard);
        MoveCards(discard, draw);
        reverse(begin(draw), end(draw));
    }

    void shuffleDiscardCharacters() {
        shuffleDiscard(deck->draw.characters, deck->discard.characters);
    }

    void shuffleDiscardSkills() {
        shuffleDiscard(deck->draw.skills, deck->discard.skills);
    }

    void deal() {
        TRACE();
        // Draw up to target character and skill card counts for each player.

        constexpr size_t TARGET_C = 4;
        constexpr size_t TARGET_R = 6;

        for (auto & p : players) {
            p->flags.affinity = NONE;
            auto &h = p->hand;

            auto c = min(TARGET_C, h.characters.size());
            auto r = min(TARGET_R, h.skillsSize());

            if (TARGET_C-c > deck->draw.characters.size()) {
                LOG("<shuffle characters>\n");
                shuffleDiscardCharacters();
            }

            if (TARGET_R-r > deck->draw.skills.size()) {
                LOG("<shuffle skills>\n");
                shuffleDiscardSkills();
            }

            assert(TARGET_R-r <= deck->draw.skills.size());
            assert(TARGET_C-c <= deck->draw.characters.size());

            auto tc = TARGET_C-c;
            auto tr = TARGET_R-r;

            for (int j=0; j < tc; ++j) {
                auto c = deck->drawCharacter();
                LOG("<player %zu:draw %s>\n", p->id, CARD_TABLE[c].repr().c_str());
                h.characters.push_back(c);
            }

            for (int j=0; j < tr; ++j) {
                auto c = deck->drawSkill();
                LOG("<player %zu:draw %s>\n", p->id, CARD_TABLE[c].repr().c_str());
                h.insertCard(c);
            }
        }
    }

    void discard(PlayerHand &hand) {
        TRACE();
        MoveCards(hand.characters, deck->discard.characters);
        MoveCards(hand.styles,     deck->discard.skills);
        MoveCards(hand.weapons,    deck->discard.skills);
        MoveCards(hand.wrenches,   deck->discard.skills);
    }

    void discard(const PlayerVisible::Character &c) {
        TRACE();
        CopyCards(c.styles,  deck->discard.skills);
        CopyCards(c.weapons, deck->discard.skills);
        deck->discard.characters.push_back(c.character);
    }

    void discard(PlayerVisible &vis) {
        TRACE();
        for (const auto &c : vis.characters) {
            discard(c);
        }

        MoveCards(vis.wrenches, deck->discard.skills);
        vis.characters.clear();
        assert(vis.empty());
    }

    void discard(Player *p, bool discardHand=false) {
        TRACE();
        discard(p->visible);
        if (discardHand) {
            discard(p->hand);
            assert(p->hand.empty());
        }
        assert(p->visible.empty());
    }

    // Used by W_CLEAR_FIELD to remove all played cards and reset affinity.
    void discardVisible() {
        TRACE();
        for (auto & p : players) {
            //p->print_visible();
            discard(p->visible);
            p->flags.affinity = NONE;
        }
    }

    // At the end of a challenge, discard the event and player cards, including
    // the hand if the game is over.
    void discard() {
        TRACE();
        auto discardAll = gameOver();
        for (auto & p : players) {
            discard(p.get(), discardAll);
        }
        if (discardAll) {
            for (auto event : events) {
                deck->discard.events.push_back(event);
            }
            events.clear();
        }
    }

    // Randomize the hidden cards with respect to player i.
    void randomizeHiddenState(size_t i) {
        TRACE();
        auto hidden = make_shared<Deck>();

        // Copy all unseen cards (draw pile) from the main to the hidden deck.
        CopyCards(deck->draw.characters, hidden->draw.characters);
        CopyCards(deck->draw.skills,  hidden->draw.skills);

        // Copy all cards from players to hidden deck.
        for (auto &p : players) {
            if (p->id != i) {
                CopyCards(p->hand.characters, hidden->draw.characters);
                CopyCards(p->hand.styles,     hidden->draw.skills);
                CopyCards(p->hand.weapons,    hidden->draw.skills);
                CopyCards(p->hand.wrenches,   hidden->draw.skills);
            }
        }

        // Shuffle cards. Now we can redistribute from the deck
        hidden->draw.shuffle();

        // Shuffle events directly since we didn't copy them.
        Shuffle(deck->draw.events);

        // Distribute cards to the players
        for (auto &p : players) {
            if (p->id != i) {
                auto &hand = p->hand;
                // Remember the size of the hand.
                auto nc = hand.characters.size();
                auto nr = hand.styles.size() +
                          hand.weapons.size() +
                          hand.wrenches.size();

                // Now it is safe to clear the hand and redistribute.
                hand.clear();
                for (int j=0; j < nc; ++j) {
                    auto c = hidden->drawCharacter();
                    hand.characters.push_back(c);
                }
                for (int j=0; j < nr; ++j) {
                    auto c = hidden->drawSkill();
                    hand.insertCard(c);
                }
            }
        }

        // Move the remaining deck
        deck->draw.characters = std::move(hidden->draw.characters);
        deck->draw.skills  = std::move(hidden->draw.skills);
    }

    void initializeOpponents() {
        TRACE();
        opponents.clear();

        size_t j;
        for (size_t i=0; i < num_players; ++i) {
            j = (i+current_challenger) % num_players;
            opponents.push_back(j);
        }
    }

    void incrementChallenger() {
        TRACE();
        ++challenge_num;
        current_challenger = (current_challenger+1) % num_players;
    }

    int playedCharacterValue() const {
        TRACE();
        int value = 0;
        for (const auto &s : { opponents, conceded }) {
            for (const auto p : s) {
                value += players[p]->visible.characterValue(flags);
            }
        }
        return value;
    }

    void score() {
        TRACE();
        // TODO inline point tracking into game play to eliminate these loops
        assert(!opponents.empty());
        size_t winner = opponents[0];
        bool tie = false;
        if (opponents.size() == 1) {
            LOG("player %zu is the winner\n", opponents[0]);
            players[winner]->score += playedCharacterValue();
        } else {
            // more than one player standing - we need to identify the winner
            auto points = 0;
            auto high   = 0;

            int count = 0;
            for (int i=0; i < opponents.size(); ++i) {
                auto p = opponents[i];
                points += players[p]->visible.characterValue(flags);

                int value = players[p]->visible.value(flags);
                if (value > high) {
                    high = value;
                    winner = p;
                    count = 0;
                } else if (value == high) {
                    ++count;
                }
            }
            if (count > 0) {
                LOG("tie - no points for any one\n");
                tie = true;
            } else {
                for (auto i : conceded) {
                    points += players[i]->visible.characterValue(flags);
                }
                LOG("player %zu is the winner\n", winner);
                players[winner]->score += points;
            }
        }
        for (auto &set : { conceded, opponents }) {
            for (auto i : set) {
                if (tie || i != winner) {
                    players[i]->stats.addLoss(players[i]->flags.affinity);
                } else {
                    players[i]->stats.addWin(players[i]->flags.affinity);
                }
            }
        }
        printScore();
    }

    Move dispatchPlay(size_t i) {
        TRACE();
        auto move = players[i]->move(self());
        doMove(i, move);
        return move;
    }

    MoveList getCurrentMoveList() const {
        TRACE();
        MoveList moves;
        if (game_over) {
            return moves;
        }

        MoveCount moveCount(this);
        for (int i=0; i < moveCount.count; ++i) {
            moves.push_back(moveCount.move(i));
        }
        return moves;
    }

    void doMove(size_t i, Move &move) {
        TRACE();
        auto p = players[i];
        size_t card;
        switch (move.action) {
        case Move::PLAY_CHARACTER:
            card = p->playCharacter(move.index);
            handleCardAction(CARD_TABLE[card].action, i);
            break;
        case Move::PLAY_STYLE:
            assert(!flags.no_styles);
            p->playStyle(move.index);
            break;
        case Move::PLAY_WEAPON:
            assert(!flags.no_weapons);
            p->playWeapon(move.index);
            break;
        case Move::PLAY_WRENCH:
            card = p->playCard(p->hand.wrenches, p->visible.wrenches, move.index);
            handleCardAction(CARD_TABLE[card].action, i);
            break;
        case Move::PASS:
            LOG("<player %zu:pass>\n", i);
            break;
        case Move::CONCEDE:
            LOG("<player %zu:concede>\n", i);
            break;
        case Move::KNOCKOUT_CHARACTER: knockoutCharacter(move); break;
        case Move::KNOCKOUT_STYLE:     knockoutStyle(move); break;
        case Move::KNOCKOUT_WEAPON:    knockoutWeapon(move); break;
        case Move::CAPTURE_WEAPON:     captureWeapon(move); break;
        case Move::THIEF_STEAL:        thiefSteal(move); break;
        case Move::TRADE_HAND:         tradeHand(move); break;
        case Move::DISCARD_ONE:        discardOne(move); break;
        case Move::DRAW_CARD:          playerDrawCard(move); break;
        case Move::BUDDY_CHAR:         playerBringChar(move); break;
        case Move::NULL_ACTION:
            break;
        }
    }

    void playerBringChar(const Move &move) {
        auto p = currentPlayer();
        if (move.index > 0) {
            auto c = p->playCharacter(move.index-1);
            LOG("<player %zu:play_card %s>\n", p->id, CARD_TABLE[c].repr().c_str());
            handleCardAction(CARD_TABLE[c].action, p->id);
        }
        p->additional.erase(begin(p->additional));
    }

    void tradeHand(const Move &move) {
        auto p = currentPlayer();
        int i = 0;
        bool found = false;
        for (auto o : opponents) {
            if (o != p->id && !players[o]->hand.empty()) {
                if (i == move.index) {
                    LOG("<player %zu:swap_hand %zu>\n", p->id, o);
                    std::swap(p->hand, players[o]->hand);
                    found = true;
                    break;
                }
                ++i;
            }
        }
        assert(found);
        p->additional.erase(begin(p->additional));
    }

    void thiefSteal(const Move &move) {
        auto p = currentPlayer();
        int i = 0;
        bool found = false;
        for (auto o : opponents) {
            if (o != p->id && !players[o]->hand.empty()) {
                if (i == move.index) {
                    auto card = players[o]->hand.drawRandomCard();
                    LOG("<player %zu:steal %zu %s>\n", p->id, o, CARD_TABLE[card.first].repr().c_str());
                    p->hand.insertCard(card.first);
                    found = true;
                    break;
                }
                ++i;
            }
        }
        assert(found);
        p->additional.erase(begin(p->additional));
    }

    void discardOne(const Move &move) {
        auto p = currentPlayer();

        size_t card;
        CardType type;
        tie(card, type) = p->hand.drawIndexed(move.index);
        LOG("<player %zu:discard %s>\n", p->id, CARD_TABLE[card].repr().c_str());
        switch (type) {
        case CHARACTER:
            deck->discard.characters.push_back(card);
            break;
        case STYLE:
        case WEAPON:
        case WRENCH:
            deck->discard.skills.push_back(card);
            break;
        case EVENT:
            ERROR("unexpected event");
        }
        p->additional.erase(begin(p->additional));
    }

    void knockoutCharacter(const Move &move) {
        TRACE();
        auto p = currentPlayer();
        int j = 0;
        for (auto o : opponents) {
            if (o != p->id) {
                auto &v = players[o]->visible;
                for (size_t k=0; k < v.characters.size(); ++k) {
                    auto &c = v.characters[k];
                    if (c.weapons.empty() && c.styles.empty() && !c.isImmune()) {
                        if (j == move.index) {
                            LOG("<player %zu:knockout player %zu %s>\n", p->id, o,
                                CARD_TABLE[v.characters[k].character].repr().c_str());
                            deck->discard.characters.push_back(c.character);
                            v.characters.erase(begin(v.characters)+k);
                            if (v.characters.empty()) {
                                players[o]->flags.affinity = NONE;
                            }

                            p->additional.erase(begin(p->additional));
                            return;
                        } else {
                            ++j;
                        }
                    }
                }
            }
        }
        ERROR("character not found");
    }

    void knockoutStyle(const Move &move) {
        TRACE();
        int i = 0;
        auto p = currentPlayer();
        for (auto o : opponents) {
            if (o != p->id) {
                for (auto &c : players[o]->visible.characters) {
                    if (!c.styles.empty() && !c.isImmune()) {
                        if (i == move.index) {
                            auto card = c.styles.back();
                            LOG("<player %zu:knockout player %zu %s>\n", p->id, o,
                                CARD_TABLE[card].repr().c_str());
                            deck->discard.skills.push_back(card);
                            c.styles.pop_back();
                            p->additional.erase(begin(p->additional));
                            return;
                        }
                        ++i;
                    }
                }
            }
        }
        ERROR("style not found");
    }

    void knockoutWeapon(const Move &move, bool withCapture=false) {
        auto p = currentPlayer();
        int i = 0;
        for (auto o : opponents) {
            if (o != p->id) {
                for (auto &c : players[o]->visible.characters) {
                    if (!c.weapons.empty() && !c.isImmune()) {
                        if (i == move.index) {
                            auto t = c.weapons.back();
                            if (withCapture) {
                                LOG("<player %zu:capture player %zu %s>\n", p->id, o,
                                    CARD_TABLE[t].repr().c_str());
                                p->hand.weapons.push_back(t);
                            } else {
                                LOG("<player %zu:knockout player %zu %s>\n", p->id, o,
                                    CARD_TABLE[t].repr().c_str());
                                deck->discard.skills.push_back(t);
                            }
                            c.weapons.pop_back();
                            p->additional.erase(begin(p->additional));
                            return;
                        }
                        ++i;
                    }
                }
            }
        }
        ERROR("weapon not found");
    }

    void captureWeapon(const Move &move) {
        return knockoutWeapon(move, true);
    }

    void playerDrawCard(const Move &move) {
        TRACE();
        auto p = currentPlayer();
        switch (move.index) {
            case 0:
                if (!deck->draw.characters.empty()) {
                    auto c = deck->drawCharacter();
                    p->hand.characters.push_back(c);
                    LOG("<player %zu:draw %s>\n", p->id, CARD_TABLE[c].repr().c_str());
                    break;
                }
            case 1:
                if (!deck->draw.skills.empty()) {
                    auto c = deck->drawSkill();
                    p->hand.insertCard(c);
                    LOG("<player %zu:draw %s>\n", p->id, CARD_TABLE[c].repr().c_str());
                } else {
                    if (!deck->draw.characters.empty()) {
                        auto c = deck->drawCharacter();
                        p->hand.characters.push_back(c);
                        LOG("<player %zu:draw %s>\n", p->id, CARD_TABLE[c].repr().c_str());
                    }
                }
                break;
            default:
                ERROR("invalid");
        }
        p->additional.erase(begin(p->additional));
    }

    void playersDrawOneCharacter() {
        TRACE();
        for (auto i : opponents) {
            if (deck->draw.characters.empty()) { break; }
            players[i]->hand.characters.push_back(deck->drawCharacter());
        }
    }

    void playersDrawTwoSkills() {
        TRACE();
        auto done = false;
        for (auto i : opponents) {
            for (int j=0; j < 2; ++j) {
                if (deck->draw.skills.empty()) {
                    done = true;
                    break;
                }
                players[i]->hand.insertCard(deck->drawSkill());
            }
            if (done) { break; }
        }
    }

    void playersDiscardTwo() {
        TRACE();
        for (auto i : opponents) {
            for (int j=0; j < 2; ++j) {
                auto p = players[i];
                if (p->hand.empty()) {
                    break;
                }
                auto r = p->hand.drawRandomCard();
                LOG("<player %zu:discard %s>\n",
                    p->id,
                    CARD_TABLE[r.first].repr().c_str());
                switch (r.second) {
                case CHARACTER:
                    deck->discard.characters.push_back(r.first);
                    break;
                case STYLE:
                case WEAPON:
                case WRENCH:
                    deck->discard.skills.push_back(r.first);
                    break;
                case EVENT:
                    ERROR("unexpected card");
                    break;
                }
            }
        }
    }

    void playersRandomSteal() {
        TRACE();
        auto osize = opponents.size();

        // The chosen card from each player's hand.
        vector<size_t> stolen;
        stolen.reserve(osize);

        // Some players may not have a card, so track whether found one.
        vector<bool> hasCard;
        hasCard.reserve(osize);

        // Choose a card from each player.
        for (auto i : opponents) {
            auto p = players[i];
            if (!p->hand.empty()) {
                auto card = p->hand.drawRandomCard().first;
                stolen.push_back(card);
                hasCard.push_back(true);
            } else {
                stolen.push_back(0);
                hasCard.push_back(false);
            }
        }

        // Transfer each card (if available) to the player to the left.
        for (int i=0; i < osize; ++i) {
            if (hasCard[i]) {
                int left = (i + 1) % osize;
                auto recipient = players[opponents[left]];
                LOG("<transfer_card:%zu->%zu %s>\n",
                    opponents[i],
                    opponents[left],
                    CARD_TABLE[stolen[i]].repr().c_str());
                recipient->hand.insertCard(stolen[i]);
            }
        }
    }

    void handleCardAction(CardAction action, size_t player) {
        TRACE();
        if (action == NULL_ACTION) {
            return;
        }

        auto &p = players[player];
        switch (action) {
            case E_CHAR_VALUE_FLIP:
                flags.char_value_flip = true;
                break;
            case E_NO_STYLES:
                flags.no_styles = true;
                break;
            case E_NO_WEAPONS:
                flags.no_weapons = true;
                break;
            case E_DRAW_ONE_CHAR:   playersDrawOneCharacter(); break;
            case E_DRAW_TWO_SKILLS: playersDrawTwoSkills(); break;
            case E_DISCARD_TWO:     playersDiscardTwo(); break;
            case E_RANDOM_STEAL:    playersRandomSteal(); break;

            case E_CHAR_POINT_INCREASE:
                flags.char_point_increase = true;
                break;

            case C_IGNORE_AFFINITY:
                p->flags.ignore_affinity = true;
                break;
            case W_CLEAR_FIELD: discardVisible(); break;
            case C_KNOCKOUT_CHARACTER: p->addAction(Move::KNOCKOUT_CHARACTER); break;
            case C_KNOCKOUT_STYLE:     p->addAction(Move::KNOCKOUT_STYLE); break;
            case C_KNOCKOUT_WEAPON:    p->addAction(Move::KNOCKOUT_WEAPON); break;
            case C_CAPTURE_WEAPON:     p->addAction(Move::CAPTURE_WEAPON); break;
            case C_BUDDY_CHAR:         p->addAction(Move::BUDDY_CHAR); break;
            case C_THIEF:              p->addAction(Move::THIEF_STEAL); break;
            case C_DISCARD:            p->addAction(Move::DISCARD_ONE); break;
            case C_TRADE:              p->addAction(Move::TRADE_HAND); break;
            case C_DRAW_CARD:          p->addAction(Move::DRAW_CARD); break;
            case W_NULLIFY_STYLE:      p->addAction(Move::KNOCKOUT_STYLE); break;
            case W_DISARM_PLAYER:      p->addAction(Move::KNOCKOUT_WEAPON); break;
            case C_INCREASE_WEAPON_LIMIT:
            case C_INCREASE_STYLE_RATE:
            case C_DOUBLE_WEAPON_VALUE:
            case C_STYLE_POINTS:
            case C_NO_STYLES:
            case C_NO_WEAPONS:
            case C_IMMUNE:
                // nothing to do
                break;
            case NULL_ACTION:
                return;
                break;
        }
        LOG(">> %s\n", CardActionDescription(action).c_str());
    }

    void nameOpponents() {
        TRACE();
        conceded.clear();
        initializeOpponents();
        printPlayerList();
    }

    void playEventCard() {
        TRACE();
        flags.reset();
        events.push_back(deck->drawEvent());
        LOG("event: %s\n", CARD_TABLE[events.back()].repr().c_str());
        handleCardAction(CARD_TABLE[events.back()].action, 0);
    }

    void beginChallenge() {
        TRACE();
        assert(!game_over);
        LOG("\n");
        LOG("-- BEGIN CHALLENGE #%zu--\n", challenge_num+1);
        LOG("\n");
        deck->print();
        validateSize();
        deal();

        // Note: it doesn't matter if this happens before or after the challenger plays a
        // character.
        nameOpponents();

        // Play event card.
        playEventCard();

        printOpponents();
        round_num = 0;
        round.reset();
    }

    void printPointStatus() {
#ifdef LOGGING
        TRACE();
        int points = 0;
        auto s = string("[");
        for (size_t i=0; i < players.size(); ++i) {
            s += "p" + to_string(i) + "=" + to_string(players[i]->visible.value(flags)) + " ";
            points += players[i]->visible.characterValue(flags);
        }
        s += "points=" + to_string(points) + "]";
        LOG("%s\n", s.c_str());
#endif
    }

    void playRounds(std::function<void()> f=nullptr) {
        TRACE();
        while (!round.challenge_finished) {
            validateSize();
            LOG("\n");
            LOG("ROUND <%zu>\n", round_num);
            round.reset();
            while (!round.round_finished) {
                round.step(f);
                //for (auto &p: players) { p->print_visible(); }
            }
            if (!gDisableLog) {
                printPointStatus();
                if (round.challenge_finished) {
                    printValue();
                }
            }
            ++round_num;
        }
    }

    void endChallenge() {
        TRACE();
        score();
        discard();
        incrementChallenger();
        validateSize();
    }

    void finishGame() {
        TRACE();
        // Finish current round
        while (!round.round_finished) {
            assert(!round.challenge_finished);
            round.step();
        }

        // Finish current challenge
        playRounds();
        endChallenge();

        // Finish remaining challenges
        playGame();
    }

    void playGame(std::function<void()> f=nullptr) {
        TRACE();
        while (!gameOver()) {
            beginChallenge();
            playRounds(f);
            endChallenge();
            if (f) { f(); }
        }
    }

    void print() const {
#ifdef LOGGING
        for (const auto & p : players) {
            p->print();
        }
#endif
    }

    void printOpponents() const {
#ifdef LOGGING
        for (const auto & p : opponents) {
            players[p]->print();
        }
#endif
    }

    void printPlayerList() {
#ifdef LOGGING
        for (auto i : opponents) {
            assert(i == players[i]->id);
            LOG("- player %zu%s\n", i, i == current_challenger ? " (challenger)" : "");
        }
#endif
    }

    void printValue() {
#ifdef LOGGING
        int value = 0;
        int points = 0;
        for (const auto & set : { opponents, conceded }) {
            for (const auto i : set) {
                value += players[i]->visible.value(flags);
                points += players[i]->visible.characterValue(flags);
            }
        }
        LOG("value/points: (total=%d/%d)\n", value, points);
        for (auto i : opponents) {
            LOG(" - player %zu: %d/%d (opponent%s)\n",
                    i,
                    players[i]->visible.value(flags),
                    players[i]->visible.characterValue(flags),
                    i == current_challenger ? " - challenger" : "");
        }
        for (auto i : conceded) {
            LOG(" - player %zu: %d/%d (conceded%s)\n",
                    i,
                    players[i]->visible.value(flags),
                    players[i]->visible.characterValue(flags),
                    i == current_challenger ? " - challenger" : "");
        }
#endif
    }

    void printScore() const {
        if (gDisableLog > 0) {
            return;
        }
#ifdef LOGGING
        LOG("score:\n");
        for (const auto &p : players) {
            LOG(" - player %zu: %d\n", p->id, p->score);
        }
#endif
    }
};

// ---------------------------------------------------------------------------

MoveCount::MoveCount(const GameState *_state)
: state(_state)
, count(0)
, wmax(0)
, smax(0)
, cmax(0)
, imax(0)
, can_concede(state->opponents.size() > 1)
, vc_empty(state->currentPlayer()->visible.characters.empty())
, hc_empty(state->currentPlayer()->hand.characters.empty())
{
    countMoves();
}

void MoveCount::countMoves() {
    TRACE();
    auto p = state->currentPlayer();
    if (!p->additional.empty()) {
        buildAdditional();
        return;
    }

    const auto &hand = p->hand;
    const auto &visible = p->visible;

    // No characters played?
    if (vc_empty) {
        // No characters in hand?
        if (hc_empty) {
            count = 1; // concede
            return;
        }
        count = hand.characters.size();
        return;
    }

    count = 1+can_concede;
    imax = count;

    count += p->availableCharacters();
    cmax = count;

    smax = count;
    if (!state->flags.no_styles) {
        count += visible.styleMoveCount(hand.styles.size());
        smax = count;
    }

    count += hand.wrenches.size();
    wmax = count;

    if (!state->flags.no_weapons && visible.canReceiveWeapon()) {
        auto sz = hand.weapons.size() * visible.freeWeaponPositions();
        count += sz;
    }
}

void MoveCount::buildAdditional() {
    TRACE();
    auto p = state->currentPlayer();
    additional = p->additional;
    const auto &a = additional.front();
    switch (a.action) {
    case Move::DRAW_CARD:
        // 2 moves: draw a character or a skills
        count = 2;
        break;
    case Move::KNOCKOUT_CHARACTER:
        count = 0;
        for (auto o : state->opponents) {
            if (o != p->id) {
                for (const auto &c : state->players[o]->visible.characters) {
                    // Only consider characters that have no styles or weapons
                    if (c.weapons.empty() && c.styles.empty() && !c.isImmune()) {
                        ++count;
                    }
                }
            }
        }
        break;
    case Move::KNOCKOUT_STYLE:
        count = 0;
        for (auto o : state->opponents) {
            if (o != p->id) {
                for (const auto &c : state->players[o]->visible.characters) {
                    if (!c.styles.empty() && !c.isImmune()) {
                        ++count;
                    }
                }
            }
        }
        break;
    case Move::KNOCKOUT_WEAPON:
    case Move::CAPTURE_WEAPON:
        count = 0;
        for (auto o : state->opponents) {
            if (o != p->id) {
                for (const auto &c : state->players[o]->visible.characters) {
                    if (!c.weapons.empty() && !c.isImmune()) {
                        ++count;
                    }
                }
            }
        }
        break;
    case Move::DISCARD_ONE:
        count = p->hand.size();
        break;
    case Move::TRADE_HAND:
    case Move::THIEF_STEAL:
        count = 0;
        for (auto o : state->opponents) {
            if (o != p->id) {
                if (!state->players[o]->hand.empty()) {
                    count++;
                }
            }
        }
        break;
    case Move::BUDDY_CHAR:
        // 0 = pass
        count = 1+p->availableCharacters();
        break;
    default:
        ERROR("unhandled move type");
    }
}

Move MoveCount::move(size_t i) {
    TRACE();
    auto p = state->currentPlayer();

    if (!additional.empty()) {
        if (count == 0) {
            p->additional.erase(begin(p->additional));
            additional.erase(begin(additional));
            assert(p->additional.empty());
            return Move::NullMove();
        }
        switch (additional.front().action) {
        case Move::DRAW_CARD:
        case Move::KNOCKOUT_CHARACTER:
        case Move::KNOCKOUT_STYLE:
        case Move::KNOCKOUT_WEAPON:
        case Move::CAPTURE_WEAPON:
        case Move::DISCARD_ONE:
        case Move::THIEF_STEAL:
        case Move::TRADE_HAND:
        case Move::BUDDY_CHAR:
            return Move {additional.front().action, i};
        case Move::PLAY_CHARACTER:
        case Move::PLAY_STYLE:
        case Move::PLAY_WEAPON:
        case Move::PLAY_WRENCH:
        case Move::PASS:
        case Move::CONCEDE:
        case Move::NULL_ACTION:
            // Continue through to regular case.
            break;
        }
    }

    assert(i < count);

    if (vc_empty) {
        if (hc_empty) {
            LOG("player %zu is out of characters\n", p->id);
            return {Move::CONCEDE, 0};
        }
        return {Move::PLAY_CHARACTER, i};
    }

    if      (i >= wmax) { i -= wmax; return Move {Move::PLAY_WEAPON,    i}; }
    else if (i >= smax) { i -= smax; return Move {Move::PLAY_WRENCH,    i}; }
    else if (i >= cmax) { i -= cmax; return Move {Move::PLAY_STYLE,     i}; }
    else if (i >= imax) { i -= imax; return Move {Move::PLAY_CHARACTER, i}; }

    assert(i == 0 || i == 1);
    if (i == 1) {
        return Move {Move::CONCEDE, 0};
    }
    return Move {Move::PASS, 0};
}

void MoveCount::print() {
    LOG("moves:\n");
    for (int i=0; i < count; ++i) {
        LOG(" - %s\n", move(i).repr().c_str());
    }
}

// ---------------------------------------------------------------------------

struct ViewCard {
    size_t id;
    int rotation;

    ViewCard() : id(-1), rotation(0) {}
    ViewCard(size_t i) : id(-1), rotation(0) { update(i); }

    void update(size_t i) {
        if (i != id) {
            id = i;
            rotation = RandFloat() * 4 - 2;
        }
    }
};

template <typename V, typename U>
static void ResizeVector(size_t target, U &r) {
    if (target < r.size()) {
        r.clear();
    }

    while (target > r.size()) {
        r.emplace_back(V());
    }
}

struct ViewCharacter {
    ViewCard character;
    vector<ViewCard> styles;
    vector<ViewCard> weapons;

    void update(const PlayerVisible::Character &c) {
        if (c.character != character.id) {
            character.update(c.character);
        }

        ResizeVector<ViewCard>(c.styles.size(), styles);
        ResizeVector<ViewCard>(c.weapons.size(), weapons);

        for (size_t i=0; i < styles.size(); ++i) {
            styles[i].update(c.styles[i]);
        }
        for (size_t i=0; i < weapons.size(); ++i) {
            weapons[i].update(c.weapons[i]);
        }
    }
};

struct ViewPlayer {
    vector<ViewCharacter> characters;

    void update(const Player::Ptr &p) {
        const auto &v = p->visible;
        ResizeVector<ViewCharacter>(v.characters.size(), characters);
        for (size_t i = 0; i < characters.size(); ++i) {
            characters[i].update(v.characters[i]);
        }
    }
};

struct ViewState {
    vector<ViewPlayer> players;

    void update(const GameState::Ptr &state) {
        ResizeVector<ViewPlayer>(state->players.size(), players);

        for (size_t i = 0; i < players.size(); ++i) {
            players[i].update(state->players[i]);
        }
    }
};

// ---------------------------------------------------------------------------

struct UICard {
    shared_ptr<sf::RenderTexture> rtext;

    static constexpr size_t width = 200;
    static constexpr size_t height = width * 1.4;

    UICard(const CardTableEntry &card, const sf::Font &font)
    : rtext(make_shared<sf::RenderTexture>())
    {

        auto text = sf::Text();
        text.setFont(font);
        auto s = to_string(card.value) + " " + card.label;
        auto n = s.find('(');
        if (n != string::npos) {
            s.insert(n, "\n");
        }
        text.setString(s);
        text.setCharacterSize(36);
        text.setColor(sf::Color::Black);

        if (!rtext->create(width, height)) {
            ERROR("could not create texture");
        }
        rtext->clear(sf::Color::White);

        sf::Texture paper;
        paper.setSmooth(true);
        paper.loadFromFile("paper.jpg");

        sf::Sprite sprite(paper);
        sprite.setScale(width / sprite.getLocalBounds().width,
                        height / sprite.getLocalBounds().height);
        rtext->draw(sprite);

        text.setPosition(4, 4);
        rtext->draw(text);
        rtext->display();
    }

    const sf::Texture &texture() const {
        assert(rtext);
        return rtext->getTexture();
    }

    static vector<UICard> cards;

    static void RenderCards(const sf::Font &font) {
        cards.clear();
        for (size_t i=0; i < CARD_TABLE_PROTO.size(); ++i) {
            cards.emplace_back(UICard(CARD_TABLE_PROTO[i], font));
        }
    }
};

vector<UICard> UICard::cards;

// ---------------------------------------------------------------------------

template <typename T>
struct MonkeyChallenge : public enable_shared_from_this<MonkeyChallenge<T>> {
    using Ptr = shared_ptr<MonkeyChallenge>;

    ViewState view_state;
    Deck::Ptr deck;
    GameState::Ptr state;
    size_t round_num;
    thread t;

    sf::RenderWindow window;
    sf::Font font;
    sf::Texture felt_tx;
    sf::Sprite felt_sprite;

    explicit MonkeyChallenge(vector<Player::Ptr> players)
    : deck(make_shared<Deck>())
    , state(make_shared<GameState>(deck, players))
#ifdef DISPLAY_UI
    , window(sf::VideoMode(2048, 1536), "Monkey", sf::Style::Default)
#endif
    {
        TRACE();
#ifdef DISPLAY_UI
        window.setVerticalSyncEnabled(true);

        if (!font.loadFromFile("ShrimpFriedRiceNo1.ttf")) {
            ERROR("could not load font");
        }
        UICard::RenderCards(font);
        felt_tx.loadFromFile("felt.jpg");
        felt_sprite.setTexture(felt_tx);
        felt_sprite.setScale(2048 / felt_sprite.getLocalBounds().width,
                             1536 / felt_sprite.getLocalBounds().height);
#endif

        state->init();
        deck->initialize();
    }

    mutex mtx;
    ChallengeFlags flags;
    GameState::Ptr clone;

    void doClone() {
        lock_guard<mutex> lock(mtx);
        clone = state->clone<T>();
    }


    Ptr Self() {
        return MonkeyChallenge<T>::shared_from_this();
    }

    void play() {
        TRACE();

        auto self = Self();
        t = thread([self] {
            auto f = [self] { self->doClone(); };
            self->state->reset();
            self->state->playGame(f);
            self->state->deck->print_size();
            self->state->deck->shuffle();
            self->state->deck->print_size();
        });
#ifdef DISPLAY_UI
        t.detach();
#else
        t.join();
#endif
    }

    void loop() {
#ifdef DISPLAY_UI
        doClone();
        while (window.isOpen()) {
            auto event = sf::Event();
            while (window.pollEvent(event)) {
                if (event.type == sf::Event::Closed) {
                    window.close();
                }
            }
            
            window.clear(sf::Color::White);
            window.draw(felt_sprite);
            
            {
                lock_guard<mutex> lock(mtx);
                view_state.update(clone);
                renderPlayers();
            }
            window.display();
        }
#endif
    }

    void renderCard(int x, int y, const ViewCard &card) {
        auto proto = CARD_TABLE[card.id].prototype;
        sf::Sprite sprite;
        assert(proto < UICard::cards.size());
        sprite.setTexture(UICard::cards[proto].texture());
        auto tx = UICard::width >> 1;
        auto ty = UICard::height >> 1;
        sprite.setOrigin(tx, ty);
        sprite.rotate(card.rotation);
        sprite.setPosition(x+tx, y+ty);
        window.draw(sprite);
    }

    static constexpr size_t score_font_size = 60;
    static constexpr size_t score_x_offset  = 10;
    static constexpr size_t font_size       = 24;
    static constexpr size_t stack_y_offset  = font_size + 10;
    static constexpr size_t stack_x_offset  = (UICard::width >> 1) + 15;

    static constexpr size_t initial_y       = 40;
    static constexpr size_t initial_x       = score_font_size * 2 + stack_x_offset;

    static constexpr size_t char_x_offset   = stack_x_offset * 2 + 30 + UICard::width;
    static constexpr size_t char_y_offset   = stack_y_offset * 4 + UICard::height;

    void renderCharacter(int x, int y, const ViewCharacter &vc) {
        renderCard(x, y, vc.character);

        int sx = x - stack_x_offset;
        int sy = y + stack_y_offset;
        for (const auto & s: vc.styles) {
            renderCard(sx, sy, s);
            sy += stack_y_offset;
        }

        int wx = x + stack_x_offset;
        int wy = y + stack_y_offset;
        for (const auto & w : vc.weapons) {
            renderCard(wx, wy, w);
            wy += stack_y_offset;
        }
    }

    void renderScore(Player *player, int x, int y) {
        auto text = sf::Text();
        text.setFont(font);
        auto s = to_string(player->visible.value(clone->flags)) + "\n" + to_string(player->score);
        text.setString(s);
        text.setCharacterSize(score_font_size);
        text.setColor(sf::Color::Black);

        text.setPosition(x+score_x_offset, y);
        window.draw(text);
    }

    void renderPlayers() {
        int y = initial_y;
        size_t i = 0;
        for (const auto &p : state->players) {

            int x = initial_x;

            renderScore(p.get(), 0, y);

            for (const auto &c: view_state.players[i].characters) {
                renderCharacter(x, y, c);
                x += char_x_offset;
            }
            y += char_y_offset;
            ++i;
        }
    }
};

// ---------------------------------------------------------------------------

struct RandomPlayer : Player {
    string name() const override { return "RandomPlayer"; }

    Move move(GameState::Ptr gs) override {
        TRACE();
        auto mc = MoveCount(gs.get());
        auto move = mc.move(urand(mc.count));
        if (move.action == Move::CONCEDE) {
            return {Move::PASS, 0};
        }
        return move;
    }
};

// ---------------------------------------------------------------------------

struct HumanPlayer : Player {
    string name() const override { return "Human"; }

    void printMoves(MoveList &m, GameState::Ptr state) {
        LOG("Available moves:\n");
        LOG("\n");
        for (int i=0; i < m.size(); ++i) {
            LOG("%2d: %s\n", i, reprForMove(m[i]).c_str());
        }
        LOG("\n");
    }

    size_t chooseMove() {
        TRACE();
        LOG("player %zu> ", id);
        string s;
        getline(cin, s);
        if (cin.eof()) {
            exit(0);
        }
        auto is_number = !s.empty() && s.find_first_not_of("0123456789") == std::string::npos;
        if (is_number) {
            return stoi(s);
        }
        return -1;
    }

    Move move(GameState::Ptr gs) override {
        TRACE();
        auto moves = gs->getCurrentMoveList();
        size_t index = 0;

        printMoves(moves, gs);

        while (true) {
            index = chooseMove();
            if (index < moves.size()) {
                break;
            }
        }

        auto mc = MoveCount(gs.get());
        return mc.move(index);
    }
};

// ---------------------------------------------------------------------------

struct NaivePlayer : Player {
    string name() const override { return "NaivePlayer"; }

    Affinity bestSide() {
        int clan = 0;
        int monk = 0;
        for (auto c : hand.characters) {
            const auto &card = CARD_TABLE[c];
            switch (card.affinity) {
            case CLAN: clan += card.value; break;
            case MONK: monk += card.value; break;
            case NONE: break;
            }
        }

        if      (clan > monk) { return CLAN; }
        else if (monk > clan) { return MONK; }

        return NONE;
    }

    Move selectMove(GameState::Ptr state) {
        auto moves = state->getCurrentMoveList();

        vector<int> values;
        values.reserve(moves.size());

        size_t sum = 0;
        for (const auto &m : moves) {
            auto v = 1+valueForMove(m, state->flags);
            sum += v;
            values.push_back(v);
        }

        auto choice = urand(sum);
        auto move = Move::NullMove();

        sum = 0;
        int i = 0;
        for (const auto &m : moves) {
            sum += values[i];
            ++i;
            if (sum >= choice) {
                move = m;
                break;
            }
        }
        return move;
    }

    Move move(GameState::Ptr state) override {
        TRACE();
        return selectMove(state);
    }
};

// ---------------------------------------------------------------------------

struct HeuristicPlayer : Player {
    string name() const override { return "HeuristicPlayer"; }

    Move move(GameState::Ptr state) override {
        TRACE();

        auto moves = state->getCurrentMoveList();

        int high = 0;
        auto best = Move::NullMove();
        for (const auto &m : moves) {
            auto value = 1+valueForMove(m, state->flags);
            if (value > high) {
                high = value;
                best = m;
            }
        }
        assert(!Move::IsNullMove(best));
        return best;
    }
};

// ---------------------------------------------------------------------------

struct MCPlayer : Player, enable_shared_from_this<MCPlayer> {
    string name() const override { return "MCPlayer"; }
    const size_t mcLen;
    const size_t concurrency;

    struct MoveStat {
        int scores;
        int visits;
    };

    using MoveStats = map<size_t, MoveStat>;
    mutable mutex mtx;

    MCPlayer(size_t l)
    : mcLen(l)
    , concurrency(thread::hardware_concurrency())
    {
    }

    void searchOne(MoveCount &mc, MoveStats &stats) const
    {
        // Clone with all random players.
        auto g2 = mc.state->clone<RandomPlayer>();
        g2->randomizeHiddenState(id);

        // Choose a move and perform it.
        auto index = urand(mc.count);
        auto m = mc.move(index);
        g2->doMove(id, m);

        g2->round.checkState(id, m);

        // Play remainder of challenges
        g2->finishGame();

        updateStats(stats, g2, index);
    }


    void updateStats(MoveStats &stats, GameState::Ptr g2, size_t index) const {
        // Find winner, ties considered losses
        int best = id;
        auto p = g2->players[id];
        int high = p->score;
        for (int j=0; j < g2->players.size(); ++j) {
            if (j != id) {
                if (g2->players[j]->score >= high) {
                    best = j;
                    high = g2->players[j]->score;
                }
            }
        }

        // Associate win rate with this move
        {
            lock_guard<mutex> lock(mtx);
            auto &s = stats[index];
            s.scores += p->score;
            ++s.visits;
        }
    }

    size_t findBest(MoveStats &stats) const {
        int highScore = 0;
        size_t bestScore = 0;

        size_t index;
        MoveStat s;
        for (auto p : stats) {
            tie(index, s) = p;
            
            int avgScore = s.scores/(float)s.visits;

            // Use >= so we skip over concede if it ties with something else
            if (avgScore >= highScore) {
                highScore = avgScore;
                bestScore = index;
            }
        }
        return bestScore;
    }

    Move move(GameState::Ptr gs) override
    {
        LogLock lock;

        auto mc = MoveCount(gs.get());
        if (mc.count == 0) {
            return Move::NullMove();
        }

        // If only one move, take that one without searching.
        if (mc.count == 1) {
            return mc.move(0);
        }
        MoveStats stats;

        size_t samples = mcLen*mc.count / concurrency;
        const auto self = shared_from_this();
        vector<thread> t(concurrency);
        for (size_t i=0; i < concurrency; ++i) {
            t[i] = thread([self,
                           mc,
                           &stats,
                           samples]
            {
                auto mcCopy = mc;
                for (int j=0; j < samples; ++j) {
                    self->searchOne(mcCopy, stats);
                }
            });
        }
        for (size_t i=0; i < concurrency; ++i) {
            t[i].join();
        }

        return mc.move(findBest(stats));
    }
};

// ---------------------------------------------------------------------------

struct Node : enable_shared_from_this<Node> {
    using Ptr = shared_ptr<Node>;
    using wPtr = weak_ptr<Node>;

    Move move;
    Cards cards;
    // The move that got us to this node. null for the root node.
    wPtr parent;
    vector<Ptr> children;
    float wins;
    size_t visits;
    size_t avails;
    int just_moved;

    Node(Move m, Cards c, Ptr _parent, int p)
    : move(m)
    , cards(c)
    , parent(_parent)
    , wins(0)
    , visits(0)
    , avails(1)
    , just_moved(p)
    {
    }

    void reset(Move m, Cards c, Ptr _parent, int p) {
        TRACE();
        move = m;
        cards = c;
        parent = _parent;
        wins = 0;
        visits = 0;
        avails = 1;
        just_moved = p;
    }

    Ptr self() { return this->shared_from_this(); }

    // Returns the elements of the legal_moves for which this node does not have children.
    MoveList getUntriedMoves(MoveList legal_moves) {
        TRACE();
        if (legal_moves.empty()) {
            return {};
        }

        // Find all moves for which this node *does* have children.
        MoveList tried;
        for (const auto &c : children) {
            if (!Move::IsNullMove(c->move)) {
                tried.push_back(c->move);
            }
        }

        MoveList untried;
        for (const auto m : legal_moves) {
            if (!Contains(tried, m)) {
                untried.push_back(m);
            }
        }
        return untried;
    }

    float ucb(float exploration) {
        constexpr float EPSILON = 1e-6;
        auto tiebreaker = RandFloat() * EPSILON;
        return (float) wins / (float) visits
            + exploration * sqrt(log(avails) / (float) visits)
            + tiebreaker;
    }

    // Use the UCB1 formula to select a child node, filtered by the given list
    // of legal moves.
    Ptr selectChildUCB(GameState::Ptr &state,
                       const MoveList &legal_moves,
                       float exploration) {
        TRACE();
        // Filter the list of children by the list of legal moves.
        vector<Ptr> legal_children;
        for (const auto &child : children) {
            if (Contains(legal_moves, child->move)) {
                legal_children.push_back(child);
            }
        }

        // Get the child with the highest UCB score
        auto s = std::max_element(begin(legal_children),
                                  end(legal_children),
                                  [exploration](const auto &a, const auto &b) {
                                    return a->ucb(exploration) < b->ucb(exploration);
                                  });

        // Update availability counts -- it is easier to do this now than during
        // backpropagation
        for (auto &child : legal_children) {
            child->avails += 1;
        }

        // Return the child selected above
        return *s;
    }

    // Add a new child for the move m, returning the added child node.
    Ptr addChild(Move m, Cards c, int p) {
        TRACE();
        auto n = make_shared<Node>(m, c, self(), p);
        children.push_back(n);
        return n;
    }

    // Update this node - increment the visit count by one and increase the
    // win count by the result of the terminalState for just_moved
    void update(typename GameState::Ptr terminal) {
        TRACE();
        visits += 1;
        if (just_moved != -1) {
            wins += terminal->getResult(just_moved);
        }
    }

    string smartMoveRepr() const {
        string r;
        switch (move.action) {
        case Move::PLAY_CHARACTER:
        case Move::PLAY_STYLE:
        case Move::PLAY_WEAPON:
        case Move::PLAY_WRENCH:
            if (cards.size() == 1) {
                r = CARD_TABLE[cards[0]].repr();
            } else {
                for (auto c : cards) {
                    r += CARD_TABLE[cards[c]].repr() + "+";
                }
                assert(!r.empty());
                r.pop_back();
            }
            break;
        case Move::KNOCKOUT_CHARACTER:
        case Move::KNOCKOUT_STYLE:
        case Move::KNOCKOUT_WEAPON:
        case Move::CAPTURE_WEAPON:
        case Move::BUDDY_CHAR:
        case Move::THIEF_STEAL:
        case Move::DISCARD_ONE:
        case Move::TRADE_HAND:
        case Move::DRAW_CARD:
        case Move::CONCEDE:
        case Move::PASS:
        case Move::NULL_ACTION:
            r = move.repr();
            break;
        }
        return r;
    }

    string repr() const {

        constexpr size_t BUFSZ = 80;
        char buf[BUFSZ];
        snprintf(buf, BUFSZ, "[w/v/a: %4zu/%4zu/%4zu p=%d m=%s]",
                 (size_t)wins, visits, avails,
                 just_moved,
                 smartMoveRepr().c_str());
        return buf;
    }

    string shortRepr() const {
        constexpr size_t BUFSZ = 80;
        char buf[BUFSZ];
        snprintf(buf, BUFSZ, "%zu/%zu/%zu", (size_t)wins, visits, avails);
        return buf;
    }

    void printChildren() const {
#ifdef LOGGING
        for (const auto &n : children) {
            LOG(" - %s\n", n->repr().c_str());
        }
#endif
    }

    void printTree(size_t indent=0) const {
        LOG("%s\n", (indentStr(indent) + repr()).c_str());

        for (const auto &c : children) {
            c->printTree(indent+1);
        }
    }

    string indentStr(size_t indent) const {
        TRACE();
        auto s = string();
        for (int i=0; i < indent; ++i) {
            s += "| ";
        }
        return s;
    }

    void sort() {
        TRACE();
        Sort(children, [](const auto &a, const auto &b) {
            return a->visits > b->visits;
        });

        for (auto &child : children) {
            child->sort();
        }
    }
};

// ---------------------------------------------------------------------------

// Randomization policy for ISMCTSPlayer
enum class MCTSRand {
    NEVER,  // use the same hidden state for every tree
    ONCE,   // use one hidden state per tree
    ALWAYS  // use a different state per simulation
};

static string PolicyStr(MCTSRand p) {
    switch (p) {
    case MCTSRand::NEVER:  return "0";
    case MCTSRand::ONCE:   return "1";
    case MCTSRand::ALWAYS: return "*";
    }
};

struct ISMCTSPlayer : Player {
    size_t itermax;
    size_t num_trees;
    float exploration;
    size_t move_count;
    MCTSRand policy;

    // suggest imax=1,000..10,000, n=8..10, c=0.7
    ISMCTSPlayer(size_t imax, size_t n, float c, MCTSRand p)
    : itermax(imax)
    , num_trees(n)
    , exploration(c)
    , move_count(0)
    , policy(p)
    {}

    string name() const override {
        return "ISMCTS:" + to_string(num_trees) + "/"
                         + to_string(itermax)   + "/"
                         + PolicyStr(policy)    + "/"
                         + to_string(exploration);
    }

    // Actions to take before performing a move to make sure we're in the right state.
    void preMove(const GameState::Ptr &state) {
        TRACE();
        if (state->round.challenge_finished) {
            state->beginChallenge();
        } else if (state->round.round_finished) {
            state->round.reset();
        }
        assert(!state->game_over);
    }

    // Actions to take after performing a move to make sure we're in the right state.
    void postMove(const GameState::Ptr &state) {
        TRACE();
        if (state->round.challenge_finished) {
            state->endChallenge();
            if (!state->gameOver()) {
                state->beginChallenge();
            }
        } else if (state->round.round_finished) {
            if (!state->game_over) {
                state->round.reset();
            }
        }
    }

    void makeMove(Move move, const GameState::Ptr &state) {
        TRACE();
        preMove(state);

        auto p = state->currentPlayer()->id;
        state->doMove(p, move);
        state->round.checkState(p, move);

        postMove(state);
    }

    void log(Node::Ptr root, GameState::Ptr state) {
#ifdef LOGGING
        //LOG("exploration tree:\n");
        //root->printTree();
        //auto p = state->currentPlayer();
        //LOG("player %zu children:\n", p->id);
        //root->printChildren();
#endif
        writeDot(root);
    }

    void loop(Node::Ptr root, GameState::Ptr root_state) {
        TRACE();

        auto initial = root_state->clone<NaivePlayer>();
        initial->randomizeHiddenState(id);

        for (int i=0; i < itermax; ++i) {
            initial = iterate(root, initial, i);
        }
    }

    Move move(GameState::Ptr root_state) override {
        TRACE();
        assert(!root_state->game_over);
        return parallelSearch(root_state);
        //return singleSearch(root_state);
    }

    vector<pair<Move,size_t>> iterateAndMerge(GameState::Ptr root_state) {
        TRACE();
        LogLock lock;

        vector<Node::Ptr> root_nodes(num_trees);
        vector<thread> t(num_trees);

        {
            for (size_t i=0; i < num_trees; ++i) {
                root_nodes[i] = make_shared<Node>(Move::NullMove(), Cards{0}, nullptr, -1);
                t[i] = thread([this, root_state, root=root_nodes[i]] {
                    this->loop(root, root_state);
                });
            }
        }

        vector<pair<Move,size_t>> merge;
        for (size_t i=0; i < num_trees; ++i) {
            t[i].join();
        }
        for (size_t i=0; i < num_trees; ++i) {
            for (const auto &n : root_nodes[i]->children) {
                bool found = false;
                for (int j=0; j < merge.size(); ++j) {
                    if (merge[j].first == n->move) {
                        merge[j].second += 1+n->wins;
                        found = true;
                        break;
                    }
                }
                if (!found) {
                    merge.push_back({n->move, 1+n->wins});
                }
            }
        }
        return merge;
    }

    Move parallelSearch(GameState::Ptr root_state) {
        auto merge = iterateAndMerge(root_state);

        // This can happen at the last move; not entirely sure why.
        if (merge.empty()) {
            return {Move::PASS, 0};
        }

        auto best = Move {Move::NULL_ACTION, 0};
        int high = 0;
        LOG("player %zu children:\n", id);
        Sort(merge, [](const auto &a, const auto &b) {
            return a.second > b.second;
        });
        for (const auto &e : merge) {
            LOG(" - %lu %s\n", e.second, reprForMove(e.first).c_str());
            if (e.second > high) {
                high = e.second;
                best = e.first;
            }
        }

        //LOG("best: %lu %s\n", high, CARD_TABLE[Player::cardForMove(best)].repr().c_str());
        assert(!Move::IsNullMove(best));
        return best;
    }

    Move singleSearch(GameState::Ptr root_state) {
        TRACE();
        auto root = make_shared<Node>(Move::NullMove(), Cards{0}, nullptr, -1);

        loop(root, root_state);

        log(root, root_state);

        // This can happen at the last move; not entirely sure why.
        if (root->children.empty()) {
            return {Move::PASS, 0};
        }

        // Helper for writeDot
        ++move_count;

        // Find the most visited node.
        auto best = std::max_element(begin(root->children),
                                     end(root->children),
                                     [](const auto &a, const auto &b) {
                                         return a->visits < b->visits;
                                     });

        return (*best)->move;
    }

    // Randomize the hidden state.
    pair<GameState::Ptr,GameState::Ptr> determinize(GameState::Ptr rootstate, size_t i) {
        TRACE();
        GameState::Ptr initial = rootstate;
        GameState::Ptr state = rootstate->clone<NaivePlayer>();
        if (policy == MCTSRand::ALWAYS || (policy == MCTSRand::ONCE && i == 0)) {
            state->randomizeHiddenState(id);
            initial = state->clone<NaivePlayer>();
        }
        return {initial, state};
    }

    pair<GameState::Ptr, Node::Ptr> select(GameState::Ptr state,
                                           Node::Ptr node)
    {
        TRACE();
        auto moves = state->getCurrentMoveList();
        auto untried = node->getUntriedMoves(moves);

        while (!moves.empty())
        {
            if (!untried.empty()) {
                return expand(state, node, untried);
            }
            node = node->selectChildUCB(state, moves, exploration);
            makeMove(node->move, state);

            moves = state->getCurrentMoveList();
            untried = node->getUntriedMoves(moves);
        }
        return {state, node};
    }

    // Create a new node to explore.
    pair<GameState::Ptr, Node::Ptr> expand(GameState::Ptr state,
                                           Node::Ptr node,
                                           MoveList &untried)
    {
        TRACE();
        if (!untried.empty()) {
            auto m = untried[urand(untried.size())];
            auto p = state->currentPlayer();
            auto c = cardsForMove(m, p);
            makeMove(m, state);
            node = node->addChild(m, c, p->id);
        }
        return {state, node};
    }

    GameState::Ptr iterate(Node::Ptr root, GameState::Ptr initial, int i) {
        TRACE();
        auto node = root;

        // Determinize
        GameState::Ptr state;
        tie(initial,state) = determinize(initial, i);

        // Find next node
        tie(state, node) = select(state, node);

        // Simulate
        state->finishGame();

        // Backpropagate
        while (node) {
            node->update(state);
            node = node->parent.lock();
        }
        return initial;
    }

    Cards cardsForMove(const Move &m, Player *p) const {
        TRACE();
        Cards c {0};
        switch (m.action) {
        case Move::PLAY_STYLE:
            c = p->cardsForStyle(m.index);
            break;
        case Move::PLAY_CHARACTER:
        case Move::PLAY_WEAPON:
        case Move::PLAY_WRENCH:
            c[0] = p->cardForMove(m);
            break;
        default:
            break;
        }
        return c;
    }

    void writeNodeDefinitions(FILE *fp, Node::Ptr node, int &n, int indent=1) {
        int m = n;
        fprintf(fp, "%*cn%d [\n", indent*2, ' ', m);
        fprintf(fp, "%*c  label = \"%s | %s\"\n",
                indent*2, ' ', 
                node->shortRepr().c_str(),
                EscapeQuotes(node->smartMoveRepr()).c_str());
        fprintf(fp, "%*c  shape = record\n", indent*2, ' ');
        fprintf(fp, "%*c  style = filled\n", indent*2, ' ');
        string color;
        switch (node->just_moved) {
        case 0: color = "fc1e33"; break;
        case 1: color = "ff911e"; break;
        case 2: color = "21abd8"; break;
        case 3: color = "40ee1c"; break;
        }
        if (!color.empty()) {
            fprintf(fp, "%*c  fillcolor = \"#%s\"\n", indent*2, ' ', color.c_str());
        }
        fprintf(fp, "%*c];\n", indent*2, ' ');

        for (auto &child : node->children) {
            ++n;
            writeNodeDefinitions(fp, child, n, indent+1);
        }
    }

    void writeNodeConnections(FILE *fp, Node::Ptr node, int &n) {
        int m = n;
        for (auto &child : node->children) {
            ++n;
            fprintf(fp, "  n%d -> n%d;\n", m, n);
            writeNodeConnections(fp, child, n);
        }
    }

    void writeDot(Node::Ptr root) {
        root->sort();

        string fname("ismcts/ismcts.");
        fname += to_string(move_count);
        FILE *fp = fopen(fname.c_str(), "w");
        assert(fp);

        fprintf(fp, "digraph G {\n");
        fprintf(fp, "  rankdir = LR;\n");

        int n = 0;
        writeNodeDefinitions(fp, root, n);
        n = 0;
        writeNodeConnections(fp, root, n);
        fprintf(fp, "}\n");
        fclose(fp);
    }
};

// ---------------------------------------------------------------------------

vector<Player::Ptr> BuildPlayers() {
    TRACE();
    vector<Player::Ptr> players;
    //players.emplace_back(make_shared<RandomPlayer>());
#if 0
    //players.emplace_back(make_shared<ISMCTSPlayer>(500, 10, 10, MCTSRand::NEVER));
    //players.emplace_back(make_shared<ISMCTSPlayer>(500, 10, 10, MCTSRand::ONCE));
    //players.emplace_back(make_shared<RandomPlayer>());
    //players.emplace_back(make_shared<NaivePlayer>());
    players.emplace_back(make_shared<ISMCTSPlayer>(MCTS_LEN*2, 10, EXPLORATION, MCTSRand::ALWAYS));
    players.emplace_back(make_shared<ISMCTSPlayer>(MCTS_LEN, 10, 10, MCTSRand::NEVER));
    players.emplace_back(make_shared<ISMCTSPlayer>(MCTS_LEN, 10, EXPLORATION, MCTSRand::ONCE));
    players.emplace_back(make_shared<MCPlayer>(MC_LEN));
#else
    players.emplace_back(make_shared<RandomPlayer>());
    players.emplace_back(make_shared<RandomPlayer>());
    players.emplace_back(make_shared<RandomPlayer>());
    players.emplace_back(make_shared<RandomPlayer>());
#endif
    //players.emplace_back(make_shared<MCPlayer>(MC_LEN));
    //players.emplace_back(make_shared<NaivePlayer>());

    //players.emplace_back(make_shared<HumanPlayer>());
    return players;
}

// ---------------------------------------------------------------------------

struct ScoreLog {
    FILE *fp;

    ScoreLog() {
        fp = fopen("score.log", "w");
        assert(fp);
    }

    ~ScoreLog() { fclose(fp); }

    void update(vector<Player::Ptr> players) {
        for (auto &p : players) {
            fprintf(fp, "%d,", p->score);
        }
        fprintf(fp, "%d\n", players.back()->score);
        fflush(fp);
    }
};

// ---------------------------------------------------------------------------

void Play(int games) {
    TRACE();
    auto players = BuildPlayers();
#ifndef LOGGING
    vector<int> scores(players.size(), 0);
#endif
    printf("Players:\n");
    for (int i=0; i < players.size(); ++i) {
        printf("- %d: %s\n", i, players[i]->name().c_str());
    }

    auto m = make_shared<MonkeyChallenge<RandomPlayer>>(players);

    ScoreLog slog;
    for (int i=0; i < games ; ++i) {
        m->play();
        m->loop();
        slog.update(m->state->players);
#ifndef LOGGING
        for (int p=0; p < players.size(); ++p) {
            scores[p] += m->state->players[p]->score;
        }
#if !defined(USE_MCPLAYER) && !defined(USE_ISMCTSPLAYER)
        if ((i % 5000) == 0)
#else
#endif
        {
            printf("[%d:%02.2f%%] ", i+1, 100*(float)i/(float)games);
            for (int p=0; p < players.size(); ++p) {
                printf("%d: %03.2f ",
                        p,
                        ((float)scores[p])/((float)(i+1)));
            }
            printf("\r");
            fflush(stdout);
        }
#endif
    }
#ifndef LOGGING
    printf("\n");
    for (int i=0; i < players.size(); ++i) {
        for (auto a : { NONE, CLAN, MONK }) {
            auto count = m->state->players[i]->stats.detail[a];
            printf("%d[%s]: %10d/%10d -> %0.2f\n",
                   i,
                   AffinityStr(a).c_str(),
                   count.wins,
                   count.losses,
                   (float)count.wins/(float)(count.wins+count.losses));
        }
    }
#endif
}

// ---------------------------------------------------------------------------

int main() {
    //chdir("/Users/tmarsh/Dropbox (Personal)/monkey");
    LoadCards();

#ifdef LOGGING
    const float samples = 1;
#else
    const float samples = NSAMPLES;
    auto start = chrono::steady_clock::now();
#endif

    Play(samples);

#ifndef LOGGING
    auto end = chrono::steady_clock::now();
    auto diff = end - start;
    auto count = chrono::duration<double, milli>(diff).count();
    printf("execution time: %.2f ms\n", count);
    printf("    per sample: %.4f ms\n", count/samples);
    printf(" samples / sec: %d\n", (int)(samples/(count/1000)));
#endif
}
