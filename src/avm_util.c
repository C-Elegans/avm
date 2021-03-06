#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "avm.h"
#include "avm_util.h"
#include "avm_def.h"

int asizet_add_bounds_check(avm_size_t address, avm_size_t size)
{
  return (uint64_t) address + (uint64_t) size - 1 > AVM_SIZE_MAX;
}


void *my_crealloc(void *buffer, size_t oldsize, size_t newsize)
{
  void *result = realloc(buffer, newsize);

  if (newsize > oldsize && result != NULL) {
    size_t grow_len = newsize - oldsize;
    memset(((char *) result) + oldsize, 0, grow_len);
  }

  return result;
}

void *my_malloc(size_t size)
{
  return malloc(size);
}
void *my_calloc(size_t count, size_t size)
{
  return calloc(count, size);
}
void *my_realloc(void *buffer, size_t newsize)
{
  return realloc(buffer, newsize);
}

size_t min(size_t a, size_t b)
{
  if (a < b) { return a; }
  else { return b; }
}

#define BUFFER_SIZE 4095
char *read_file(FILE *file, size_t *len)
{
  char buffer[BUFFER_SIZE];
  char *result = NULL;
  size_t resultlen = 0;

  while (1) {
    size_t amount_read = fread(buffer, 1, BUFFER_SIZE, file);
    size_t oldlen = resultlen;
    resultlen += amount_read;

    result = my_realloc(result, resultlen + 1);

    memcpy(result + oldlen, buffer, amount_read);

    if (amount_read < BUFFER_SIZE) {
      *len = resultlen;
      result[resultlen] = '\0';
      return result;
    }
  }
}


#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wformat-nonliteral"
int avm__error(AVM_Context *ctx, const char *fmt, ...)
{
  // black magic based on afmt()
  va_list ap;
  va_start(ap, fmt);
  if (vasprintf(&ctx->error, fmt, ap) < 0) {
    ctx->error = NULL;
  }
  va_end(ap);
  return 1;
}
#pragma clang diagnostic pop
