// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "cuda/contracts/buffer.hpp"
#include "cuda/runtime/program.hpp"

namespace Cuda::Runtime {

// Device allocation belongs to an independently released publication. The
// retained Program supplies context lifetime even after its original caller
// drops the compiled program. Image dimensions have no role in this owner.
class Buffer {
 public:
  static auto allocate(Program& program, Count size, ttx_publication* output)
      -> ttx_data_status;
  auto get_query() const -> ttx_semantic_query;

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
