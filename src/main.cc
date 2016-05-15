#include "ui.h"

#include <unistd.h>

std::vector<UICard> UICard::cards;

int main() {
    chdir("/Users/tmarsh/Dropbox (Personal)/monkey");
    auto console = spd::stdout_logger_mt("console", true);
    //auto console = spd::rotating_logger_mt("console", "build/log", 1048576 * 500, 20);
    size_t q_size = 4096; //queue size must be power of 2
    spdlog::set_async_mode(q_size);
    spd::set_pattern("%H:%M:%S.%e%v");

    SET_LOG_LEVEL(trace);
    Initialize();

    GameUI::New()->play();
    //for (int i=0; i < 10000; ++i) {
    //    Game().play();
   // }
}
