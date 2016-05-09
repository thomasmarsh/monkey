#include "agent.h"
#include "log.h"

#include "support/catch.hpp"

TEST_CASE("perform an agent move", "[agent]") {
    State s(4);
    auto a = RandomAgent();
    for (int i=0; i < 4; ++i) {
        s.current().debug();
        a.move(s);
    }
}
