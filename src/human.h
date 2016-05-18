#include "agent.h"

#include <iostream>

struct HumanAgent {
    std::string name() const { return "Human"; }

    void announce(const State &s, const Moves &m) const {
        BASE_LOG(info, "Hand:");
        BASE_LOG(info, "    CHARACTERS");
        for (const auto &c : s.current().hand.characters) {
            BASE_LOG(info, "    {}", to_string(Card::Get(c)).c_str());
        }
        BASE_LOG(info, "    SKILLS");
        for (const auto &c : s.current().hand.skills) {
            BASE_LOG(info, "    {}", to_string(Card::Get(c)).c_str());
        }
        BASE_LOG(info, "Available moves:");
        BASE_LOG(info, "");
        for (int i=0; i < m.moves.size(); ++i) {
            BASE_LOG(info, "{:2d}: {}", i, to_string(m.moves[i]).c_str());
        }
        BASE_LOG(info, "");
    }

    size_t chooseMove(size_t id) const {
        TRACE();
        BASE_LOG(info, "player {}> ", id);
        std::string s;
        std::getline(std::cin, s);
        if (std::cin.eof()) {
            exit(0);
        }
        auto is_number = !s.empty() && s.find_first_not_of("0123456789") == std::string::npos;
        if (is_number) {
            return std::stoi(s);
        }
        return -1;
    }

    void move(State &s) const {
        TRACE();
        const Moves m(s);
        size_t index = 0;

        announce(s, m);

        while (true) {
            index = chooseMove(s.current().id);
            if (index < m.moves.size()) {
                break;
            }
        }

        s.perform(m.moves[index]);
    }
};
