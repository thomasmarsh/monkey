#include "log.h"
#include "moves.h"
#include "cards.h"
#include "util.h"

void Play() {
    State s(4);
    size_t c = 0;
    while (!s.game_over) {
        LOG("");
        LOG("CHALLENGE #{}", c);
        LOG("");
        size_t r = 0;
        while (!s.challenge.finished()) {
            LOG("ROUND <{}>", r);
            while (!s.challenge.round.finished()) {
                Moves m(s);
                assert(!m.moves.empty());
                const auto &p = s.current();
                for (const auto &move : m.moves) {
                    LOG("    - {}", to_string(move, p));
                }
                auto &move = m.moves[urand(m.moves.size())];
                s.perform(move);
            }
            s.challenge.round.reset();
            s.validateCards();
            ++r;
        }
        s.reset();
        ++c;
    }
}

int main() {
    chdir("/Users/tmarsh/Dropbox (Personal)/monkey");
    auto console = spd::stdout_logger_mt("console", true);
    spd::set_pattern("%H:%M:%S.%e%v");

    Initialize();
    Play();
}
