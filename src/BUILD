cc_binary(
  name = "monkey",
  deps = ["//third_party:json11", "//third_party:spdlog", "//third_party:sfml"],
  copts = [
    "-std=c++14",
    "-Ofast",
    "-DNDEBUG",
    "-DSFML_NO_DEPRECATED_WARNINGS",
    "-Ithird_party",
    "-Ithird_party/spdlog/include",
    "-Ithird_party/sfml/include"
  ],
  linkopts = [
#   "-ltcmalloc",
    "-Lthird_party/sfml/lib",
    "-lsfml-window",
    "-lsfml-graphics",
    "-lsfml-system",
  ],
  srcs = [
    "agent.h",
    "bits.h",
    "cards.h",
    "challenge.h",
    "core.h",
    "debug.h",
    "deck.h",
    "dot.h",
    "flatmc.h",
    "game.h",
    "hand.h",
    "human.h",
    "log.h",
    "mcts.h",
    "mcts_node.h",
    "move.h",
    "moves.h",
    "naive.h",
    "player.h",
    "rand.h",
    "round.h",
    "state.h",
    "ui.h",
    "util.h",
    "view.h",
    "visible.h",
    "cards.cc",
    "init.cc",
    "main.cc",
  ]
)
