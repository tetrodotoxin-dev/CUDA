// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#ifndef TTX_CUDA_CONTRACTS_COMPILER_H
#define TTX_CUDA_CONTRACTS_COMPILER_H

#include "cuda/contracts/diagnostics.h"
#include "ttx/semantic/ownership/publication.h"

#define TTX_CUDA_COMPILER_HIGH ((U64)0xc1bcf233d4104c0bULL)
#define TTX_CUDA_COMPILER_LOW ((U64)0x84f0529809175601ULL)

// The host owns resource paths. CUDA receives source bytes and named includes,
// allowing the same compiler to serve an application or a toolchain consumer.
// Inputs are borrowed only during compilation. Success transfers a Program
// publication. Failure leaves the output untouched and reports source errors
// through the caller's diagnostic sink.
typedef struct cuda_source {
  perimortem_view_bytes name;
  perimortem_view_bytes text;
} cuda_source;

typedef struct cuda_compile_request {
  cuda_source source;
  const cuda_source* headers;
  Count header_count;
  const perimortem_view_bytes* options;
  Count option_count;
  S32 device;
} cuda_compile_request;

typedef struct cuda_compiler {
  const void* source;
  ttx_data_status (*compile)(
      const void* source,
      cuda_compile_request request,
      cuda_diagnostics diagnostics,
      ttx_publication* output);
} cuda_compiler;

// C consumers can request the same prepared API form as the C++ facade.
PERIMORTEM_C const ttx_representation* cuda_compiler_representation(void);

#endif
