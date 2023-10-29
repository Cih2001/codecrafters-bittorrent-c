#ifndef BENCODE_INTERNAL_H__
#define BENCODE_INTERNAL_H__

enum bencode_type {
  BENCODE_INVALID = 0,
  BENCODE_STRING = 1,
  BENCODE_INTEGER = 2,
  BENCODE_LIST = 3,
  BENCODE_DICT = 4
};

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

typedef struct {
  enum bencode_type type;
  int raw_size;
  int length;
  char **keys;
  bencode **values;
} bencode_dict;

bencode *bencode_invalid();
bencode *decode_string_bencode(const char *bencoded_value);
bencode *decode_integer_bencode(const char *bencoded_value);
bencode *decode_list_bencode(const char *bencoded_value);
bencode *decode_dict_bencode(const char *bencoded_value);

#include "bencode.h"

#endif /* BENCODE_INTERNAL_H_ */
