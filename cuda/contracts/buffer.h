// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#ifndef TTX_CUDA_CONTRACTS_BUFFER_H
#define TTX_CUDA_CONTRACTS_BUFFER_H

#include "perimortem/core/view/bytes.h"
#include "ttx/data/status.h"
#include "ttx/data/form/representation.h"

#define TTX_CUDA_BUFFER_HIGH ((U64)0xc1bcf233d4104c0bULL)
#define TTX_CUDA_BUFFER_LOW ((U64)0x84f0529809175604ULL)

// A device buffer owns CUDA storage independently of any image interpretation.
// Its address is meaningful only to kernels in the supplying CUDA context.
// The caller retains the publication through every launch that uses it. Read
// and write finish before returning and borrow host bytes only for that call.
typedef struct cuda_buffer {
  const void* source;
  U64 (*address)(const void* source);
  Count (*size)(const void* source);
  ttx_data_status (*read)(const void* source, Count offset, U8* output, Count size);
  ttx_data_status (*write)(const void* source, Count offset, perimortem_view_bytes input);
} cuda_buffer;

// C consumers can request the same prepared API form as the C++ facade.
PERIMORTEM_C const ttx_representation* cuda_buffer_representation(void);

#endif
