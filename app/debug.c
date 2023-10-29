#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void *f_debug_mem_malloc(unsigned long size, char *file, unsigned int line) {
  void *result = malloc(size);
  if (result == NULL) {
    fprintf(stderr, "%s %d: malloc failed: %s\n", file, line, strerror(errno));
    return NULL;
  }
  fprintf(stderr, "%s %d: malloc(%lu) = %p \n", file, line, size, result);
  return result;
}

void *f_debug_mem_realloc(void *ptr, unsigned long size, char *file,
                          unsigned int line) {
  void *result = realloc(ptr, size);
  if (result == NULL) {
    fprintf(stderr, "%s %d: realloc failed: %s\n", file, line, strerror(errno));
    return NULL;
  }
  fprintf(stderr, "%s %d: realloc(%p, %lu) = %p\n", file, line, ptr, size,
          result);
  return result;
}
