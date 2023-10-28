#include "list.h"
#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

bool is_digit(char c) { return c >= '0' && c <= '9'; }

enum bencode_type {
  BENCODE_INVALID = 0,
  BENCODE_STRING = 1,
  BENCODE_INTEGER = 2,
  BENCODE_LIST = 3
};

enum bencode_type get_bencode_type(const char *bencoded_value) {
  char c = bencoded_value[0];
  if (is_digit(c))
    return BENCODE_STRING;
  if (c == 'i')
    return BENCODE_INTEGER;
  if (c == 'l')
    return BENCODE_LIST;

  return BENCODE_INVALID;
}

typedef struct {
  enum bencode_type type;
  int raw_size;
} bencode;

typedef struct {
  enum bencode_type type;
  int raw_size;
  int length;
  char *value;
} bencode_string;

typedef struct {
  enum bencode_type type;
  int raw_size;
  long int value;
} bencode_integer;

typedef struct {
  enum bencode_type type;
  int raw_size;
  int length;
  bencode **values;
} bencode_list;

void bencode_free(bencode *b) {
  bencode_list *list = NULL;
  switch (b->type) {
  case BENCODE_STRING:
    free(((bencode_string *)b)->value);
    break;
  case BENCODE_LIST:
    list = (bencode_list *)b;
    for (int i = 0; i < list->length; i++) {
      bencode_free(list->values[i]);
    }
  default:
    break;
  }

  free(b);
}

bencode *bencode_invalid() {
  bencode *result = malloc(sizeof(bencode));
  assert(result != NULL);
  result->type = BENCODE_INVALID;
  return result;
}

bencode *decode_string_bencode(const char *bencoded_value) {
  int len = atoi(bencoded_value);
  const char *colon_index = strchr(bencoded_value, ':');
  if (colon_index == NULL) {
    return bencode_invalid();
  }

  const char *start = colon_index + 1;
  char *decoded_str = (char *)malloc(len + 1);
  strncpy(decoded_str, start, len);
  decoded_str[len] = '\0';

  bencode_string *result = NULL;
  result = malloc(sizeof(*result));
  assert(result != NULL);

  result->type = BENCODE_STRING;
  result->length = len;
  result->value = decoded_str;
  result->raw_size = (colon_index - bencoded_value + len + 1);
  return (bencode *)result;
}

bencode *decode_integer_bencode(const char *bencoded_value) {
  const char *e_index = strchr(bencoded_value, 'e');
  int len = e_index - bencoded_value - 1;
  if (len <= 0 || bencoded_value[len + 1] != 'e') {
    return bencode_invalid();
  }

  bencode_integer *result = NULL;
  result = malloc(sizeof(*result));
  assert(result != NULL);

  result->type = BENCODE_INTEGER;
  result->value = atol(bencoded_value + 1);
  result->raw_size = (e_index - bencoded_value + 1);
  return (bencode *)result;
}

bencode *decode_bencode(const char *);

bencode *decode_list_bencode(const char *bencoded_value) {
  const char *e_index = strchr(bencoded_value, 'e');
  if (!e_index) {
    return bencode_invalid();
  }

  bencode_list *result = NULL;
  result = malloc(sizeof(*result));
  assert(result != NULL);

  result->type = BENCODE_LIST;
  result->length = 0;
  result->values = NULL;
  result->raw_size = 2;

  bencode *b;

  const char *cur = bencoded_value + 1;
  while (cur - bencoded_value < strlen(bencoded_value) - 1) {
    if (cur[0] == 'e') {
      return (bencode *)result;
    }

    b = decode_bencode(cur);
    result->length++;
    result->values =
        realloc(result->values, sizeof(bencode *) * result->length);
    assert(result->values != NULL);
    result->values[result->length - 1] = b;
    cur += b->raw_size;
    result->raw_size += b->raw_size;
  }

  // This should not really happen.
  return (bencode *)result;
}

bencode *decode_bencode(const char *bencoded_value) {
  enum bencode_type t = get_bencode_type(bencoded_value);
  if (t == BENCODE_STRING)
    return decode_string_bencode(bencoded_value);
  if (t == BENCODE_INTEGER)
    return decode_integer_bencode(bencoded_value);
  if (t == BENCODE_LIST)
    return decode_list_bencode(bencoded_value);

  fprintf(stderr, "Not supported: %s\n", bencoded_value);
  exit(1);
}

void bencode_print(bencode *b) {
  bencode_list *list = NULL;

  switch (b->type) {
  case BENCODE_STRING:
    printf("\"%s\"", ((bencode_string *)b)->value);
    break;
  case BENCODE_INTEGER:
    printf("%ld", ((bencode_integer *)b)->value);
    break;
  case BENCODE_LIST:
    list = (bencode_list *)b;
    printf("[");
    bencode_list *list = (bencode_list *)b;
    for (int i = 0; i < list->length; i++) {
      bencode_print(list->values[i]);
      if (i < list->length - 1)
        printf(",");
    }
    printf("]");

  default:
    return;
  }
}

int start(int argc, char *argv[]) {
  if (argc < 3) {
    fprintf(stderr, "Usage: your_bittorrent.sh <command> <args>\n");
    return 1;
  }

  const char *command = argv[1];

  if (strcmp(command, "decode") == 0) {
    // You can use print statements as follows for debugging, they'll be visible
    // when running tests. printf("Logs from your program will appear here!\n");

    // Uncomment this block to pass the first stage
    const char *encoded_str = argv[2];
    bencode *b = decode_bencode(encoded_str);
    bencode_print(b);
    printf("\n");
    bencode_free(b);
  } else {
    fprintf(stderr, "Unknown command: %s\n", command);
    return 1;
  }

  return 0;
}

int main(int argc, char *argv[]) { start(argc, argv); }
