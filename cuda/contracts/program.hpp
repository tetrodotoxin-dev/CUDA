// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "cuda/contracts/compiler.hpp"
#include "cuda/contracts/program.h"

TTX_DATA_RECORD(
    cuda_argument,
    TTX_DATA_MEMBER(cuda_argument, offset),
    TTX_DATA_MEMBER(cuda_argument, representation));
TTX_DATA_RECORD(
    cuda_program,
    TTX_DATA_MEMBER(cuda_program, source),
    TTX_DATA_MEMBER(cuda_program, prepare),
    TTX_DATA_MEMBER(cuda_program, allocate));

namespace Cuda::Contracts {

// Program prepares independently owned kernels and storage. Their publications
// preserve executable and context state after this borrowed view is discarded.
class Program {
 public:
  static constexpr Perimortem::System::Uuid contract_id{
    TTX_CUDA_PROGRAM_HIGH,
    TTX_CUDA_PROGRAM_LOW,
  };

  using Api = cuda_program;
  explicit constexpr Program(Api api) : api(api) {}
  auto prepare(
      Perimortem::Core::View::Bytes entry,
      const Ttx::Data::Form::Representation& frame,
      Perimortem::Core::View::Vector<cuda_argument> arguments,
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

    const auto status = api.prepare(
        api.source,
        {
          entry.get_data(),
          entry.get_size(),
        },
        &frame, arguments.get_data(), arguments.get_size(), sink, &output);
    if (status != TTX_DATA_SUCCESS) {
      return static_cast<Ttx::Data::Status>(status);
    }

    return Ttx::Semantic::Ownership::Publication(output);
  }

  auto allocate(Count size) const -> Perimortem::Utility::
      Result<Ttx::Semantic::Ownership::Publication, Ttx::Data::Status> {
    ttx_publication output = {};

    const auto status = api.allocate(api.source, size, &output);
    if (status != TTX_DATA_SUCCESS) {
      return static_cast<Ttx::Data::Status>(status);
    }

    return Ttx::Semantic::Ownership::Publication(output);
  }

 private:
  Api api;
};

}  // namespace Cuda::Contracts
