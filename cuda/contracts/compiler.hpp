// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "perimortem/memory/dynamic/bytes.hpp"

#include "cuda/contracts/compiler.h"
#include "ttx/concept/abstract.hpp"
#include "ttx/semantic/ownership/publication.hpp"

TTX_DATA_RECORD(
    cuda_diagnostics,
    TTX_DATA_MEMBER(cuda_diagnostics, source),
    TTX_DATA_MEMBER(cuda_diagnostics, write));
TTX_DATA_RECORD(
    cuda_source,
    TTX_DATA_MEMBER(cuda_source, name),
    TTX_DATA_MEMBER(cuda_source, text));
TTX_DATA_RECORD(
    cuda_compile_request,
    TTX_DATA_MEMBER(cuda_compile_request, source),
    TTX_DATA_MEMBER(cuda_compile_request, headers),
    TTX_DATA_MEMBER(cuda_compile_request, header_count),
    TTX_DATA_MEMBER(cuda_compile_request, options),
    TTX_DATA_MEMBER(cuda_compile_request, option_count),
    TTX_DATA_MEMBER(cuda_compile_request, device));
TTX_DATA_RECORD(
    cuda_compiler,
    TTX_DATA_MEMBER(cuda_compiler, source),
    TTX_DATA_MEMBER(cuda_compiler, compile));

namespace Cuda::Contracts {

// Compiler lends an executable service. The resulting publication owns the
// program state, while the caller keeps this provider's module loaded through
// all publications and borrowed operations originating from it.
class Compiler {
 public:
  static constexpr Perimortem::System::Uuid contract_id{
    TTX_CUDA_COMPILER_HIGH,
    TTX_CUDA_COMPILER_LOW,
  };

  using Api = cuda_compiler;
  explicit constexpr Compiler(Api api) : api(api) {}
  auto compile(
      cuda_compile_request request,
      Perimortem::Memory::Dynamic::Bytes& errors) const -> Perimortem::Utility::
      Result<Ttx::Semantic::Ownership::Publication, Ttx::Data::Status> {
    ttx_publication output = {};

    const cuda_diagnostics sink = {
      &errors,
      [](void* source, perimortem_view_bytes text) {
        *static_cast<Perimortem::Memory::Dynamic::Bytes*>(source) =
            Perimortem::Memory::Dynamic::Bytes({
              text.data,
              text.size,
            });
      },
    };
    errors.clear();

    const auto status = api.compile(api.source, request, sink, &output);
    if (status != TTX_DATA_SUCCESS) {
      return static_cast<Ttx::Data::Status>(status);
    }

    return Ttx::Semantic::Ownership::Publication(output);
  }

 private:
  Api api;
};

}  // namespace Cuda::Contracts
