# # Tetrodotoxin
# Copyright (c) 2023-present Matt Kaes and contributors

"""CUDA SDK archives for the selected Linux or Windows target."""

load("@rules_pkg//pkg:mappings.bzl", "pkg_attributes", "pkg_files", "strip_prefix")
load("@rules_pkg//pkg:zip.bzl", "pkg_zip")
load("@tetro_toolchain//:library.bzl", "LINUX", "WINDOWS")

def sdk_release(name, headers, library, sources, scripts):
    """Package public headers, the shared library and rebuildable source."""
    archive = native.module_name() + "-" + native.module_version()
    platform = select({LINUX: "linux-x86_64-v3", WINDOWS: "windows-x86_64-msvc"})
    pkg_files(
        name = name + "_headers",
        srcs = headers,
        prefix = "include",
        strip_prefix = strip_prefix.from_root(),
    )
    pkg_files(
        name = name + "_library",
        srcs = [library],
        prefix = "lib",
    )
    pkg_files(
        name = name + "_interface",
        srcs = select({WINDOWS: ["//cuda:interface"], "//conditions:default": []}),
        prefix = "lib",
        renames = select({WINDOWS: {"//cuda:interface": "ttx_cuda.lib"}, "//conditions:default": {}}),
    )

    # The Windows driver loader is linked into the provider. Carry its notice
    # with that binary while NVIDIA's shared runtimes remain separate inputs.
    pkg_files(
        name = name + "_driver_license",
        srcs = select({WINDOWS: ["@cuda_cudart_windows//:LICENSE"], "//conditions:default": []}),
        renames = select({WINDOWS: {"@cuda_cudart_windows//:LICENSE": "NVIDIA-CUDA-LICENSE"}, "//conditions:default": {}}),
    )
    pkg_files(
        name = name + "_sources",
        srcs = sources,
        strip_prefix = strip_prefix.from_root(),
    )
    pkg_files(
        name = name + "_scripts",
        srcs = scripts,
        attributes = pkg_attributes(mode = "0755"),
        strip_prefix = strip_prefix.from_root(),
    )
    for suffix, contents in {
        "headers": [":" + name + "_headers", "LICENSE"],
        "binary": [":" + name + "_library", ":" + name + "_interface", ":" + name + "_driver_license", "LICENSE"],
        "source": [":" + name + "_sources", ":" + name + "_scripts"],
    }.items():
        pkg_zip(
            name = name + "_" + suffix + "_archive",
            srcs = contents,
            out = archive + "-" + suffix + ".zip",
            package_file_name = archive + "-" + platform + ".zip" if suffix == "binary" else "",
            package_dir = "/",
            mode = "0644",
        )
    native.filegroup(
        name = name,
        srcs = [":" + name + "_" + suffix + "_archive" for suffix in ["headers", "binary", "source"]],
    )
