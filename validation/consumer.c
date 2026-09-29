// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#ifdef _WIN32
#include <windows.h>
#else
#include <dlfcn.h>
#endif
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "cuda/contracts/buffer.h"
#include "cuda/contracts/compiler.h"
#include "cuda/contracts/kernel.h"
#include "cuda/contracts/program.h"
#include "ttx/concept/policies/borrowed.h"
#include "ttx/semantic/negotiation/library.h"

static void require(int condition, const char* message) {
  if (!condition) {
    fputs(message, stderr);
    abort();
  }
}

static void diagnostic(void* unused, perimortem_view_bytes text) {
  (void)unused;
  fwrite(text.data, 1, text.size, stderr);
}

static ttx_binding_status compiler_view(
    void* source,
    ttx_semantic_query query) {
  return query.bind(
      query.source,
      (perimortem_uuid){TTX_CUDA_COMPILER_HIGH, TTX_CUDA_COMPILER_LOW},
      (ttx_storage){
        cuda_compiler_representation(), source, sizeof(cuda_compiler)});
}

int main(int argc, char** argv) {
  require(argc == 2, "Supply the independent CUDA module.");
#ifdef _WIN32
  HMODULE module = LoadLibraryA(argv[1]);
#else
  void* module = dlopen(argv[1], RTLD_NOW | RTLD_LOCAL);
#endif
  require(module != NULL, "Could not load CUDA module.");
  ttx_library_entry open_module;
#ifdef _WIN32
  FARPROC symbol = GetProcAddress(module, TTX_LIBRARY_ENTRY);
#else
  void* symbol = dlsym(module, TTX_LIBRARY_ENTRY);
#endif
  memcpy(&open_module, &symbol, sizeof(open_module));
  require(symbol != NULL, "Library has no Query entry.");
  cuda_compiler compiler;
  require(
      open_module(
          (ttx_semantic_query){0},
          (ttx_query_receiver){&compiler, compiler_view}) ==
          TTX_BINDING_SATISFIED,
      "C compiler binding failed.");

  // The C consumer uses public records and negotiated operations from the
  // loaded module. Its compiler remains usable after discovery is released.
  static const char source[] = "extern \"C\" __global__ void empty() {}";
  static const char name[] = "consumer.cu";
  const cuda_compile_request request = {
    {
      {
        (const U8*)name,
        sizeof(name) - 1,
      },
      {
        (const U8*)source,
        sizeof(source) - 1,
      },
    },
    NULL,
    0,
    NULL,
    0,
    0,
  };
  ttx_borrowed publication;
  require(
      compiler.operations->compile(
          compiler.source, request,
          (cuda_diagnostics){
            NULL,
            diagnostic,
          },
          &publication) == TTX_DATA_SUCCESS,
      "C source compilation failed.");
  cuda_program program;
  require(
      publication.operations->abstract.bind(
          publication.source,
          (perimortem_uuid){
            TTX_CUDA_PROGRAM_HIGH,
            TTX_CUDA_PROGRAM_LOW,
          },
          (ttx_storage){
            cuda_program_representation(),
            (U8*)&program,
            sizeof(program),
          }) == TTX_BINDING_SATISFIED,
      "C Program binding failed.");
  ttx_borrowed allocation;
  require(
      program.operations->allocate(program.source, 4, &allocation) ==
          TTX_DATA_SUCCESS,
      "C allocation failed.");
  cuda_buffer buffer;
  require(
      allocation.operations->abstract.bind(
          allocation.source,
          (perimortem_uuid){
            TTX_CUDA_BUFFER_HIGH,
            TTX_CUDA_BUFFER_LOW,
          },
          (ttx_storage){
            cuda_buffer_representation(),
            (U8*)&buffer,
            sizeof(buffer),
          }) == TTX_BINDING_SATISFIED,
      "C Buffer binding failed.");
  publication.operations->release(publication.source);
  const U8 expected[] = {
    1,
    2,
    3,
    4,
  };
  U8 observed[4];
  require(
      buffer.operations->write(
          buffer.source, 0,
          (perimortem_view_bytes){
            expected,
            sizeof(expected),
          }) == TTX_DATA_SUCCESS,
      "C upload failed.");
  require(
      buffer.operations->read(buffer.source, 0, observed, sizeof(observed)) ==
          TTX_DATA_SUCCESS,
      "C readback failed.");
  require(
      memcmp(expected, observed, sizeof(expected)) == 0, "C readback differs.");
  allocation.operations->release(allocation.source);
#ifdef _WIN32
  FreeLibrary(module);
#else
  dlclose(module);
#endif
  puts(
      "PASS C consumer: full API negotiation, source compilation and retained "
      "CUDA storage");
  return 0;
}
