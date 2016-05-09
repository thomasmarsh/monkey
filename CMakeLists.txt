cmake_minimum_required(VERSION 3.5.0)
project(monkey)

set(CMAKE_BUILD_TYPE RELEASE)
set(CMAKE_CXX_FLAGS "-pipe -Wall -std=c++14 -march=native -fcolor-diagnostics")
set(CMAKE_CXX_FLAGS_DEBUG "-O0 -g")

# Passing -DNDEBUG only gets about a 4% increase
set(CMAKE_CXX_FLAGS_RELEASE "-O3")

include_directories(
    /usr/local/include
    src/support
    src/support/spdlog/include
    src)

link_directories(/usr/local/lib)

add_executable(
    monkey
    src/support/json11.cc
    src/cards.cc
    src/init.cc
    src/main.cc
    src/util.cc)

add_executable(
    mtest
    src/cards.cc
    src/init.cc
    src/util.cc
    src/support/json11.cc
    tests/test_agent.cc
    tests/test_challenge.cc
    tests/test_main.cc
    tests/test_state.cc
    tests/test_bits.cc
    tests/test_deck.cc
    tests/test_round.cc)

target_link_libraries(monkey tcmalloc)
target_link_libraries(mtest tcmalloc)
