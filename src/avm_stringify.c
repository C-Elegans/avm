#include <stdint.h>
#include <stdlib.h>
#include "avm.h"
#include "avm_util.h"


typedef int (*Stringifier)(AVM_Context *, avm_size_t *ins, char **);

#define SIMPLE_BINOP(NAME) \
static int stringify_ ## NAME (AVM_Context* ctx, avm_size_t* ins, char** out) { \
  (*out) = afmt(#NAME); \
  if((*out) == NULL) return 1; \
  return 0; \
}

static int stringify_load(AVM_Context *ctx, avm_size_t *ins, char **out)
{
  AVM_Operation op;
  if (avm_heap_get(ctx, (avm_int *) &op, *ins)) {
    return 1;
  }

  (*out) = afmt("load %db from 0x%.4x", op.size, op.address);
  if (*out == NULL) { return 1; }
  return 0;
}

static int stringify_store(AVM_Context *ctx, avm_size_t *ins, char **out)
{
  AVM_Operation op;
  if (avm_heap_get(ctx, (avm_int *) &op, *ins)) {
    return 1;
  }

  (*out) = afmt("store %db to 0x%.4x", op.size, op.address);
  if (*out == NULL) { return 1; }
  return 0;
}

static int stringify_push(AVM_Context *ctx, avm_size_t *ins, char **out)
{
  avm_int val;
  if (avm_heap_get(ctx, &val, *ins + 1)) {
    return 1;
  }

  (*out) = afmt("push 0x%.16x (%d)", val, val);
  if (*out == NULL) { return 1; }
  *ins += 1;
  return 0;
}

static int stringify_calli(AVM_Context *ctx, avm_size_t *ins, char **out)
{
  AVM_Operation op;
  if (avm_heap_get(ctx, (avm_int *) &op, *ins)) {
    return 1;
  }

  (*out) = afmt("call 0x%.4x", op.target);
  if (*out == NULL) { return 1; }
  return 0;
}

static int stringify_error(AVM_Context *ctx, avm_size_t *ins, char **out)
{
  AVM_Operation op;
  (*out) = afmt("error 0x%.16x", op.value);
  if (*out == NULL) { return 1; }
  return 0;
}

// *INDENT-OFF*
SIMPLE_BINOP(add)
SIMPLE_BINOP(sub)
SIMPLE_BINOP(and)
SIMPLE_BINOP(or)
SIMPLE_BINOP(xor)
SIMPLE_BINOP(shr)
SIMPLE_BINOP(shl)
SIMPLE_BINOP(ret)
SIMPLE_BINOP(call)
SIMPLE_BINOP(jmpez)
SIMPLE_BINOP(quit)
// *INDENT-ON*

static const Stringifier stringifiers[opcode_count] = {
  [avm_opc_error] = &stringify_error,
  [avm_opc_load ] = &stringify_load,
  [avm_opc_store] = &stringify_store,
  [avm_opc_push ] = &stringify_push,
  [avm_opc_add  ] = &stringify_add,
  [avm_opc_sub  ] = &stringify_sub,
  [avm_opc_and  ] = &stringify_and,
  [avm_opc_or   ] = &stringify_or,
  [avm_opc_xor  ] = &stringify_xor,
  [avm_opc_shr  ] = &stringify_shr,
  [avm_opc_shl  ] = &stringify_shl,
  [avm_opc_calli] = &stringify_calli,
  [avm_opc_call ] = &stringify_call,
  [avm_opc_ret  ] = &stringify_ret,
  [avm_opc_jmpez] = &stringify_jmpez,
  [avm_opc_quit ] = &stringify_quit,
};

/* Stringifies the instruction in memory at the given
 * memory pointer. `ins` is incremented according to
 * the amount that has been stringified
 */
int avm_stringify(AVM_Context *ctx, avm_size_t *ins, char **output)
{
  AVM_Operation op;
  if (avm_heap_get(ctx, (avm_int *) &op, *ins)) {
    return 1;
  }

  if (op.kind >= opcode_count) {
    op.kind = avm_opc_error;
  }

  int retcode = stringifiers[op.kind](ctx, ins, output);
  *ins += 1;
  return retcode;
}

int avm_stringify_count(AVM_Context *ctx, avm_size_t ins, avm_size_t len,
                        char **output)
{
  if ((uint64_t) ins + (uint64_t) len > AVM_SIZE_MAX) {
    return avm__error(ctx, "Index %d and length %d are out of bounds", ins, len);
  }

  *output = my_calloc(0, 0);

  for (avm_size_t i = ins; i < (ins + len);) {
    char *tmpout;
    if (avm_stringify(ctx, &i, &tmpout)) {
      my_free(tmpout);
      return 1;
    }

    char *oldvalue = *output;
    *output = afmt("%s\n%s", oldvalue, tmpout);

    my_free(oldvalue);
    my_free(tmpout);

    if (*output == NULL) {
      return avm__error(ctx, "Unable to concatenate operation strings");
    }
  }

  return 0;
}
