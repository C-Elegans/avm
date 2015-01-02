#include <stdint.h>
#include <stdlib.h>
#include <assert.h>
#include "avm.h"
#include "avm_util.h"
#include "avm_def.h"


typedef int (*Evaluator)(const AVM_Operation, AVM_Context *);

static int push_call(AVM_Context *ctx, avm_size_t target)
{
  if (ctx->call_stack_size + 1 == AVM_SIZE_MAX) {
    return avm__error(ctx, "Call stack overflow");
  }

  if (ctx->call_stack_cap == ctx->call_stack_size + 1) { // overflowing? resize
    size_t new_size = min(ctx->call_stack_cap * 2, AVM_SIZE_MAX);
    ctx->call_stack = my_crealloc(ctx->call_stack,
                                  ctx->call_stack_cap * sizeof(avm_size_t),
                                  new_size * sizeof(avm_size_t));

    if (ctx->call_stack == NULL) {
      return avm__error(ctx, "Unable to reallocate call stack of %d elements",
                        new_size);
    }

    assert((avm_size_t) new_size == new_size);
    ctx->call_stack_cap = (avm_size_t) new_size;
  }

  ctx->call_stack[ctx->call_stack_size] = target;

  ctx->call_stack_size += 1;

  return 0;
}

static int eval_error ( const AVM_Operation op, AVM_Context *ctx )
{
  return avm__error(ctx, "Invalid opcode 0x%.16x", op.value);
}

/* Extract the amount of memory specified in `size` and push it to
 * the stack, lowest byte first.
 *
 * ┌─┬─┬─┬─┐
 * │1│2│3│4│
 * └─┴─┴─┴─┘
 *    \_/
 *     ↓
 *    ┌─┐
 *    │2│
 *    ├─┤
 *    │3│
 *    ├─┤
 *    │⋮│
 *    └─┘
 */
static int eval_load ( const AVM_Operation op, AVM_Context *ctx )
{
  avm_size_t size = op.size;
  avm_size_t address = op.address;

  if (asizet_add_bounds_check(address, size))
    return avm__error(ctx, "Unable to execute load from %x, size %x: out of bounds",
                      address, size);

  for (avm_size_t idx = address; idx < size + address; ++idx) {
    avm_int data;

    if (avm_heap_get(ctx, &data, idx)) { return 1; }
    if (avm_stack_push(ctx, data)) { return 1; }
  }

  return 0;
}

/* Pops `size` items off the stack and places them on the heap
 * at the given location.
 */
static int eval_store ( const AVM_Operation op, AVM_Context *ctx )
{
  avm_size_t size = op.size;
  avm_size_t address = op.address;

  if (asizet_add_bounds_check(address, size))
    return avm__error(ctx, "Unable to execute store to %x, size %x: out of bounds",
                      address, size);

  for (avm_size_t idx = address; idx < size + address; ++idx) {
    avm_int data;

    if (avm_stack_pop(ctx, &data)) { return 1; }
    if (avm_heap_set(ctx, data, idx)) { return 1; }
  }

  return 0;
}

/* Places the immediate value at the top of the sack
 */
static int eval_push ( const AVM_Operation op, AVM_Context *ctx )
{
  avm_int data = ctx->memory[ctx->ins + 1];
  ctx->ins += 1;
  return avm_stack_push(ctx, data);
}

#define SIMPLE_BINOP(NAME, OP) \
static int eval_ ## NAME ( const AVM_Operation op, AVM_Context* ctx ) { \
  avm_int a, b; \
  if(avm_stack_pop(ctx, &a)) return 1; \
  if(avm_stack_pop(ctx, &b)) return 1; \
  if(avm_stack_push(ctx, OP)) return 1; \
  return 0; \
}

// *INDENT-OFF*

/* push(pop() + pop()) */
SIMPLE_BINOP(add, a + b)

/* push(pop() - pop()) */
SIMPLE_BINOP(sub, a - b)

/* push(pop() & pop()) */
SIMPLE_BINOP(and, a & b)

/* push(pop() | pop()) */
SIMPLE_BINOP(or, a | b)

/* push(pop() ^ pop()) */
SIMPLE_BINOP(xor, a ^ b)

/* push(pop() >> pop()), rhs > 63 is defined as 0 */
SIMPLE_BINOP(shr, a >> (b & 0x3F))

/* push(pop() << pop()), rhs > 63 is defined as 0 */
SIMPLE_BINOP(shl, a << (b & 0x3F))

// *INDENT-ON*

/* call(0xF00BA4) */
static int eval_calli ( const AVM_Operation op, AVM_Context *ctx )
{
  ctx->ins = op.address;
  ctx->ins -= 1;  // exec() increments it 1 later, compensate
  // Underflow is OK - it will overflow back immediately
  return push_call(ctx, op.address);
}

/* call(pop()) */
static int eval_call ( const AVM_Operation op, AVM_Context *ctx )
{
  avm_int target;
  if (avm_stack_pop(ctx, &target)) { return 1; }
  ctx->ins = (avm_size_t) target;
  ctx->ins -= 1;  // see eval_calli
  return push_call(ctx, (avm_size_t) target);
}

static int eval_ret ( const AVM_Operation op, AVM_Context *ctx )
{
  if (ctx->call_stack_size == 0) {
    return avm__error(ctx, "Unable to return with no functions in the call stack");
  }

  ctx->ins = ctx->call_stack[ctx->call_stack_size];
  ctx->call_stack_size -= 0;

  return 0;
}

/* if(pop() == 0) goto 0xF00BA4 */
static int eval_jmpez ( const AVM_Operation op, AVM_Context *ctx )
{
  avm_int test;
  if (avm_stack_pop(ctx, &test)) { return 1; }
  if (test == 1) { ctx->ins = op.address; }
  ctx->ins -= 1;  // see eval_calli
  return 0;
}

static const Evaluator opcode_evalutators[opcode_count] = {
  [avm_opc_error] = &eval_error,
  [avm_opc_load ] = &eval_load,
  [avm_opc_store] = &eval_store,
  [avm_opc_push ] = &eval_push,
  [avm_opc_add  ] = &eval_add,
  [avm_opc_sub  ] = &eval_sub,
  [avm_opc_and  ] = &eval_and,
  [avm_opc_or   ] = &eval_or,
  [avm_opc_xor  ] = &eval_xor,
  [avm_opc_shr  ] = &eval_shr,
  [avm_opc_shl  ] = &eval_shl,
  [avm_opc_calli] = &eval_calli,
  [avm_opc_call ] = &eval_call,
  [avm_opc_ret  ] = &eval_ret,
  [avm_opc_jmpez] = &eval_jmpez
};

#ifdef AVM_DEBUG
#include "avm_debug.c"
#endif

int avm_eval(AVM_Context *ctx, avm_int *result)
{
  assert(ctx != NULL);
  assert(result != NULL);

  while (1) {
    AVM_Operation op;
    avm_heap_get(ctx, (avm_int *) &op, ctx->ins);
#ifdef AVM_DEBUG
    dump_ins(ctx);
#endif

    if (op.kind == avm_opc_quit) {
      return avm_stack_pop(ctx, result);
    }

    if (op.kind >= opcode_count) {
      op.kind = avm_opc_error;
    }

    if (opcode_evalutators[op.kind](op, ctx)) {
      return 1;
    }

#ifdef AVM_DEBUG
    dump_stack(ctx);
#endif

    ctx->ins += 1;
  }
}
