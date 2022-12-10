load("@rules_cc//cc:defs.bzl", "cc_binary")

cc_library(
    name = "httpParser",
    srcs = ["httpResponse.cpp", 
            "httpResponse.h", 
            "httpParser.cpp", 
            "httpParser.h"],
    hdrs = glob(["**/*.h",
                "**/*.hpp"]),
    deps = ["//public_def:public_def", "//log:logLib"],
    linkopts = ["-std=c++11"],
    visibility = ["//visibility:public"],
)

cc_test(
  name = "test_httpParser",
  srcs = ["httpParserTest.cpp"],
  deps = [
    ":httpParser",
    "@com_google_googletest//:gtest_main",
  ],
)