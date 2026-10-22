// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#ifndef TTX_CUDA_CONTRACTS_DIAGNOSTICS_H
#define TTX_CUDA_CONTRACTS_DIAGNOSTICS_H

#include "perimortem/core/view/bytes.h"

// Compilation can fail before there is a program to own a diagnostic. The
// requester copies each synchronous message through this borrowed sink, so a
// failed compiler allocation never leaves a dangling diagnostic view.
typedef struct cuda_diagnostics {
  void* source;
  void (*write)(void* source, perimortem_view_bytes text);
} cuda_diagnostics;
#endif
