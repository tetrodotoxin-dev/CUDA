# # Tetrodotoxin
# Copyright (c) 2023-present Matt Kaes and contributors

load("@rules_cc//cc:cc_import.bzl", "cc_import")
load("@rules_cc//cc:cc_library.bzl", "cc_library")
load("@rules_cc//cc:cc_shared_library.bzl", "cc_shared_library")
load("@tetro_toolchain//:library.bzl", "LINUX", "WINDOWS", "exports")

package(default_visibility = ["//visibility:public"])

cc_library(
    name = "module",
    srcs = ["module.cpp"],
    deps = ["//cuda/runtime"],
)

exports(
    name = "exports",
    library = ":module",
    visibility = ["//visibility:private"],
)

cc_shared_library(
    name = "plugin",
    exports_filter = ["//cuda:__subpackages__"],
    shared_lib_name = select({
        LINUX: "libttx_cuda.so",
        WINDOWS: "ttx_cuda.dll",
    }),
    tags = ["__DONT_DEPEND_ON_DEF_PARSER__"],
    user_link_flags = select({
        LINUX: [
            "-Wl,-z,defs",
            "-Wl,-soname,libttx_cuda.so",
            "-Wl,-rpath,$ORIGIN",
        ],
        WINDOWS: [],
    }),
    win_def_file = select({
        WINDOWS: ":exports",
        "//conditions:default": None,
    }),
    deps = [":module"],
)

filegroup(
    name = "interface",
    srcs = [":plugin"],
    output_group = "interface_library",
)

cc_import(
    name = "ttx_cuda",
    interface_library = select({
        WINDOWS: ":interface",
        "//conditions:default": None,
    }),
    shared_library = ":plugin",
    # Keep the compiler's runtime libraries in the consumer's deployment graph.
    deps = [
        "//build:compute",
        "//cuda/contracts:headers",
    ],
)

filegroup(
    name = "sources",
    srcs = [
        "BUILD",
        "module.cpp",
    ],
)
