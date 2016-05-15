#include "cards.h"
#include "log.h"

#include "support/json11.hpp"

#include <fstream>

using namespace json11;

using CardTable = std::vector<Card>;
std::map<Action,std::string> CARD_ACTION_DESC;
CardTable CARD_TABLE_PROTO;
CardTable CARD_TABLE;

const Card& Card::Get(CardRef card) {
    return CARD_TABLE[card];
}

std::string ActionDescription(Action a) {
    auto it = CARD_ACTION_DESC.find(a);
    assert(it != CARD_ACTION_DESC.end());
    return it->second;
}

static void ExpandCardTable() {
    assert(CARD_TABLE.empty());
    CARD_TABLE.reserve(NUM_CARDS);
    CardRef id = 0;
    CardRef prototype = 0;
    for (auto &e : CARD_TABLE_PROTO) {
        for (CardRef i=0; i < e.quantity; ++i) {
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
        ERROR("CARD_TABLE.size() = {} (expected {})", CARD_TABLE.size(), NUM_CARDS);
    }
}

static Affinity ParseAffinity(const std::string &s) {
    if      (s == "NONE")  { return Affinity::NONE; }
    else if (s == "MONK")  { return Affinity::MONK; }
    else if (s == "CLAN")  { return Affinity::CLAN; }
    ERROR("unexpected value: {}", s);
}

static CardType ParseCardType(const std::string &s) {
    if      (s == "CHARACTER") { return CardType::CHARACTER; }
    else if (s == "STYLE")     { return CardType::STYLE; }
    else if (s == "WEAPON")    { return CardType::WEAPON; }
    else if (s == "WRENCH")    { return CardType::WRENCH; }
    else if (s == "EVENT")     { return CardType::EVENT; }
    ERROR("unexpected value: {}", s);
}

static ArgType ParseArgType(const std::string &s) {
    if      (s == "NONE")                 { return ArgType::NONE; }
    else if (s == "VISIBLE")              { return ArgType::VISIBLE; }
    else if (s == "RECV_STYLE")           { return ArgType::RECV_STYLE; }
    else if (s == "RECV_WEAPON")          { return ArgType::RECV_WEAPON; }
    else if (s == "EXPOSED_CHAR")         { return ArgType::EXPOSED_CHAR; }
    else if (s == "EXPOSED_STYLE")        { return ArgType::EXPOSED_STYLE; }
    else if (s == "EXPOSED_WEAPON")       { return ArgType::EXPOSED_WEAPON; }
    else if (s == "VISIBLE_CHAR_OR_HOLD") { return ArgType::VISIBLE_CHAR_OR_HOLD; }
    else if (s == "OPPONENT")             { return ArgType::OPPONENT; }
    else if (s == "OPPONENT_HAND")        { return ArgType::OPPONENT_HAND; }
    else if (s == "HAND")                 { return ArgType::HAND; }
    else if (s == "DRAW_PILE")            { return ArgType::DRAW_PILE; }
    ERROR("unexpected value: {}", s);
}

static Special ParseSpecial(const std::string &s) {
    if      (s == "NONE")                { return Special::NONE; }
    else if (s == "IMMUNE")              { return Special::IMMUNE; }
    else if (s == "DOUBLE_STYLES")       { return Special::DOUBLE_STYLES; }
    else if (s == "STYLE_PLUS_ONE")      { return Special::STYLE_PLUS_ONE; }
    else if (s == "TWO_WEAPONS")         { return Special::TWO_WEAPONS; }
    else if (s == "DOUBLE_WEAPON_VALUE") { return Special::DOUBLE_WEAPON_VALUE; }
    else if (s == "IGNORE_AFFINITY")     { return Special::IGNORE_AFFINITY; }
    else if (s == "NO_STYLES")           { return Special::NO_STYLES; }
    else if (s == "NO_WEAPONS")          { return Special::NO_WEAPONS; }
    ERROR("unexpected value: {}", s);
}

static Action ParseAction(const std::string &s) {
	if      (s == "NONE")               { return Action::NONE; }
	else if (s == "PASS")               { return Action::PASS; }
	else if (s == "CONCEDE")            { return Action::CONCEDE; }
	else if (s == "CLEAR_FIELD")        { return Action::CLEAR_FIELD; }
	else if (s == "STEAL_CARD")         { return Action::STEAL_CARD; }
	else if (s == "TRADE_HAND")         { return Action::TRADE_HAND; }
	else if (s == "DISCARD_ONE")        { return Action::DISCARD_ONE; }
	else if (s == "DRAW_CARD")          { return Action::DRAW_CARD; }
	else if (s == "PLAY_WEAPON")        { return Action::PLAY_WEAPON; }
	else if (s == "PLAY_STYLE")         { return Action::PLAY_STYLE; }
	else if (s == "DISARM_CHARACTER")   { return Action::DISARM_CHARACTER; }
	else if (s == "KNOCKOUT_CHARACTER") { return Action::KNOCKOUT_CHARACTER; }
	else if (s == "KNOCKOUT_STYLE")     { return Action::KNOCKOUT_STYLE; }
	else if (s == "KNOCKOUT_WEAPON")    { return Action::KNOCKOUT_WEAPON; }
	else if (s == "PLAY_WEAPON_RETAIN") { return Action::PLAY_WEAPON_RETAIN; }
	else if (s == "CAPTURE_WEAPON")     { return Action::CAPTURE_WEAPON; }
	else if (s == "PLAY_CHARACTER")     { return Action::PLAY_CHARACTER; }
    else if (s == "E_DRAW_TWO_SKILLS")  { return Action::E_DRAW_TWO_SKILLS; }
    else if (s == "E_NO_STYLES")        { return Action::E_NO_STYLES; }
    else if (s == "E_DRAW_ONE_CHAR")    { return Action::E_DRAW_ONE_CHAR; }
    else if (s == "E_NO_WEAPONS")       { return Action::E_NO_WEAPONS; }
    else if (s == "E_RANDOM_STEAL")     { return Action::E_RANDOM_STEAL; }
    else if (s == "E_DISCARD_TWO")      { return Action::E_DISCARD_TWO; }
    else if (s == "E_CHAR_BONUS")       { return Action::E_CHAR_BONUS; }
    else if (s == "E_INVERT_VALUE")     { return Action::E_INVERT_VALUE; }
    ERROR("unexpected value: {}", s);
}

static std::string LoadFile(const std::string &fname) {
    std::ifstream t(fname);
    std::string str;

    if (!t.is_open()) {
        return str;
    }

    t.seekg(0, std::ios::end);   
    str.reserve(t.tellg());
    t.seekg(0, std::ios::beg);

    str.assign(std::istreambuf_iterator<char>(t),
               std::istreambuf_iterator<char>());
    return str;
}

static std::string LoadCardsJson() {
    auto jsonStr = LoadFile("resources/cards.json");
    if (jsonStr.empty()) {
        jsonStr = LoadFile("../resources/cards.json");
    }
    assert(!jsonStr.empty());
    return jsonStr;
}

static void ValidateCardJson(const Json &json) {
    assert(json.is_object());
    std::string error;
    bool has_shape = json.has_shape({
        { "quantity",    Json::NUMBER },
        { "type",        Json::STRING },
        { "affinity",    Json::STRING },
        { "special",     Json::STRING },
        { "action",      Json::STRING },
        { "arg_type",    Json::STRING },
        //{ "isUber",      Json::BOOL },
        { "face_value",  Json::NUMBER },
        //{ "inverted_value",  Json::NUMBER },
        { "label",       Json::STRING },
        { "description", Json::STRING },
    }, error);

    if (!error.empty()) {
        WARN("has_shape error: {}", error);
    }
    assert(has_shape);
}

static void SetInverseValue(Card &c) {
    if (c.type == CHARACTER) {
        if (c.face_value != 6) {
            switch (c.affinity) {
            case Affinity::CLAN:
                c.inverted_value = 5-c.face_value;
                return;
            case Affinity::MONK:
                c.inverted_value = 7-c.face_value;
                return;
            default:
                break;
            }
        }
    }
    c.inverted_value = c.face_value;
}

static void ParseCard(const Json &json) {
    ValidateCardJson(json);

    Card proto;
    proto.quantity   = json["quantity"].int_value();
    proto.type       = ParseCardType(json["type"].string_value());
    proto.affinity   = ParseAffinity(json["affinity"].string_value());
    proto.special    = ParseSpecial(json["special"].string_value());
    proto.action     = ParseAction(json["action"].string_value());
    proto.arg_type   = ParseArgType(json["arg_type"].string_value());
    //proto.isUber     = json["isUber"].bool_value();
    proto.face_value = json["face_value"].int_value();
    proto.label      = json["label"].string_value();

    SetInverseValue(proto);

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
    std::string error;

    auto json = Json::parse(LoadCardsJson(), error);
    if (!error.empty()) {
        ERROR("JSON parse error: {}", error);
    }

    ParseJson(json);
}

void LoadCards() {
    LoadCardTableProto();
    ExpandCardTable();
    assert(CARD_TABLE.size() == NUM_CARDS);
}

