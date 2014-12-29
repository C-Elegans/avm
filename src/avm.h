#ifndef _AVM_
#define _AVM_

typedef uint16_t avm_size_t;
#define AVM_SIZE_MAX UINT16_MAX

typedef uint64_t avm_int;
#define AVM_INT_MAX UINT64_MAX

typedef struct { // contents on the struct are internal
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

  uint32_t padding_;
  char* error;
} AVM_Context;

typedef enum {
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
} AVM_Opcode;


typedef union{
  avm_int value;

  struct {
    AVM_Opcode kind;
    union {
      /* load, store */
      struct {
        avm_size_t address;
        avm_size_t size;
      };
      /* calli */
      struct { avm_size_t target; };
    };
  };
} AVM_Operation;

int init_avm(AVM_Context* ctx, const AVM_Operation* ops, size_t oplen);
void avm_free(AVM_Context ctx);
int eval(AVM_Context* ctx, avm_int* result);

int avm_heap_get(AVM_Context* ctx, avm_int* data, avm_size_t loc);
int avm_heap_set(AVM_Context* ctx, avm_int data, avm_size_t loc);

int avm_stack_push(AVM_Context* ctx, avm_int data);
int avm_stack_pop(AVM_Context* ctx, avm_int* data);
int avm_stack_peak(AVM_Context* ctx, avm_int* data);

int avm_stringify(AVM_Context* ctx, avm_size_t* ins, char** output);
int avm_stringify_count(AVM_Context* ctx, avm_size_t ins, avm_size_t len, char** output);


int avm__error(AVM_Context* ctx, const char* fmt, ...);

#endif
