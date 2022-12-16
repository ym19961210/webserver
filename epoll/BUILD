load("@rules_cc//cc:defs.bzl", "cc_binary")

cc_library(
    name = "epollConnect",
    srcs = ["epollConnect.cpp","epollConnect.h"],
    deps = ["//public_def:public_def", "//httpParser:httpParser", "//threadpool:threadPoolLib", "//timer:timer", "//log:logLib", "//:headersOffer"],
    linkopts = ["-std=c++11", "-lpthread", "-g"],
    hdrs = glob(["**/*.h",
                "**/*.hpp"]),
    visibility = ["//visibility:public"],
)