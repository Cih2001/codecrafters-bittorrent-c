#ifndef TORRENT_INTERNAL_H__
#define TORRENT_INTERNAL_H__

#include <openssl/sha.h>
#include <stdint.h>

typedef struct {
  int socketfd;
  char *torrent_file;
} *THandle;

enum message_ids {
  MSG_UNCHOCK = 1,
  MSG_INTERESTED = 2,
  MSG_BITFIELD = 5,
  MSG_REQUEST = 6,
  MSG_PIECE = 7,
};

typedef struct __attribute__((packed)) {
  uint32_t length;
  uint8_t id;
  uint8_t payload[];
} peer_message;

typedef struct {
  uint32_t index;
  uint32_t begin;
  uint32_t length;
} piece_request;

typedef struct {
  uint32_t index;
  uint32_t begin;
  uint32_t data[];
} piece_response;

// ltob converts a number presented in little endian to
// its big endian representation.
uint32_t ltob(uint32_t n);

// url_encode encodes a hashinfo to its url encoded form.
int url_encode(char *output, int output_size,
               const unsigned char hash_info[SHA_DIGEST_LENGTH]);

#include "torrent.h"

#endif /* TORRENT_INTERNAL_H__ */
