load("@rules_cc//cc:defs.bzl", "cc_binary")

cc_test(
  name = "test_threadPool",
  srcs = ["threadPoolTest.cpp", "threadpool.h"],
  deps = [
    "@com_google_googletest//:gtest_main",
    "//:headersOffer",
  ],
)

cc_library(
    name = "threadPoolLib",
    srcs = ["threadpool.h"],
    linkopts = ["-std=c++11", "-lpthread", "-g"],
    hdrs = glob(["**/*.h",
                "**/*.hpp"]),
    deps = ["//:headersOffer"],
    visibility = ["//visibility:public"],
)