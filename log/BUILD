load("@rules_cc//cc:defs.bzl", "cc_binary")

cc_test(
  name = "test_log",
  srcs = ["circulQueueTest.cpp", "circulQueue.h"],
  deps = [
    "@com_google_googletest//:gtest_main",
  ],
)


cc_library(
    name = "logLib",
    srcs = ["circulQueue.h", "log.h"],
    hdrs = glob(["**/*.h",
                "**/*.hpp"]),
    deps = ["//public_def:public_def"],
    linkopts = ["-std=c++11", "-g"],
    visibility = ["//visibility:public"],
)