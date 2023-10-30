#include "bencode.h"
#include "debug.h"
#include "torrent_internal.h"
#include <arpa/inet.h>
#include <assert.h>
#include <curl/curl.h>
#include <netinet/in.h>
#include <openssl/sha.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>

int url_encode(char *output, int output_size,
               const unsigned char hash_info[SHA_DIGEST_LENGTH]) {
  if (output_size - 1 < SHA_DIGEST_LENGTH * 3) {
    return 0;
  }
  for (int i = 0; i < SHA_DIGEST_LENGTH; i++) {
    snprintf(output + i * 3, output_size - i * 3, "%%%02x", hash_info[i]);
  }
  return SHA_DIGEST_LENGTH * 3;
}

THandle torrent_open(const char *torrent_file_path) {
  FILE *file = fopen(torrent_file_path, "r");
  if (!file) {
    fprintf(stderr, "Failed to open the file.\n");
    return NULL;
  }

  fseek(file, 0, SEEK_END);
  long file_size = ftell(file);
  fseek(file, 0, SEEK_SET);
  THandle torrent = (unsigned char *)malloc(file_size);
  assert(torrent != NULL);

  size_t bytes_read = fread(torrent, 1, file_size, file);
  if (bytes_read != file_size) {
    fprintf(stderr, "Failed to read file.\n");
    free(torrent);
    fclose(file);
    return NULL;
  }

  fclose(file);
  return torrent;
};

void torrent_close(THandle handle) { free(handle); }

int torrent_get_info(THandle handle, TInfo *result) {
  bencode *root = decode_bencode((const char *)handle);
  assert(root != NULL);
  bencode *annouce = bencode_key(root, "announce");
  assert(annouce != NULL);
  bencode *info = bencode_key(root, "info");
  assert(info != NULL);
  bencode *length = bencode_key(info, "length");
  assert(length != NULL);

  bencode_to_string(annouce, result->tracker, SMALL_BUFFER_SIZE);

  char buffer[SMALL_BUFFER_SIZE] = {0};
  bencode_to_string(length, buffer, SMALL_BUFFER_SIZE);
  result->length = atol(buffer);

  memset(buffer, 0, SMALL_BUFFER_SIZE);
  int n = bencode_print(info, buffer, SMALL_BUFFER_SIZE);

  unsigned char hash[SHA_DIGEST_LENGTH];
  SHA1((const unsigned char *)buffer, n, hash);
  memcpy(result->info_hash, hash, SHA_DIGEST_LENGTH);

  bencode *piece_length = bencode_key(info, "piece length");
  assert(piece_length != NULL);
  memset(buffer, 0, SMALL_BUFFER_SIZE);
  bencode_to_string(piece_length, buffer, SMALL_BUFFER_SIZE);
  result->piece_length = atol(buffer);

  bencode *pieces = bencode_key(info, "pieces");
  assert(pieces != NULL);

  char b_pieces[LARGE_BUFFER_SIZE] = {0};
  n = bencode_to_string(pieces, b_pieces, LARGE_BUFFER_SIZE);
  for (int i = 0; i * SHA_DIGEST_LENGTH < n; i++) {
    result->no_of_piece_hashes = i + 1;
    int size = result->no_of_piece_hashes * sizeof(*result->pieces);
    result->pieces = realloc(result->pieces, size);
    memcpy(result->pieces[i], &b_pieces[i * SHA_DIGEST_LENGTH],
           SHA_DIGEST_LENGTH);
  }

  bencode_free(root);

  return 0;
}

size_t get_peers_callback(char *ptr, size_t size, size_t nmemb,
                          void *userdata) {
  size_t total_size = size * nmemb;
  memcpy(userdata, ptr, total_size);
  return total_size;
}

int torrent_get_peers(THandle handle, TPeers *result) {
  TInfo t = {0};
  if (torrent_get_info(handle, &t) == -1) {
    return -1;
  }
  free(t.pieces);

  char info_hash[SMALL_BUFFER_SIZE] = {0};
  url_encode(info_hash, SMALL_BUFFER_SIZE, t.info_hash);

  char url[LARGE_BUFFER_SIZE] = {0};
  snprintf(url, LARGE_BUFFER_SIZE,
           "%s?info_hash=%s&peer_id=00112233445566778899&port=6881&uploaded="
           "0&downloaded=0&left=%lu&compact=1",
           t.tracker, info_hash, t.length);

  char response[LARGE_BUFFER_SIZE] = {0};
  CURL *curl;
  CURLcode res;
  curl = curl_easy_init();
  if (curl == NULL) {
    fprintf(stderr, "curl init failed: %s\n", curl_easy_strerror(res));
    return -1;
  };

  curl_easy_setopt(curl, CURLOPT_URL, url);
  curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, get_peers_callback);
  curl_easy_setopt(curl, CURLOPT_WRITEDATA, response);
  curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
  // Set the buffer as the user data to be passed to the callback function
  res = curl_easy_perform(curl);
  if (res != CURLE_OK) {
    fprintf(stderr, "request failed: %s\n", curl_easy_strerror(res));
    return -1;
  }

  curl_easy_cleanup(curl);

  bencode *root = decode_bencode(response);
  bencode *peers = bencode_key(root, "peers");
  if (peers == NULL) {
    fprintf(stderr, "peers key not found in response\n");
    return -1;
  }

  unsigned char b_peers[SMALL_BUFFER_SIZE] = {0};
  int n = bencode_to_string(peers, (char *)b_peers, SMALL_BUFFER_SIZE);

  int count = (n / 6);
  int size = count * sizeof(*result[0]);
  *result = realloc(*result, size);
  assert(*result);

  hexdump(b_peers, count * 6);
  for (int i = 0; i < count; i++) {
    memcpy(&(*result)[i].ip, &b_peers[i * 6], 4);
    memcpy(&(*result)[i].port, &b_peers[i * 6 + 4], 2);
  }

  bencode_free(root);

  return count;
}

int torrent_do_handshake(THandle handle, TPeer peer,
                         uint8_t info_hash[SHA_DIGEST_LENGTH]) {
  struct sockaddr_in server_addr;
  char buffer[10];
  int sockfd = socket(AF_INET, SOCK_STREAM, 0);
  if (sockfd == -1) {
    perror("socket");
    exit(1);
  }
  // Set up the server address
  server_addr.sin_family = AF_INET;
  server_addr.sin_port = peer.port;
  server_addr.sin_addr.s_addr = peer.ip;

  if (connect(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) ==
      -1) {
    fprintf(stderr, "error connecting to peer\n");
    return -1;
  }

  struct {
    uint8_t size;
    uint8_t message[19];
    uint8_t hash[SHA_DIGEST_LENGTH];
    uint8_t peer_id[20];
    uint64_t reserved;
  } handshake, ack = {0};

  handshake.size = 19;
  memcpy(&handshake.message, "BitTorrent protocol", 19);
  memcpy(&handshake.hash, info_hash, SHA_DIGEST_LENGTH);
  memcpy(&handshake.peer_id, "00112233445566778899", 20);

  int n = send(sockfd, &handshake, sizeof(handshake), 0);
  assert(n == sizeof(handshake));
  fprintf(stderr, "wrote %d bytes\n", n);

  n = recv(sockfd, &ack, sizeof(ack), 0);
  if (n == -1) {
    fprintf(stderr, "error reading peer handshake");
    return -1;
  }

  hexdump(&ack, sizeof(ack));
  return 0;
}
