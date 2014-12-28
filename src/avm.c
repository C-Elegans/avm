#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <assert.h>
#include "avm.h"
#include "avm_util.h"

struct Context_s {
  void* *memory;
  void* *stack;
  /**
   * each `call` execution pushes the given address to this
   * stack, and each call to `ret` remotes an element from the
   * stack.
   */
  avm_size_t *call_stack;

  avm_size_t memory_size;
  avm_size_t stack_size;

  avm_size_t call_stack_size;

  /* The instruction pointer */
  avm_size_t ins;

  char* error;
};

int main(int argv, char** args) {
  printf("test\n");
  return 0;
}


/**
 * Sets an error code on the context with the given format
 * string and args.
 *
 * Returns failure so that `return error(...);` is useful
 */
static int error(AVM_Context ctx, const char* fmt, ...) {
  va_list ap;
  va_start(ap, fmt);
  if (vasprintf(&ctx->error, fmt, ap) < 0)
    ctx->error = NULL;
  va_end(ap);
  return 1;
}


/**
 * Returns 0 unless there has been an error.
 *
 * Error information can be obtained from `ctx` unless it's
 * an allocation error, in which case `ctx` will be NULL
 */
int init_avm(AVM_Context* ctx, AVM_Operation** ops, size_t oplen) {
  static const avm_size_t INITIAL_MEMORY_OVERHEAD = 1 << 12;
  static const size_t INITIAL_CALLSTACK_SIZE = 256;

  (*ctx) = my_malloc(sizeof(AVM_Context));
  if(ctx == NULL) return 1;

  (*ctx)->error = NULL;

  size_t memory_size = oplen * sizeof(AVM_Operation) + INITIAL_MEMORY_OVERHEAD;
  assert(memory_size < AVM_SIZE_MAX);

  // allocate enough memory for opcodes and some slack besides
  (*ctx)->memory_size = (avm_size_t) memory_size;
  (*ctx)->memory = my_calloc((*ctx)->memory_size, 1);
  if((*ctx)->memory == NULL)
    return error(*ctx, "unable to allocate heap (%d bytes)", memory_size);

  (*ctx)->stack_size = INITIAL_MEMORY_OVERHEAD;
  // malloc used because stack semantics guarantee
  // uninitialized data cannot be read
  (*ctx)->stack = my_malloc((*ctx)->stack_size);
  if((*ctx)->stack == NULL)
    return error(*ctx, "unable to allocate stack (%d bytes)", (*ctx)->stack_size);

  (*ctx)->stack_size = INITIAL_CALLSTACK_SIZE;
  (*ctx)->call_stack = my_malloc(INITIAL_CALLSTACK_SIZE * sizeof(avm_size_t));
  if((*ctx)->stack == NULL)
    return error(*ctx, "unable to allocate call stack", (*ctx)->call_stack_size);

  (*ctx)->ins = 0;

  return 0;
}
