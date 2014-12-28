#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <assert.h>
#include "avm.h"
#include "avm_util.h"


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
int avm__error(AVM_Context ctx, const char* fmt, ...) {
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

  size_t memory_size = oplen * sizeof(AVM_Operation) + INITIAL_MEMORY_OVERHEAD * sizeof(avm_int);
  assert(memory_size < AVM_SIZE_MAX);

  // allocate enough memory for opcodes and some slack besides
  (*ctx)->memory_size = (avm_size_t) memory_size;
  (*ctx)->memory = my_calloc((*ctx)->memory_size, 1);
  if((*ctx)->memory == NULL)
    return avm__error(*ctx, "unable to allocate heap (%d bytes)", memory_size);

  (*ctx)->stack_size = 0;
  (*ctx)->stack_cap = INITIAL_MEMORY_OVERHEAD * sizeof(avm_int);
  // malloc used because stack semantics guarantee
  // uninitialized data cannot be read
  (*ctx)->stack = my_malloc((*ctx)->stack_size);
  if((*ctx)->stack == NULL)
    return avm__error(*ctx, "unable to allocate stack (%d bytes)", (*ctx)->stack_size);

  (*ctx)->call_stack_size = 0;
  (*ctx)->call_stack_cap = INITIAL_CALLSTACK_SIZE;
  (*ctx)->call_stack = my_malloc(INITIAL_CALLSTACK_SIZE * sizeof(avm_size_t));
  if((*ctx)->call_stack == NULL)
    return avm__error(*ctx, "unable to allocate call stack", (*ctx)->call_stack_size);

  (*ctx)->ins = 0;

  return 0;
}


int avm_heap_get(AVM_Context ctx, avm_int* data, avm_size_t loc) {
  if(loc >= ctx->memory_size){ // memory never written, it's 0.
    *data = 0;
    return 0;
  }

  *data = ctx->memory[loc];
  return 0;
}

int avm_heap_set(AVM_Context ctx, avm_int data, avm_size_t loc) {
  if(loc >= ctx->memory_size){
    if(data == 0) // pretend it's been written
      return 0;
    else {
      avm_size_t new_size = ctx->memory_size * 2 * sizeof(avm_int);
      ctx->memory = my_crealloc(ctx->memory, ctx->memory_size, new_size);
      if(ctx->memory == NULL)
        return avm__error(ctx, "unable to allocate more memory (%d bytes)", new_size);

      ctx->memory_size = new_size;
      // recursivly double memory until enough is initialized
      return avm_heap_set(ctx, data, loc);
    }
  }

  ctx->memory[loc] = data;
  return 0;
}

int avm_stack_push(AVM_Context ctx, avm_int data) {
  if(ctx->stack_cap <= ++ctx->stack_size) {
    avm_size_t new_cap = ctx->stack_cap * 2 * sizeof(avm_int);
    ctx->stack = my_realloc(ctx->stack, new_cap);
    if(ctx->stack == NULL)
      return avm__error(ctx, "unable to increase stack size (%d bytes)", new_cap);

    ctx->stack_cap = new_cap;
  }

  ctx->stack[ctx->stack_size - 1] = data;
  return 0;
}

int avm_stack_pop(AVM_Context ctx, avm_int* data) {
  if(ctx->stack_size == 0)
    return avm__error(ctx, "unable to pop item off stack: stack underrun");

  *data = ctx->stack[--ctx->stack_size];
  return 0;
}

int avm_stack_peak(AVM_Context ctx, avm_int* data) {
  if(ctx->stack_size == 0)
    return avm__error(ctx, "unable to read item off stack: stack underrun");

  *data = ctx->stack[ctx->stack_size - 1];
  return 0;
}
