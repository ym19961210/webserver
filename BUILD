cc_library(
    name = "headersOffer",
    srcs = ["test.h", "test.cpp"],
    hdrs = ["test.h"],
    visibility = ["//visibility:public"],                                                                                                                                                                                   
)

cc_binary(
    name = "main",
    srcs = ["main.cpp", "test.h", "test.cpp"],
    deps = ["//epoll:epollConnect", "//httpParser:httpParser", "//:headersOffer"],
    visibility = ["//visibility:public"],
    linkopts = ["-g"],                                                                                                                                                                                     
)