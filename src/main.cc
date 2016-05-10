#include "log.h"
#include "moves.h"
#include "cards.h"
#include "util.h"
#include "naive.h"
#include "flatmc.h"

void Play() {
    State s(4);
    s.init();
    size_t c = 0;
    auto ragent = RandomAgent();
    auto naive = NaiveAgent();
    auto flatmc = std::make_shared<MCAgent>();
    while (!s.gameOver()) {
        LOG("");
        LOG("CHALLENGE #{}", c);
        LOG("");
        size_t r = 0;
        while (!s.challenge.finished()) {
            LOG("ROUND <{}>", r);
            while (!s.challenge.round.finished()) {
                //const auto &p = s.current();
                //DLOG("PLAYER {}", p.id);
                //p.debug();
                Moves m(s);
                //assert(!m.moves.empty());
#ifndef NO_LOGGING
                //LOG("Moves:");
                //for (const auto &move : m.moves) {
                //    LOG("    - {}", to_string(move, p));
               // }
#endif
                switch (s.current().id) {
                case 0: ragent.move(s); break;
                case 1: naive.move(s); break;
                case 2: flatmc->move(s); break;
                case 3: ragent.move(s); break;
                }
                //auto &move = m.moves[urand(m.moves.size())];
                //s.perform(&move);
#ifndef NO_LOGGING
                for (const auto &p : s.players) {
                    LOG("Player {}", p.id);
                    p.visible.print();
                }
#endif
            }
            s.challenge.round.reset();
            if (s.gameOver()) {
                break;
            }
            ++r;
        }
        s.printScore();
        s.reset();
        ++c;
    }
}

int main() {
    chdir("/Users/tmarsh/Dropbox (Personal)/monkey");
    auto console = spd::stdout_logger_mt("console", true);
    //auto console = spd::rotating_logger_mt("console", "build/log", 1048576 * 5, 3);
    spd::set_pattern("%H:%M:%S.%e%v");

    SET_LOG_LEVEL(trace);
    Initialize();
    //for (int i=0; i < 10000; ++i) {
        Play();
    //}
}
