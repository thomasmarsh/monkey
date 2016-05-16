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

    const auto start = std::chrono::steady_clock::now();

    //GameUI::New()->play();
    const size_t samples = 10000;
    for (int i=0; i < samples; ++i) {
        Game().play();
    }

    const auto end = std::chrono::steady_clock::now();
    const auto diff = end - start;
    const auto count = std::chrono::duration<double, std::milli>(diff).count();
    printf("execution time: %.2f ms\n", count);
    printf("    per sample: %.4f ms\n", count/samples);
    printf(" samples / sec: %d\n", (int)(samples/(count/1000)));
}
