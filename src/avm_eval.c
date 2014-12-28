#include <stdint.h>
#include <stdlib.h>
#include "avm.h"
#include "avm_util.h"


typedef void (*Evaluator)(const AVM_Operation, AVM_Context);

static void eval_calli(const AVM_Operation op, AVM_Context ctx) {

}

static Evaluator opcode_evalutators[opcode_count] = {
  [calli] = &eval_calli,
};
