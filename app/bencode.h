#ifndef BENCODE_H__
#define BENCODE_H__

#ifndef BENCODE_INTERNAL_H__
typedef void bencode;
#endif

#include "stddef.h"

bencode *decode_bencode(const char *bencoded_value);
void bencode_free(bencode *b);

size_t bencode_to_string(bencode *b, char *buffer, size_t size);
void bencode_json(bencode *b);
size_t bencode_print(bencode *b, char *buffer, size_t size);

// bencode key returns the value associated to a key in bencode dict.
// in case of errors, it will return NULL.
bencode *bencode_key(bencode *b, const char *key);

#endif /* BENCODE_H__*/
