#ifndef _AVM_DEF_H
#define _AVM_DEF_H

enum {
  avm_opc_error,  /* should never be executed */
  avm_opc_load,   /* loads `size` bytes from `address` and pushes them to the stack */
  avm_opc_store,  /* pops `size` bytes from the stack and stores them at `address` */
  avm_opc_push,
  avm_opc_add,
  avm_opc_sub,
  avm_opc_and,
  avm_opc_or,
  avm_opc_xor,
  avm_opc_shr,
  avm_opc_shl,
  avm_opc_calli,
  avm_opc_call,
  avm_opc_ret,
  avm_opc_jmpez,  /* Jumps to the `target` if the top of the stack is `1` */
  avm_opc_quit,

  opcode_count
};

typedef uint8_t AVM_Opcode;

_Static_assert(sizeof(AVM_Opcode) == 1, "opcode must be one byte");

typedef union {
  avm_int value;

  struct {
    AVM_Opcode kind : 8;
    uint32_t size : 24;
    avm_size_t address : 32;
  };
} AVM_Operation;

_Static_assert(sizeof(AVM_Operation) == sizeof(avm_int),
               "Operataion must be same size as an avm_int");

typedef struct AVM_Context_s {
  avm_int *memory;
  avm_int *stack;
  /**
   * each `call` execution pushes the given address to this
   * stack, and each call to `ret` remotes an element from the
   * stack.
   */
  avm_size_t *call_stack;

  avm_size_t memory_size;
  avm_size_t stack_size;
  avm_size_t stack_cap;

  avm_size_t call_stack_size;
  avm_size_t call_stack_cap;

  /* The instruction pointer */
  avm_size_t ins;


  char *error;
} AVM_Context;

#endif
