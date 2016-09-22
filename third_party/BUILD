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
