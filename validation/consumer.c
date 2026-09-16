// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include <dlfcn.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "cuda/contracts/compiler.h"
#include "cuda/contracts/program.h"
#include "cuda/contracts/buffer.h"
#include "cuda/contracts/kernel.h"
#include "ttx/concept/modules/module.h"

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

int main(int argc, char** argv) {
  require(argc == 2, "Supply the independent CUDA module.");
  void* module = dlopen(argv[1], RTLD_NOW | RTLD_LOCAL);
  require(module != NULL, "Could not load CUDA module.");
  ttx_data_status (*open_module)(ttx_semantic_query, ttx_module_acquisition*);
  void* symbol = dlsym(module, "ttx_module_open");
  memcpy(&open_module, &symbol, sizeof(open_module));
  require(symbol != NULL, "Module has no TTX entry.");
  ttx_module_acquisition root;
  require(open_module((ttx_semantic_query){0}, &root) == TTX_DATA_SUCCESS, "Module did not open.");
  const perimortem_uuid compiler_id = {TTX_CUDA_COMPILER_HIGH, TTX_CUDA_COMPILER_LOW};
  require(root.root.operations->supports(root.root.source, compiler_id) == TTX_BINDING_SATISFIED, "Compiler support failed.");
  cuda_compiler compiler;
  require(root.root.operations->bind(root.root.source, compiler_id,
      (ttx_storage){cuda_compiler_representation(), (U8*)&compiler, sizeof(compiler)}) == TTX_BINDING_SATISFIED, "C compiler binding failed.");
  root.release(root.owner);

  // C uses the same artifact and full API representations, independently of
  // any engine and every native C++ owner class. The compiler outlives discovery.
  static const char source[] = "extern \"C\" __global__ void empty() {}";
  static const char name[] = "consumer.cu";
  const cuda_compile_request request = {{{(const U8*)name, sizeof(name) - 1}, {(const U8*)source, sizeof(source) - 1}}, NULL, 0, NULL, 0, 0};
  ttx_publication publication;
  require(compiler.compile(compiler.source, request, (cuda_diagnostics){NULL, diagnostic}, &publication) == TTX_DATA_SUCCESS, "C source compilation failed.");
  cuda_program program;
  require(publication.query.bind(publication.query.source,
      (perimortem_uuid){TTX_CUDA_PROGRAM_HIGH, TTX_CUDA_PROGRAM_LOW},
      (ttx_storage){cuda_program_representation(), (U8*)&program, sizeof(program)}) == TTX_BINDING_SATISFIED, "C Program binding failed.");
  ttx_publication allocation;
  require(program.allocate(program.source, 4, &allocation) == TTX_DATA_SUCCESS, "C allocation failed.");
  cuda_buffer buffer;
  require(allocation.query.bind(allocation.query.source,
      (perimortem_uuid){TTX_CUDA_BUFFER_HIGH, TTX_CUDA_BUFFER_LOW},
      (ttx_storage){cuda_buffer_representation(), (U8*)&buffer, sizeof(buffer)}) == TTX_BINDING_SATISFIED, "C Buffer binding failed.");
  publication.release(publication.query.source);
  const U8 expected[] = {1, 2, 3, 4};
  U8 observed[4];
  require(buffer.write(buffer.source, 0, (perimortem_view_bytes){expected, sizeof(expected)}) == TTX_DATA_SUCCESS, "C upload failed.");
  require(buffer.read(buffer.source, 0, observed, sizeof(observed)) == TTX_DATA_SUCCESS, "C readback failed.");
  require(memcmp(expected, observed, sizeof(expected)) == 0, "C readback differs.");
  allocation.release(allocation.query.source);
  dlclose(module);
  puts("PASS C consumer: full API negotiation, source compilation and retained CUDA storage");
  return 0;
}
