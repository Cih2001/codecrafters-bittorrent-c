#ifndef DEBUG_H__
#define DEBUG_H__

#include <stdlib.h>

void *f_debug_mem_malloc(unsigned long size, char *file, unsigned long line);
void *f_debug_mem_realloc(void *ptr, unsigned long size, char *file,
                          unsigned long line);

#ifdef _DEBUG
#define malloc(m) f_debug_mem_malloc(m, __FILE__, __LINE__)
#define realloc(m, n) f_debug_mem_realloc(m, n, __FILE__, __LINE__)
#else
#endif /* _DEBUG */

#endif /* DEBUG_H__ */
