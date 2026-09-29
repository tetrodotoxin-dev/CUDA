// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "cuda/contracts/buffer.hpp"
#include "cuda/runtime/program.hpp"

namespace Cuda::Runtime {

// Device allocation belongs to an independently released publication. The
// retained Program supplies context lifetime even after its original caller
// drops the compiled program. The caller gives those bytes their application
// meaning.
class Buffer {
 public:
  static auto allocate(Program& program, Count size, ttx_borrowed* output)
      -> ttx_data_status;
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
  Buffer(Program& program, CUdeviceptr address, Count size)
      : program(program), address(address), size(size) {
    program.retain();
  }

  ~Buffer();
  Program& program;
  CUdeviceptr address;
  Count size;
};
}  // namespace Cuda::Runtime
