#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "avm_util.h"


void* my_crealloc(void* buffer, size_t oldsize, size_t newsize) {
  void* result = realloc(buffer, newsize);

  if(newsize > oldsize && result != NULL) {
    size_t grow_len = newsize - oldsize;
    memset(((char*) result) + oldsize, 0, grow_len);
  }

  return result;
}

void* my_malloc(size_t size) { return malloc(size); }
void* my_calloc(size_t count, size_t size) { return calloc(count, size); }
