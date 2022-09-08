load("@rules_cc//cc:defs.bzl", "cc_binary")

cc_test(
  name = "timerTest",
  srcs = ["timerGtest.cpp", "timerClass.h", "timerClass.cpp"],
  deps = [
    "@com_google_googletest//:gtest_main",
  ],
)

cc_library(
    name = "timer",
    srcs = ["timerClass.h", "timerClass.cpp"],
    hdrs = glob(["**/*.h",
                "**/*.hpp"]),
    linkopts = ["-std=c++11"],
    visibility = ["//visibility:public"],
)