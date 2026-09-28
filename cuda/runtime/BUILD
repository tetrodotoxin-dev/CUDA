# # Tetrodotoxin
# Copyright (c) 2023-present Matt Kaes and contributors

load("@rules_cc//cc:cc_library.bzl", "cc_library")

package(default_visibility = ["//visibility:public"])

cc_library(
    name = "runtime",
    srcs = glob(["*.cpp"]),
    hdrs = glob(["*.hpp"]),
    target_compatible_with = [
        "@platforms//os:linux",
        "@platforms//cpu:x86_64",
    ],
    deps = [
        "//cuda/contracts",
        "@cuda_sdk//:compute",
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
