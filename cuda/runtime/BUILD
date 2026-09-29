# # Tetrodotoxin
# Copyright (c) 2023-present Matt Kaes and contributors

load("@rules_cc//cc:cc_library.bzl", "cc_library")

package(default_visibility = ["//visibility:public"])

cc_library(
    name = "runtime",
    srcs = glob(["*.cpp"]),
    hdrs = glob(["*.hpp"]),
    deps = [
        "//build:compute",
        "//cuda/contracts",
    ],
)

filegroup(
    name = "public_headers",
    srcs = glob(["*.hpp"]),
)

filegroup(
    name = "sources",
    srcs = glob([
        "BUILD",
        "*.cpp",
        "*.hpp",
    ]),
)
