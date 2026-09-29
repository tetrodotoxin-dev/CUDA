// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "perimortem/memory/dynamic/bytes.hpp"
#include "perimortem/memory/dynamic/vector.hpp"

#include "cuda/contracts/kernel.hpp"
#include "cuda/runtime/program.hpp"

namespace Cuda::Runtime {

// Kernel retains the resolved entry and copied frame description. Preparing
// argument offsets once avoids driver name lookup and allocations at launch.
// Its argument pointer scratch belongs to this synchronous, serialized owner.
class Kernel {
 public:
  static auto prepare(
      Program& program,
      perimortem_view_bytes entry,
      const ttx_representation& frame,
      const cuda_argument* arguments,
      Count count,
      cuda_diagnostics diagnostics,
      ttx_borrowed* output) -> ttx_data_status;
  auto get_query() const -> ttx_semantic_query;
  auto get_data() const -> Perimortem::Core::View::Bytes {
    return Perimortem::Core::View::Bytes();
  }
  auto supports(Perimortem::System::Uuid id) const
      -> Ttx::Semantic::Negotiation::Binding::Status;
  auto bind_interface(
      Perimortem::System::Uuid id,
      Ttx::Data::Form::Storage target) const
      -> Ttx::Semantic::Negotiation::Binding::Status;
  auto borrow() const -> Perimortem::Utility::Result<
      Ttx::Concept::Policies::Borrowed,
      Ttx::Semantic::Negotiation::Binding::Failure>;
  auto release() const -> void;

 private:
  Kernel(
      Program& program,
      CUfunction function,
      const ttx_representation& frame,
      Perimortem::Memory::Dynamic::Vector<Count> offsets);
  ~Kernel();
  // Invocations share the prepared argument pointers. The caller serializes
  // access while each launch keeps the Program's context current through
  // completion.
  auto launch(cuda_launch geometry, ttx_storage arguments) const
      -> ttx_data_status;
  Program& program;
  CUfunction function;
  Perimortem::Memory::Dynamic::Bytes form;
  Perimortem::Memory::Dynamic::Vector<Count> offsets;
  mutable Perimortem::Memory::Dynamic::Vector<void*> arguments;
};
}  // namespace Cuda::Runtime
