// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "cuda/contracts/compiler.hpp"
#include "cuda/contracts/program.h"
#include "ttx/concept/policies/borrowed.hpp"

TTX_DATA_RECORD(
    cuda_argument,
    TTX_DATA_MEMBER(cuda_argument, offset),
    TTX_DATA_MEMBER(cuda_argument, representation));
TTX_DATA_RECORD(
    cuda_program_ops,
    TTX_DATA_MEMBER(cuda_program_ops, abstract),
    TTX_DATA_MEMBER(cuda_program_ops, prepare),
    TTX_DATA_MEMBER(cuda_program_ops, allocate));
TTX_DATA_RECORD(
    cuda_program,
    TTX_DATA_MEMBER(cuda_program, source),
    TTX_DATA_MEMBER(cuda_program, operations));

namespace Cuda::Contracts {

// Program produces kernels and device storage as Borrowed answers. Each keeps
// the state it needs after the producing Program is released. The caller binds
// the corresponding Kernel or Buffer interface and releases the acquired
// answer after its final use.
class Program : public Ttx::Concept::Abstract {
 public:
  static constexpr auto contract_id =
      Perimortem::System::Uuid(TTX_CUDA_PROGRAM_HIGH, TTX_CUDA_PROGRAM_LOW);

  using Api = cuda_program;
  using Operations = cuda_program_ops;
  explicit constexpr Program(Api api)
      : Abstract(api.source, api.operations->abstract) {}
  static auto accept(Api value) -> Bool {
    return value.operations &&
           Abstract::accept(
               ttx_abstract(value.source, &value.operations->abstract)) &&
           value.operations->prepare && value.operations->allocate;
  }
  auto get_abi() const -> Api {
    const auto value = Abstract::get_abi();
    return Api(
        value.source, reinterpret_cast<const Operations*>(value.operations));
  }
  auto prepare(
      Perimortem::Core::View::Bytes entry,
      const Ttx::Data::Form::Representation& frame,
      Perimortem::Core::View::Vector<cuda_argument> arguments,
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

    const auto status = get_abi().operations->prepare(
        get_abi().source,
        {
          entry.get_data(),
          entry.get_size(),
        },
        &frame, arguments.get_data(), arguments.get_size(), sink, &output);
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

  auto allocate(Count size) const -> Perimortem::Utility::
      Result<Ttx::Concept::Policies::Borrowed, Ttx::Data::Status> {
    ttx_borrowed output = ttx_borrowed();
    const auto status =
        get_abi().operations->allocate(get_abi().source, size, &output);
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
