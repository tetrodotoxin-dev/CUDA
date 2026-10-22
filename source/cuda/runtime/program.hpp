// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include <cuda.h>

#include "perimortem/core/object.hpp"

#include "cuda/contracts/compiler.hpp"
#include "cuda/contracts/program.hpp"

namespace Cuda::Runtime {

// A compiled program owns one device context reference and its loaded code.
// Kernels and buffers retain this owner, keeping replacement independent of
// work already published. All operations use the caller's current worker.
class Program {
 public:
  static auto compile(
      cuda_compile_request request,
      cuda_diagnostics diagnostics,
      ttx_borrowed* output) -> ttx_data_status;
  static auto create(cuda_compile_request request, cuda_diagnostics diagnostics)
      -> Perimortem::Utility::Result<Program&, Ttx::Data::Status>;
  static auto compiler() -> Ttx::Semantic::Negotiation::Query;
  auto get_context() const -> CUcontext { return context; }
  auto get_module() const -> CUmodule { return module; }
  auto retain() -> void;
  auto release() const -> void;
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

 private:
  Program() = default;
  ~Program();
  CUdevice device = 0;
  CUcontext context = nullptr;
  CUmodule module = nullptr;
};
}  // namespace Cuda::Runtime
