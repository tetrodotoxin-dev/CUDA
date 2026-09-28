// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#ifndef TTX_CUDA_CONTRACTS_KERNEL_H
#define TTX_CUDA_CONTRACTS_KERNEL_H

#include "ttx/data/form/storage.h"

#define TTX_CUDA_KERNEL_HIGH ((U64)0xc1bcf233d4104c0bULL)
#define TTX_CUDA_KERNEL_LOW ((U64)0x84f0529809175603ULL)

// Launch geometry belongs to the caller's CUDA policy rather than an image
// convention. The prepared argument frame is borrowed until launch returns.
// Completion includes device execution, keeping asynchronous scheduling outside
// this contract. A failed launch may have modified device buffers, so
// publishing a new application result remains the enclosing operation's
// responsibility.
typedef struct cuda_launch {
  U32 grid_x, grid_y, grid_z;
  U32 block_x, block_y, block_z;
  U32 shared_bytes;
} cuda_launch;

typedef struct cuda_kernel {
  const void* source;
  ttx_data_status (
      *launch)(const void* source, cuda_launch geometry, ttx_storage arguments);
} cuda_kernel;

// C consumers can request the same prepared API form as the C++ facade.
PERIMORTEM_C const ttx_representation* cuda_kernel_representation(void);

#endif
