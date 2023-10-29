#include "bencode_internal.h"
#include "debug.h"
#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

bool is_digit(char c) { return c >= '0' && c <= '9'; }

enum bencode_type bencode_get_type(const char *bencoded_value) {
  char c = bencoded_value[0];
  if (is_digit(c))
    return BENCODE_STRING;
  if (c == 'i')
    return BENCODE_INTEGER;
  if (c == 'l')
    return BENCODE_LIST;
  if (c == 'd')
    return BENCODE_DICT;

  return BENCODE_INVALID;
}

void bencode_free(bencode *b) {
  switch (b->type) {
  case BENCODE_STRING: {
    free(((bencode_string *)b)->value);
    break;
  }
  case BENCODE_LIST: {
    bencode_list *list = (bencode_list *)b;
    for (int i = 0; i < list->length; i++) {
      bencode_free(list->values[i]);
    }
    break;
  }
  case BENCODE_DICT: {
    bencode_dict *dict = (bencode_dict *)b;
    for (int i = 0; i < dict->length; i++) {
      free(dict->keys[i]);
      bencode_free(dict->values[i]);
    }
    break;
  }
  default:
    break;
  }

  free(b);
}

bencode *bencode_invalid() {
  fprintf(stderr, "invalid bencode");
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

  // should be unreachable code.
  return (bencode *)result;
}

bencode *decode_dict_bencode(const char *bencoded_value) {
  const char *e_index = strchr(bencoded_value, 'e');
  if (!e_index) {
    return bencode_invalid();
  }

  bencode_dict *result = NULL;
  result = malloc(sizeof *result);
  assert(result != NULL);

  result->keys = NULL;
  result->values = NULL;
  result->type = BENCODE_DICT;
  result->raw_size = 2;
  result->length = 0;

  bencode *key = NULL;
  bencode *value = NULL;

  const char *cur = bencoded_value + 1;
  while (cur - bencoded_value < strlen(bencoded_value) - 1) {
    if (cur[0] == 'e') {
      return (bencode *)result;
    }

    // decode the key
    if (bencode_get_type(cur) != BENCODE_STRING) {
      return bencode_invalid();
    }

    key = decode_bencode(cur);
    result->raw_size += key->raw_size;

    cur += key->raw_size;

    // decode the value
    value = decode_bencode(cur);
    result->raw_size += value->raw_size;

    result->length++;

    result->keys = realloc(result->keys, sizeof(char *) * result->length);
    assert(result->keys != NULL);
    result->values =
        realloc(result->values, sizeof(bencode *) * result->length);
    assert(result->values != NULL);

    bencode_string *str = (bencode_string *)key;
    result->keys[result->length - 1] = malloc(str->length + 1);
    strncpy(result->keys[result->length - 1], str->value, str->length);
    result->keys[result->length - 1][str->length] = 0;
    bencode_free(key);

    result->values[result->length - 1] = value;

    cur += value->raw_size;
  }

  // should be unreachable code.
  return (bencode *)result;
}

bencode *decode_bencode(const char *bencoded_value) {
  enum bencode_type t = bencode_get_type(bencoded_value);
  if (t == BENCODE_STRING)
    return decode_string_bencode(bencoded_value);
  if (t == BENCODE_INTEGER)
    return decode_integer_bencode(bencoded_value);
  if (t == BENCODE_LIST)
    return decode_list_bencode(bencoded_value);
  if (t == BENCODE_DICT)
    return decode_dict_bencode(bencoded_value);

  fprintf(stderr, "Not supported: %s\n", bencoded_value);
  exit(1);
}

void bencode_json(bencode *b) {
  switch (b->type) {
  case BENCODE_STRING: {
    printf("\"%s\"", ((bencode_string *)b)->value);
    break;
  }
  case BENCODE_INTEGER: {
    printf("%ld", ((bencode_integer *)b)->value);
    break;
  }
  case BENCODE_LIST: {
    printf("[");
    bencode_list *list = (bencode_list *)b;
    for (int i = 0; i < list->length; i++) {
      bencode_json(list->values[i]);
      if (i < list->length - 1)
        printf(",");
    }
    printf("]");
    break;
  }
  case BENCODE_DICT: {
    printf("{");
    bencode_dict *dict = (bencode_dict *)b;
    for (int i = 0; i < dict->length; i++) {
      printf("\"%s\":", dict->keys[i]);
      bencode_json(dict->values[i]);
      if (i < dict->length - 1)
        printf(",");
    }
    printf("}");
    break;
  }

  default:
    fprintf(stderr, "invalid bencode");
    return;
  }
}

size_t bencode_print(bencode *b, char *buffer, size_t size) {
  switch (b->type) {
  case BENCODE_STRING: {
    bencode_string *s = (bencode_string *)b;
    return snprintf(buffer, size, "%d:%s", s->length, s->value);
  }
  case BENCODE_INTEGER: {
    bencode_integer *i = (bencode_integer *)b;
    return snprintf(buffer, size, "i%lde", i->value);
  }
  case BENCODE_LIST: {
    int n = 0;
    n += snprintf(buffer, size - n, "l");
    bencode_list *list = (bencode_list *)b;
    for (int i = 0; i < list->length; i++) {
      n += bencode_print(list->values[i], buffer + n, size - n);
    }
    n += snprintf(buffer + n, size - n, "e");
    return n;
    break;
  }
  case BENCODE_DICT: {
    int n = 0;
    n += snprintf(buffer, size - n, "d");
    bencode_dict *dict = (bencode_dict *)b;
    for (int i = 0; i < dict->length; i++) {
      n += snprintf(buffer + n, size - n, "%lu:%s", strlen(dict->keys[i]),
                    dict->keys[i]);
#ifdef _DEBUG
      fprintf(stderr, "key[%d]: %s\n", i, dict->keys[i]);
#endif
      n += bencode_print(dict->values[i], buffer + n, size - n);
    }
    n += snprintf(buffer + n, size - n, "e");
    return n;
    break;
  }

  default:
    fprintf(stderr, "invalid bencode");
    return 0;
  }

  return 0;
}

size_t bencode_to_string(bencode *b, char *buffer, size_t size) {
  switch (b->type) {
  case BENCODE_STRING: {
    return snprintf(buffer, size, "%s", ((bencode_string *)b)->value);
    break;
  }
  case BENCODE_INTEGER: {
    return snprintf(buffer, size, "%ld", ((bencode_integer *)b)->value);
    break;
  }

  default:
    fprintf(stderr, "invalid bencode");
    return 0;
  }
}

bencode *bencode_key(bencode *b, const char *key) {
  if (!b || b->type != BENCODE_DICT) {
    return NULL;
  }

  bencode_dict *dict = (bencode_dict *)b;
  for (int i = 0; i < dict->length; i++) {
    if (strcmp(dict->keys[i], key) == 0) {
      return dict->values[i];
    }
  }

  return NULL;
}
