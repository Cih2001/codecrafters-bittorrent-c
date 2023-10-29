
#include <stdio.h>
#include <stdlib.h>

void *f_debug_mem_malloc(unsigned long size, char *file, unsigned int line) {
  fprintf(stderr, "%s: %d: allocating memory of size %lu\n", file, line, size);
  return malloc(size);
}

void *f_debug_mem_realloc(void *ptr, unsigned long size, char *file,
                          unsigned int line) {
  fprintf(stderr, "%s: %d: reallocating memory of size %lu\n", file, line,
          size);
  return realloc(ptr, size);
}
