#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <assert.h>
#include "avm.h"
#include "avm_util.h"

#ifdef AVM_EXECUTABLE

int main(int argv, char **args)
{
  size_t bytes_read;
  char *opc = read_file(stdin, &bytes_read);
  if (opc == NULL) {
    printf("unable to read input");
    return 1;
  }

  size_t ops_read = bytes_read / sizeof(AVM_Operation);

  AVM_Context ctx;
  int retcode = init_avm(&ctx, (void *) opc, ops_read);
  my_free(opc);
  if (retcode) {
    printf("failed to initialize vm");
    return 1;
  }

  char *result;
  if (avm_stringify_count(&ctx, 0, (avm_size_t) ops_read, &result)) {
    printf("err: %s\n", ctx.error);
  }
  printf("%s\n", result);
  my_free(result);
  my_free(ctx.error);

  avm_int eval_prog_ret = 0;
  if (eval(&ctx, &eval_prog_ret)) {
    printf("err: %s\n", ctx.error);
    avm_free(ctx);
    return 1;
  }

  avm_free(ctx);
  return (int) eval_prog_ret;
}

#endif  /* AVM_EXECUTABLE */



#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wformat-nonliteral"
/**
 * Sets an error code on the context with the given format
 * string and args.
 *
 * Returns failure so that `return error(...);` is useful
 */
int avm__error(AVM_Context *ctx, const char *fmt, ...)
{
  // black magic based on afmt()
  va_list ap;
  va_start(ap, fmt);
  if (vasprintf(&ctx->error, fmt, ap) < 0) {
    ctx->error = NULL;
  }
  va_end(ap);
  return 1;
}
#pragma clang diagnostic pop


/**
 * Returns 0 unless there has been an error.
 *
 * Error information can be obtained from `ctx` unless it's
 * an allocation error, in which case `ctx` will be NULL
 */
int init_avm(AVM_Context *ctx, const AVM_Operation *ops, size_t oplen)
{
  static const avm_size_t INITIAL_MEMORY_OVERHEAD = 1 << 12;
  static const size_t INITIAL_CALLSTACK_SIZE = 256;

  ctx->error = NULL;

  assert((oplen * sizeof(AVM_Operation)) / sizeof(avm_int) < AVM_SIZE_MAX / 2);
  size_t memory_size = oplen  + INITIAL_MEMORY_OVERHEAD;

  // allocate enough memory for opcodes and some slack besides
  ctx->memory_size = (avm_size_t) memory_size;
  ctx->memory = my_calloc(ctx->memory_size * sizeof(avm_int), 1);
  if (ctx->memory == NULL) {
    return avm__error(ctx, "unable to allocate heap (%d avm_int)", memory_size);
  }
  memcpy(ctx->memory, ops, oplen * sizeof(AVM_Operation));

  ctx->stack_size = 0;
  ctx->stack_cap = INITIAL_MEMORY_OVERHEAD;
  // malloc used because stack semantics guarantee
  // uninitialized data cannot be read
  ctx->stack = my_malloc(ctx->stack_cap * sizeof(avm_int));
  if (ctx->stack == NULL) {
    return avm__error(ctx, "unable to allocate stack (%d bytes)", ctx->stack_cap);
  }

  ctx->call_stack_size = 0;
  ctx->call_stack_cap = INITIAL_CALLSTACK_SIZE;
  ctx->call_stack = my_malloc(INITIAL_CALLSTACK_SIZE * sizeof(avm_size_t));
  if (ctx->call_stack == NULL) {
    return avm__error(ctx, "unable to allocate call stack", ctx->call_stack_cap);
  }

  ctx->ins = 0;

  ctx->initialized = _INITIALIZED_CONSTANT;

  return 0;
}

void avm_free(AVM_Context ctx)
{
  my_free(ctx.error);
  my_free(ctx.memory);
  my_free(ctx.stack);
  my_free(ctx.call_stack);
}


int avm_heap_get(AVM_Context *ctx, avm_int *data, avm_size_t loc)
{
  if (loc >= ctx->memory_size) { // memory never written, it's 0.
    *data = 0;
    return 0;
  }

  *data = ctx->memory[loc];
  return 0;
}

int avm_heap_set(AVM_Context *ctx, avm_int data, avm_size_t loc)
{
  if (data == 0) return 0;  // pretend it's been written

  // double memory until enough is initialized
  while (loc >= ctx->memory_size) {
    avm_size_t new_size = (avm_size_t) min(ctx->memory_size * 2, AVM_SIZE_MAX);
    ctx->memory = my_crealloc(ctx->memory, ctx->memory_size * sizeof(avm_int),
                              new_size * sizeof(avm_int));

    if (ctx->memory == NULL) {
      return avm__error(ctx, "unable to allocate more memory (%d avm_ints)", new_size);
    }

    ctx->memory_size = new_size;
  }

  ctx->memory[loc] = data;
  return 0;
}

int avm_stack_push(AVM_Context *ctx, avm_int data)
{
  if (ctx->stack_size == AVM_SIZE_MAX) {
    // incrementing it now would jump to 0
    return avm__error(ctx, "Stack overflow");
  }

  ctx->stack_size += 1;

  if (ctx->stack_cap <= ctx->stack_size) {
    avm_size_t new_cap = (avm_size_t) min(ctx->stack_cap * 2, AVM_SIZE_MAX);
    ctx->stack = my_realloc(ctx->stack, new_cap * sizeof(avm_int));
    if (ctx->stack == NULL) {
      return avm__error(ctx, "unable to increase stack size (%d bytes)", new_cap);
    }

    ctx->stack_cap = new_cap;
  }

  assert(ctx->stack_size != 0);  // if it was 0, it'd cause overflow
  ctx->stack[ctx->stack_size - 1] = data;

  return 0;
}

int avm_stack_pop(AVM_Context *ctx, avm_int *data)
{
  if (ctx->stack_size == 0) {
    return avm__error(ctx, "unable to pop item off stack: stack underrun");
  }

  *data = ctx->stack[--ctx->stack_size];
  return 0;
}

int avm_stack_peak(AVM_Context *ctx, avm_int *data)
{
  if (ctx->stack_size == 0) {
    return avm__error(ctx, "unable to read item off stack: stack underrun");
  }

  *data = ctx->stack[ctx->stack_size - 1];
  return 0;
}
