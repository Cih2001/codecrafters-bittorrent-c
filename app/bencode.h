#ifndef BENCODE_H__
#define BENCODE_H__

#ifndef BENCODE_INTERNAL_H__
typedef void bencode;
#endif

bencode *decode_bencode(const char *bencoded_value);
void bencode_free(bencode *b);
void bencode_print(bencode *b);

#endif /* BENCODE_H__*/
