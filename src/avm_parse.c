#include "avm.h"
#include "avm_def.h"
#include "avm_util.h"
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>

#define SLACK_SIZE 127

typedef enum {
  tt_operation, tt_label, tt_num, tt_eof, tt_error,
} TokenType;

typedef struct {
  TokenType type;
  uint32_t _dummy;

  union {
    AVM_Opcode opc;
    uint64_t value;
    char *message;
  };
} Token;

static void skip_whitespace (const char **input)
{
  while (isspace(**input)) {
    *input += 1;
  }
}

static void continue_until_newline (const char **input)
{
  while (*input[0] != '\n' || *input[0] != '\0') {
    *input += 1;
  }
}

static int try_lex_operation (const char **input, Token *result)
{
  static const char *str_to_opcode[opcode_count] = {
    "error",
    "load",
    "store",
    "push",
    "add",
    "sub",
    "and",
    "or",
    "xor",
    "shr",
    "shl",
    "calli",
    "call",
    "ret",
    "jmpez",
    "quit",
  };

  char *operation = my_malloc(SLACK_SIZE);
  size_t opcap = SLACK_SIZE;
  size_t oplen = 0;

  while ((*input[0] >= 'a' && *input[0] <= 'z') ||
         (*input[0] >= 'A' && *input[0] <= 'Z')) {
    operation[oplen] = *input[0];

    *input += 1;
    oplen += 1;

    if (oplen == opcap) {
      opcap += SLACK_SIZE;
      operation = my_realloc(operation, opcap);
    }

    operation[oplen] = '\0'; // always have a trailing null
  }

  if (oplen == 0) { return 0; }

  for (AVM_Opcode i = 0; i < opcode_count; ++i) {
    if (strcmp(str_to_opcode[i], operation) == 0) {
      *result = (Token) { .type = tt_operation, .opc = i };
      return 1;
    }
  }

  *result = (Token) { .type = tt_error, .message = "unkown operation" };
  return 1;
}

static int try_lex_num (const char **input, Token *result)
{
  avm_size_t target;
  int len = sscanf(*input, "%x", &target);
  if (len == 1) {
    *result = (Token) { .type = tt_num, .value = target };
    *input += len;
    return 1;
  }

  return 0;
}

static int try_lex_label (const char **input, Token *result)
{
  // number with trailing `:`
  const char *input_var = *input;
  if (try_lex_num(&input_var, result) == 1 && input_var[0] == ':') {
    result->type = tt_label;
    *input = input_var;
    *input += 1;
    return 1;
  }

  return 0;
}

static int try_lex_eof (const char **input, Token *result)
{
  if (*input[0] == '\0') {
    *result = (Token) { .type = tt_eof };
    return 1;
  }
  return 0;
}

static int lex_input(const char **input, Token *result)
{
  // unusual, returns 0 on failure and 1 on success
  return try_lex_operation(input, result) ||
         try_lex_label(input, result) ||
         try_lex_num(input, result) ||
         try_lex_eof(input, result);
}

int avm_parse(const char *input, avm_int **output, char *error)
{
  const char *input_var = input;
  avm_size_t memory_loc = 0;
  Token nextTok;

  *output = my_calloc(SLACK_SIZE, sizeof(avm_int));
  size_t memorycap = SLACK_SIZE;

  while (lex_input(&input_var, &nextTok)) {
    if (nextTok.type == tt_eof) { return 0; }

    if (nextTok.type == tt_error) {
      error = nextTok.message;
      return 1;
    }

    if (nextTok.type == tt_label) {
      if (nextTok.value > AVM_SIZE_MAX) {
        error = "label address is out of bounds";
        return 1;
      }

      memory_loc = (avm_size_t) nextTok.value;
      continue;
    }

    if (nextTok.type == tt_operation) {
      if (memory_loc + 2 >= memorycap) { // resize
        size_t newcap = memorycap + SLACK_SIZE;
        *output = my_crealloc(*output, memorycap * sizeof(avm_int), newcap * sizeof(avm_int));
        if (*output == NULL) {
          error = "Allocation failed";
          return 1;
        }
        memorycap = newcap;
      }

      if (nextTok.opc == avm_opc_error) {
        *output[memory_loc] = nextTok.value;
        memory_loc += 1;
      } else if (nextTok.opc == avm_opc_load || nextTok.opc == avm_opc_store) {
        Token size;
        Token address;
        if (!lex_input(&input_var, &size) ||
            size.type != tt_num) { error = "expected size"; return 1; }
        if (!lex_input(&input_var, &address) ||
            address.type != tt_num) { error = "expected address"; return 1; }

        if (size.value > (1 << 24) || address.value > AVM_SIZE_MAX) {
          error = "size or value out of bounds";
          return 1;
        }

        *output[memory_loc] = ((AVM_Operation) {
          .kind = nextTok.opc,
          .size = (avm_size_t) size.value,
          .address = (avm_size_t) address.value
        }).value;

        memory_loc += 1;
      } else if (nextTok.opc == avm_opc_calli) {
        Token address;
        if (!lex_input(&input_var, &address) ||
            address.type != tt_num) { error = "expected address"; return 1; }

        *output[memory_loc] = ((AVM_Operation) {
          .kind = nextTok.opc,
          .address = (avm_size_t) address.value
        }).value;

        memory_loc += 1;
      } else if (nextTok.opc == avm_opc_push) {
        Token value;
        if (!lex_input(&input_var, &value) ||
            value.type != tt_num) { error = "expected value to push"; return 1; }

        *output[memory_loc] = ((AVM_Operation) { .kind = avm_opc_push }).value;
        *output[memory_loc + 1] = value.value;

        memory_loc += 2;
      } else if (nextTok.opc == avm_opc_error) {
        *output[memory_loc] = nextTok.value;
        memory_loc += 1;
      } else if (nextTok.opc < opcode_count) {
        *output[memory_loc] = ((AVM_Operation) { .kind = nextTok.opc }).value;
        memory_loc += 1;
      } else {
        printf("Internal error: WTF");
        exit(EXIT_FAILURE);
      }
    }

    continue_until_newline(&input_var);
    skip_whitespace(&input_var);
  }

  return 1;
}
