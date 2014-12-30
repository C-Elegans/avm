#include <stdio.h>
#include <stdlib.h>
#include "avm.h"
#include "avm_util.h"

static void dump_ins(AVM_Context *ctx)
{
  char* ins_text;
  avm_size_t ins = ctx->ins;
  // don't really care about errors here
  /* discard */ avm_stringify(ctx, &ins, &ins_text);
  printf("%.4x 👉\t%s\n", ctx->ins, ins_text);
  my_free(ins_text);
}

static void dump_stack(AVM_Context *ctx)
{
  printf("┌───── stack dump ─────────\n");
  avm_size_t i = ctx->stack_size;
  while (1) {
    if (i == 0) { break; }
    i -= 1;
    printf("│%d.\t%.16lx (dec. %lu)\n", i, ctx->stack[i], ctx->stack[i]);
  }
  printf("└───── end stack dump ─────\n");
}
