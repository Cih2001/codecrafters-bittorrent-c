#include "bencode.h"
#include <assert.h>
#include <openssl/sha.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void hexdump(const void *buffer, size_t size) {
  const unsigned char *p = (const unsigned char *)buffer;
  size_t i, j;
  for (i = 0; i < size; i += 16) {
    printf("%08zx ", i);
    for (j = 0; j < 16; j++) {
      if (i + j < size)
        printf("%02x ", p[i + j]);
      else
        printf("   ");
      if (j % 8 == 7)
        printf(" ");
    }
    printf(" ");
    for (j = 0; j < 16; j++) {
      if (i + j < size) {
        unsigned char c = p[i + j];
        printf("%c", (c >= 32 && c <= 126) ? c : '.');
      }
    }
    printf("\n");
  }
}

int start(int argc, char *argv[]) {
  if (argc < 3) {
    fprintf(stderr, "Usage: your_bittorrent.sh <command> <args>\n");
    return 1;
  }

  const char *command = argv[1];

  if (strcmp(command, "decode") == 0) {
    const char *encoded_str = argv[2];
    bencode *b = decode_bencode(encoded_str);
    bencode_json(b);
    printf("\n");
    bencode_free(b);
    return 0;
  }

  if (strcmp(command, "info") == 0) {
    const char *torrent_file = argv[2];
    FILE *file = fopen(torrent_file, "r");
    if (!file) {
      fprintf(stderr, "Failed to open the file.\n");
      return 1;
    }

    fseek(file, 0, SEEK_END);
    long file_size = ftell(file);
    fseek(file, 0, SEEK_SET);
    unsigned char *torrent = (unsigned char *)malloc(file_size);
    assert(torrent != NULL);

    size_t bytes_read = fread(torrent, 1, file_size, file);
    if (bytes_read != file_size) {
      fprintf(stderr, "Failed to read file.\n");
      free(torrent);
      fclose(file);
      return 1;
    }

    fclose(file);

    bencode *root = decode_bencode((const char *)torrent);
    assert(root != NULL);
    bencode *annouce = bencode_key(root, "announce");
    assert(annouce != NULL);
    bencode *info = bencode_key(root, "info");
    assert(info != NULL);
    bencode *length = bencode_key(info, "length");
    assert(length != NULL);

    const int BUFFER_SIZE = 0x200;
    char buffer[BUFFER_SIZE];
    bencode_to_string(annouce, buffer, BUFFER_SIZE);
    printf("Tracker URL: %s\n", buffer);

    memset(buffer, 0, BUFFER_SIZE);
    bencode_to_string(length, buffer, BUFFER_SIZE);
    printf("Length: %s\n", buffer);

    memset(buffer, 0, BUFFER_SIZE);
    int n = bencode_print(info, buffer, BUFFER_SIZE);
    fprintf(stderr, "written 0x%x(%d) bytes\n", n, n);

    unsigned char hash[SHA_DIGEST_LENGTH];
    SHA1((const unsigned char *)buffer, n, hash);
    printf("Info Hash: ");
    for (int i = 0; i < SHA_DIGEST_LENGTH; i++) {
      printf("%02x", hash[i]);
    }
    printf("\n");

    bencode_free(root);

    return 0;
  }

  fprintf(stderr, "Unknown command: %s\n", command);
  return 1;
}

int main(int argc, char *argv[]) {
  // fprintf(stderr, "%lu\n", sizeof(char *));
  int errcode = start(argc, argv);
  fflush(stdout);
  return errcode;
}
