#include "log.h"
#include "moves.h"
#include "cards.h"
#include "util.h"

void Play() {
    State s(4);
    size_t c = 0;
    while (!s.gameOver()) {
        LOG("");
        LOG("CHALLENGE #{}", c);
        LOG("");
        size_t r = 0;
        while (!s.challenge.finished()) {
            LOG("ROUND <{}>", r);
            while (!s.challenge.round.finished()) {
                const auto &p = s.current();
                LOG("PLAYER {}", p.id);
                p.debug();
                Moves m(s);
                assert(!m.moves.empty());
                LOG("Moves:");
                for (const auto &move : m.moves) {
                    LOG("    - {}", to_string(move, p));
                }
                auto move = m.moves[urand(m.moves.size())];
                s.perform(move);
            }
            s.challenge.round.reset();
            if (s.gameOver()) {
                break;
            }
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

    SET_LOG_LEVEL(trace);
    Initialize();
    for (int i=0; i < 1; ++i) {
        Play();
    }
}
