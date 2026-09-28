# # Tetrodotoxin
# Copyright (c) 2023-present Matt Kaes and contributors

load("@rules_cc//cc:cc_library.bzl", "cc_library")

package(default_visibility = ["//visibility:public"])

cc_library(
    name = "headers",
    hdrs = glob([
        "*.h",
        "*.hpp",
    ]),
    deps = ["@ttx"],
)

cc_library(
    name = "contracts",
    srcs = glob(["*.cpp"]),
    deps = [":headers"],
    alwayslink = True,
)

filegroup(
    name = "public_headers",
    srcs = glob([
        "*.h",
        "*.hpp",
    ]),
)

filegroup(
    name = "sources",
    srcs = glob([
        "BUILD",
        "*.cpp",
        "*.h",
        "*.hpp",
    ]),
)
