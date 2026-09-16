# # Tetrodotoxin
# Copyright (c) 2023-present Matt Kaes and contributors

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
    dynamic_deps = ["@tetrodotoxin//ttx:runtime"],
    exports_filter = ["//cuda:__subpackages__"],
    shared_lib_name = "libttx_cuda.so",
    user_link_flags = [
        "-Wl,-z,defs",
        "-Wl,-rpath,$ORIGIN",
    ],
    deps = [":module"],
)
