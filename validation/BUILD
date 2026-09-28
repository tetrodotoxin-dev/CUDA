# # Tetrodotoxin
# Copyright (c) 2023-present Matt Kaes and contributors

load("@rules_cc//cc:cc_test.bzl", "cc_test")

cc_test(
    name = "runtime",
    srcs = ["runtime.cpp"],
    args = ["$(rootpath //cuda:plugin)"],
    data = ["//cuda:plugin"],
    tags = ["local"],
    deps = ["//cuda:ttx_cuda"],
)

cc_test(
    name = "consumer",
    srcs = ["consumer.c"],
    args = ["$(rootpath //cuda:plugin)"],
    copts = [
        "-std=c17",
        "-Wall",
        "-Wextra",
        "-Werror",
    ],
    data = ["//cuda:plugin"],
    tags = ["local"],
    deps = ["//cuda:ttx_cuda"],
)

filegroup(
    name = "sources",
    srcs = glob([
        "BUILD",
        "*.c",
        "*.cpp",
    ]),
    visibility = ["//visibility:public"],
)
