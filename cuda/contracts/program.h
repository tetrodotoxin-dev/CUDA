// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#ifndef TTX_CUDA_CONTRACTS_PROGRAM_H
#define TTX_CUDA_CONTRACTS_PROGRAM_H

#include "cuda/contracts/diagnostics.h"
#include "ttx/concept/policies/borrowed.h"
#include "ttx/data/form/representation.h"

#define TTX_CUDA_PROGRAM_HIGH ((U64)0xc1bcf233d4104c0bULL)
#define TTX_CUDA_PROGRAM_LOW ((U64)0x84f0529809175602ULL)

// Arguments retain their formal boundaries even when their storage could be
// compacted into one Data range. The author supplies each parameter's complete
// form and its placement in the host frame. CUDA verifies parameter sizes
// against the loaded function. The author still owns agreement between source
// types and these descriptions, just as a native publisher owns its Schema.
// Device addresses are eight byte CUDA values. They do not grant host access.
typedef struct cuda_argument {
  Count offset;
  const ttx_representation* representation;
} cuda_argument;

// Program keeps its code and CUDA context available. Kernel preparation copies
// the parameter descriptions and resolves the executable entry once. Both
// prepare and allocate return Borrowed answers with their own release
// obligations, retaining the state needed after this Program is released.
// Operations run serially on their owning worker because a kernel reuses its
// invocation scratch.
typedef struct cuda_program {
  const void* source;
  const struct cuda_program_ops* operations;
} cuda_program;

typedef struct cuda_program_ops {
  ttx_abstract_ops abstract;
  ttx_data_status (*prepare)(
      const void* source,
      perimortem_view_bytes entry,
      const ttx_representation* frame,
      const cuda_argument* arguments,
      Count count,
      cuda_diagnostics diagnostics,
      ttx_borrowed* output);
  ttx_data_status (
      *allocate)(const void* source, Count size, ttx_borrowed* output);
} cuda_program_ops;

// C consumers can request the same prepared API form as the C++ facade.
PERIMORTEM_C const ttx_representation* cuda_program_representation(void);

#endif
