# # Tetrodotoxin
# Copyright (c) 2023-present Matt Kaes and contributors

load("@rules_cc//cc:cc_import.bzl", "cc_import")
load("@rules_cc//cc:cc_library.bzl", "cc_library")
load("@rules_cc//cc:cc_shared_library.bzl", "cc_shared_library")

package(default_visibility = ["//visibility:public"])

cc_library(
    name = "module",
    srcs = ["module.cpp"],
    copts = ["-fvisibility=hidden"],
    deps = ["//cuda/runtime"],
)

cc_shared_library(
    name = "plugin",
    exports_filter = ["//cuda:__subpackages__"],
    shared_lib_name = "libttx_cuda.so",
    user_link_flags = [
        "-Wl,-z,defs",
        "-Wl,-soname,libttx_cuda.so",
        "-Wl,-rpath,$ORIGIN",
    ],
    deps = [":module"],
)

cc_import(
    name = "ttx_cuda",
    shared_library = ":plugin",
    deps = ["//cuda/contracts:headers"],
)

filegroup(
    name = "sources",
    srcs = [
        "BUILD",
        "module.cpp",
    ],
)
