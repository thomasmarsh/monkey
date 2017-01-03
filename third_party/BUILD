cc_library(
  name = "json11",
  hdrs = ["json11.hpp"],
  srcs = ["json11.cc"],
  visibility = ["//visibility:public"],
)

cc_library(
  name = "spdlog",
  hdrs = glob([
    "spdlog/include/spdlog/*.h",
    "spdlog/include/spdlog/details/*.h",
    "spdlog/include/spdlog/details/*.cc", # needed for format.cc
    "spdlog/include/spdlog/sinks/*.h",
  ]),
  visibility = ["//visibility:public"],
)

cc_library(
  name = "sfml",
  hdrs = glob([
    "sfml/include/SFML/*.hpp",
    "sfml/include/SFML/Audio/*.hpp",
    "sfml/include/SFML/Graphics/*.hpp",
    "sfml/include/SFML/Graphics/*.inl",
    "sfml/include/SFML/Network/*.hpp",
    "sfml/include/SFML/System/*.hpp",
    "sfml/include/SFML/System/*.inl",
    "sfml/include/SFML/Window/*.hpp",
  ]),
  srcs = glob(["sfml/lib/*.dylib"]),
  visibility = ["//visibility:public"],
)
