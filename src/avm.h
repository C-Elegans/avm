#ifndef _AVM_
#define _AVM_

typedef uint16_t avm_size_t;
#define AVM_SIZE_MAX UINT16_MAX

typedef uint64_t avm_int;
#define AVM_INT_MAX UINT64_MAX

typedef struct Context_s* AVM_Context;

typedef enum {
  load,   /* loads `size` bytes from `address` and pushes them to the stack */
  store,  /* pops `size` bytes from the stack and stores them at `address` */
  loadi, storei,
  addi, subi, andi, ori, shri, shli,
  add, sub, and, or, shr, shl,
  calli,
  ret,
  jump,

  opcode_count
} AVM_Opcode;


#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wpadded"
#endif
typedef struct {
  AVM_Opcode kind;
  union {
    /* load, store */
    struct { avm_size_t address; avm_size_t size; };
    /* addi, subi, andi, ori, shri, shli, storei, loadi, calli */
    struct { avm_int immediate; }; 
  };
} AVM_Operation;
#ifdef __clang__
#pragma clang diagnostic pop
#endif


int init_avm(AVM_Context* ctx, AVM_Operation** ops, size_t oplen);

#endif
