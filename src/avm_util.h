#ifndef _AVM_UTIL_
#define _AVM_UTIL_
#include "asprintf.h"
#include <stdio.h>

void *my_malloc(size_t size);
void *my_calloc(size_t count, size_t size);
void *my_crealloc(void *buffer, size_t oldsize, size_t newsize);
void *my_realloc(void *buffer, size_t newsize);
#define my_free(p)  { free(p); p = NULL; }

size_t min(size_t a, size_t b);

/* Reads the file until error or EOF
 */
char *read_file(FILE *file, size_t *len);

#endif
