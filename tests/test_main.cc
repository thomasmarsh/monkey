#include "log.h"
#include "cards.h"
#include "util.h"

#define CATCH_CONFIG_RUNNER
#include "support/catch.hpp"

int main(int argc, char* const argv[]) {
    auto console = spd::stdout_logger_mt("console", true);
    spd::set_pattern("%H:%M:%S.%e%v");
    SET_LOG_LEVEL(warn);

    int result = -1;
    try {

        Initialize();

        result = Catch::Session().run(argc, argv);

    } catch (const spd::spdlog_ex& ex) {
        std::cout << "Log failed: " << ex.what() << std::endl;
    }
    return result;
}
