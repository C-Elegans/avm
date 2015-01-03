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
    avm_int value;
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
  while (*input[0] != '\n' && *input[0] != '\0') {
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
    "mul",
    "div",
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
    "dup",
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

  if (oplen == 0) { 
    my_free(operation);
    return 0;
  }

  for (AVM_Opcode i = 0; i < opcode_count; ++i) {
    if (strcmp(str_to_opcode[i], operation) == 0) {
      *result = (Token) { .type = tt_operation, .opc = i };
      my_free(operation);
      return 1;
    }
  }

  my_free(operation);
  *input -= oplen;
  return 0;
}

static int try_lex_num (const char **input, Token *result)
{
  avm_int target;
  int len;
  if (sscanf(*input, " %lx%n", &target, &len) == 1) {
    *result = (Token) { .type = tt_num, .value = target };
    *input += len;
    return 1;
  }

  return 0;
}

static int try_lex_label (const char **input, Token *result)
{
  // number with trailing `:`
  int len;
  avm_int target;
  if (sscanf(*input, " %lx%n", &target, &len) == 1 &&
      (*input)[len] == ':') {
    *result = (Token) { .type = tt_label, .value = target };
    *input += len + 1;
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

static int lex_error(Token *result)
{
  *result = (Token) { .type = tt_error, .message = "unkown token" };
  return 0; // always succeeds
}

static int lex_input(const char **input, Token *result)
{
  // unusual, returns 0 on failure and 1 on success
  return try_lex_label(input, result) ||
         try_lex_operation(input, result) ||
         try_lex_num(input, result) ||
         try_lex_eof(input, result) ||
         lex_error(result);
}

int avm_parse(const char *input, avm_int **output, char **error, size_t *outputlen)
{
  const char *input_var = input;
  size_t memory_loc = 0;
  Token nextTok;

  *output = my_calloc(SLACK_SIZE, sizeof(avm_int));
  size_t memorycap = SLACK_SIZE;

  while (lex_input(&input_var, &nextTok)) {
    if (memory_loc + 2 >= memorycap) { // resize
      size_t newcap = memory_loc + SLACK_SIZE;
      *output = my_crealloc(*output, memorycap * sizeof(avm_int), newcap * sizeof(avm_int));
      if (*output == NULL) {
        *error = afmt("%d: Allocation failed\n", input_var - input);
        return 1;
      }
      memorycap = newcap;
    }

    if (nextTok.type == tt_error) {
      *error = afmt("%d: %s", input_var - input, nextTok.message);
      return 1;
    } else if (nextTok.type == tt_label) {
      if (nextTok.value > AVM_SIZE_MAX) {
        *error = afmt("%d: label address is out of bounds");
        return 1;
      }

      memory_loc = (avm_size_t) nextTok.value;
      skip_whitespace(&input_var); // do not continue to newline
      continue;
    } else if (nextTok.type == tt_operation) {
      if (nextTok.opc == avm_opc_error) {
        (*output)[memory_loc] = nextTok.value;
        memory_loc += 1;
      } else if (nextTok.opc == avm_opc_load || nextTok.opc == avm_opc_store) {
        Token size;
        Token address;
        if (!lex_input(&input_var, &size) ||
            size.type != tt_num) {
          *error = afmt("%d: expected size\n", input_var - input);
          return 1;
        }
        if (!lex_input(&input_var, &address) ||
            address.type != tt_num) {
          *error = afmt("%d: expected address\n", input_var - input);
          return 1;
        }

        if (size.value > (1 << 24) || address.value > AVM_SIZE_MAX) {
          *error = afmt("%d: size or value out of bounds\n", input_var - input);
          return 1;
        }

        (*output)[memory_loc] = ((AVM_Operation) {
          .kind = nextTok.opc,
          .size = (avm_size_t) size.value,
          .address = (avm_size_t) address.value
        }).value;

        memory_loc += 1;
      } else if (nextTok.opc == avm_opc_calli ||
          nextTok.opc == avm_opc_jmpez) {
        Token address;
        if (!lex_input(&input_var, &address) ||
            address.type != tt_num) {
          *error = afmt("%d: expected address\n", input_var - input);
          return 1;
        }

        (*output)[memory_loc] = ((AVM_Operation) {
          .kind = nextTok.opc,
          .address = (avm_size_t) address.value
        }).value;

        memory_loc += 1;
      } else if (nextTok.opc == avm_opc_push) {
        Token value;
        if (!lex_input(&input_var, &value) ||
            value.type != tt_num) {
          *error = afmt("%d: expected value to push, got %d\n", input_var - input, value.type);
          return 1;
        }

        (*output)[memory_loc] = ((AVM_Operation) { .kind = avm_opc_push }).value;
        (*output)[memory_loc + 1] = value.value;

        memory_loc += 2;
      } else if (nextTok.opc == avm_opc_error) {
        (*output)[memory_loc] = nextTok.value;
        memory_loc += 1;
      } else if (nextTok.opc < opcode_count) {
        (*output)[memory_loc] = ((AVM_Operation) { .kind = nextTok.opc }).value;
        memory_loc += 1;
      } else {
        printf("Internal error: WTF");
        exit(EXIT_FAILURE);
      }
    } else if (nextTok.type == tt_eof) {
      *outputlen = memory_loc;
      return 0;
    } else {
      *error = afmt("%d: unknown error\n", input_var - input);
      return 1;
    }

    continue_until_newline(&input_var);
    skip_whitespace(&input_var);
  }

  *error = afmt("%d: unknown error\n", input_var - input);
  return 1;
}
