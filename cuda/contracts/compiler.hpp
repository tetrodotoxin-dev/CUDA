// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "perimortem/memory/dynamic/bytes.hpp"

#include "cuda/contracts/compiler.h"
#include "ttx/concept/abstract.hpp"
#include "ttx/concept/policies/borrowed.hpp"

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
    cuda_compiler_ops,
    TTX_DATA_MEMBER(cuda_compiler_ops, abstract),
    TTX_DATA_MEMBER(cuda_compiler_ops, compile));
TTX_DATA_RECORD(
    cuda_compiler,
    TTX_DATA_MEMBER(cuda_compiler, source),
    TTX_DATA_MEMBER(cuda_compiler, operations));

namespace Cuda::Contracts {

// Compiler builds a Program from source bytes and named includes. The returned
// Borrowed answer keeps that program available until release. Diagnostics are
// copied into the caller's buffer while compilation runs.
class Compiler : public Ttx::Concept::Abstract {
 public:
  static constexpr auto contract_id =
      Perimortem::System::Uuid(TTX_CUDA_COMPILER_HIGH, TTX_CUDA_COMPILER_LOW);

  using Api = cuda_compiler;
  using Operations = cuda_compiler_ops;
  explicit constexpr Compiler(Api api)
      : Abstract(api.source, api.operations->abstract) {}
  static auto accept(Api value) -> Bool {
    return value.operations &&
           Abstract::accept(
               ttx_abstract(value.source, &value.operations->abstract)) &&
           value.operations->compile;
  }
  auto get_abi() const -> Api {
    const auto value = Abstract::get_abi();
    return Api(
        value.source, reinterpret_cast<const Operations*>(value.operations));
  }
  auto compile(
      cuda_compile_request request,
      Perimortem::Memory::Dynamic::Bytes& errors) const -> Perimortem::Utility::
      Result<Ttx::Concept::Policies::Borrowed, Ttx::Data::Status> {
    ttx_borrowed output = ttx_borrowed();
    const cuda_diagnostics sink =
        cuda_diagnostics(&errors, [](void* source, perimortem_view_bytes text) {
          *static_cast<Perimortem::Memory::Dynamic::Bytes*>(source) =
              Perimortem::Memory::Dynamic::Bytes(
                  Perimortem::Core::View::Bytes(text.data, text.size));
        });
    errors.clear();

    const auto status =
        get_abi().operations->compile(get_abi().source, request, sink, &output);
    if (status == TTX_DATA_SUCCESS) {
      if (Ttx::Concept::Policies::Borrowed::accept(output)) {
        return Ttx::Concept::Policies::Borrowed(output);
      }
      if (output.source && output.operations && output.operations->release) {
        output.operations->release(output.source);
      }
      return Ttx::Data::Status::Invalid;
    }
    return static_cast<Ttx::Data::Status>(status);
  }
};

}  // namespace Cuda::Contracts
