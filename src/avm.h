#ifndef _AVM_H
#define _AVM_H

#include <stdint.h>
#include <stddef.h>

typedef uint32_t avm_size_t;
#define AVM_SIZE_MAX UINT32_MAX

typedef uint64_t avm_int;
#define AVM_INT_MAX UINT64_MAX

typedef struct AVM_Context_s AVM_Context;

int  avm_init(AVM_Context *ctx, const avm_int *initial_mem, size_t oplen);
void avm_free(AVM_Context *ctx);

int avm_eval(AVM_Context *ctx, avm_int *result);

int avm_heap_get(AVM_Context *ctx, avm_int *data, avm_size_t loc);
int avm_heap_set(AVM_Context *ctx, avm_int data, avm_size_t loc);

int avm_stack_push(AVM_Context *ctx, avm_int data);
int avm_stack_pop(AVM_Context *ctx, avm_int *data);
int avm_stack_peak(AVM_Context *ctx, avm_int *data);

int avm_stringify(AVM_Context *ctx, avm_size_t *ins, char **output);
int avm_stringify_count(AVM_Context *ctx, avm_size_t ins, avm_size_t len,
                        char **output);


#endif /* _AVM_H_ */
