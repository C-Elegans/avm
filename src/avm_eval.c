#include <stdint.h>
#include <stdlib.h>
#include "avm.h"
#include "avm_util.h"


typedef int (*Evaluator)(const AVM_Operation, AVM_Context*);

static int check_out_of_bounds(avm_size_t address, avm_size_t size) {
  if(((uint64_t) address + (uint64_t) size) > AVM_SIZE_MAX)
    return 1;
  else
    return 0;
}

static int push_call(AVM_Context* ctx, avm_size_t target) {
  if(ctx->call_stack_cap == ++ctx->call_stack_size){ // about to overflow? resize
    size_t new_size = ctx->call_stack_cap * 2;
    ctx->call_stack = my_crealloc(ctx->call_stack, ctx->call_stack_cap, new_size);

    if(ctx->call_stack == NULL)
      return avm__error(ctx, "Unable to reallocate stack of %d bytes", new_size);
  }

  ctx->call_stack[ctx->call_stack_size - 1] = target;

  return 0;
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
static int eval_load ( const AVM_Operation op, AVM_Context* ctx ) {
  avm_size_t size = op.size;
  avm_size_t address = op.address;

  if(check_out_of_bounds(address, size))
    return avm__error(ctx, "Unable to execute load from %x, size %x: out of bounds",
                      address, size);

  for(avm_size_t idx = address; idx < size + address; ++idx){
    avm_int data;

    int ret = avm_heap_get(ctx, &data, idx);
    if(ret != 0) return ret; // avm_heap_get set the error code

    ret = avm_stack_push(ctx, data);
    if(ret != 0) return ret; // avm_stack_push set the error code
  }

  return 0;
}

/* Pops `size` items off the stack and places them on the heap
 * at the given location.
 */
static int eval_store ( const AVM_Operation op, AVM_Context* ctx ) {
  avm_size_t size = op.size;
  avm_size_t address = op.address;

  if(check_out_of_bounds(address, size))
    return avm__error(ctx, "Unable to execute store to %x, size %x: out of bounds",
                      address, size);

  for(avm_size_t idx = address; idx < size + address; ++idx) {
    avm_int data;

    int ret = avm_stack_pop(ctx, &data);
    if(ret != 0) return ret; // error string already set

    ret = avm_heap_set(ctx, data, idx);
    if(ret != 0) return ret; // error string already set
  }

  return 0;
}

/* Places the immediate value at the top of the sack
 */
static int eval_push ( const AVM_Operation op, AVM_Context* ctx ) {
  avm_int data = ctx->memory[++ctx->ins];
  return avm_stack_push(ctx, data);
}

#define SIMPLE_BINOP(NAME, OP)                                         \
static int eval_ ## NAME ( const AVM_Operation op, AVM_Context* ctx ) { \
  avm_int a, b;                                                        \
  if(avm_stack_pop(ctx, &a)) return 1;                                 \
  if(avm_stack_pop(ctx, &b)) return 1;                                 \
  if(avm_stack_push(ctx, OP)) return 1;                                \
  return 0;                                                            \
}

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

static int eval_calli ( const AVM_Operation op, AVM_Context* ctx ) {
  ctx->ins = op.target;
  push_call(ctx, op.target);
  return 0;
}

static int eval_call ( const AVM_Operation op, AVM_Context* ctx ) {
  avm_int target;
  if(avm_stack_pop(ctx, &target)) return 1;
  ctx->ins = (avm_size_t) target;
  push_call(ctx, (avm_size_t) target);
  return 0;
}

static int eval_ret ( const AVM_Operation op, AVM_Context* ctx ) {
  ctx->ins = ctx->call_stack[--ctx->call_stack_size];
  if(ctx->call_stack_size == 0)
    return avm__error(ctx, "Unable to return with no functions in the call stack");

  return 0;
}

static int eval_jmpez ( const AVM_Operation op, AVM_Context* ctx ) {
  avm_int test;
  if(avm_stack_pop(ctx, &test)) return 1;
  if(test == 1) ctx->ins = op.target;
  return 0;
}

static const Evaluator opcode_evalutators[opcode_count] = {
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

int eval(AVM_Context* ctx, avm_int* result) {
  while(1) {
    AVM_Operation op = { ctx->memory[ctx->ins] };

    if(op.kind == avm_opc_quit){
      if(avm_stack_pop(ctx, result))
        return 1;
      return 0;
    }

    if(op.kind >= opcode_count)
      return avm__error(ctx, "invalid opcode %d, cannot execute", op.kind);

    opcode_evalutators[op.kind](op, ctx);

    ++ctx->ins;
  }
}
