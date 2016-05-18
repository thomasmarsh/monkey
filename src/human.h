#include "agent.h"

#include <iostream>

struct HumanAgent {
    std::string name() const { return "Human"; }

    void announce(const State &s, const Moves &m) const {
        printf("Hand:\n");
        printf("    CHARACTERS\n");
        for (const auto &c : s.current().hand.characters) {
            printf("    %s\n", to_string(Card::Get(c)).c_str());
        }
        printf("    SKILLS\n");
        for (const auto &c : s.current().hand.skills) {
            printf("    %s\n", to_string(Card::Get(c)).c_str());
        }
        printf("Available moves:\n");
        printf("\n");
        for (int i=0; i < m.moves.size(); ++i) {
            printf("%2d: %s\n", i, to_string(m.moves[i]).c_str());
        }
        printf("\n");
    }

    size_t chooseMove(size_t id) const {
        TRACE();
        printf("player %zu> ", id);
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
