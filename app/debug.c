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

void hexdump(const void *buffer, size_t size) {
  const unsigned char *p = (const unsigned char *)buffer;
  size_t i, j;
  for (i = 0; i < size; i += 16) {
    fprintf(stderr, "%08zx(%p): ", i, i + buffer);
    for (j = 0; j < 16; j++) {
      if (i + j < size)
        fprintf(stderr, "%02x ", p[i + j]);
      else
        fprintf(stderr, "   ");
      if (j % 8 == 7)
        fprintf(stderr, " ");
    }
    fprintf(stderr, " ");
    for (j = 0; j < 16; j++) {
      if (i + j < size) {
        unsigned char c = p[i + j];
        fprintf(stderr, "%c", (c >= 32 && c <= 126) ? c : '.');
      }
    }
    fprintf(stderr, "\n");
  }
}
