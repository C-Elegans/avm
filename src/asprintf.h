/* Licensed under BSD-MIT - see LICENSE file for details */
#ifndef ASPRINTF_H
#define ASPRINTF_H

/**
 * afmt - allocate and populate a string with the given format.
 * fmt: printf-style format.
 *
 * This is a simplified asprintf interface.  Returns NULL on error.
 */
char * afmt(const char *fmt, ...);

#include <stdarg.h>

/**
 * vasprintf - vprintf to a dynamically-allocated string.
 * strp: pointer to the string to allocate.
 * fmt: printf-style format.
 *
 * Returns -1 (and leaves strp undefined) on an error.  Otherwise returns
 * number of bytes printed into strp.
 */
int vasprintf(char **strp, const char *fmt, va_list ap);

#endif /* ASPRINTF_H */
