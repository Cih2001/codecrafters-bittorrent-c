#include "bencode.h"
#include "debug.h"
#include "torrent.h"
#include <arpa/inet.h>
#include <assert.h>
#include <curl/curl.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <string.h>

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
    THandle h = torrent_open(argv[2]);
    assert(h);

    TInfo t = {0};
    int result = torrent_get_info(h, &t);
    if (result != 0) {
      torrent_close(h);
      return result;
    }

    printf("Tracker URL: %s\n", t.tracker);
    printf("Length: %lu\n", t.length);
    printf("Info Hash: ");
    for (int i = 0; i < sizeof(t.info_hash); i++) {
      printf("%02x", t.info_hash[i]);
    }
    printf("\nPiece Length: %lu\n", t.piece_length);
    printf("Piece Hashes:\n");

    for (int i = 0; i < t.no_of_piece_hashes; i++) {
      for (int j = 0; j < sizeof(t.pieces[i]); j++) {
        printf("%02x", t.pieces[i][j]);
      }
      printf("\n");
    }

    free(t.pieces);
    torrent_close(h);

    return result;
  }

  if (strcmp(command, "peers") == 0) {
    THandle h = torrent_open(argv[2]);
    assert(h);

    TPeers peers = NULL;
    int n = torrent_get_peers(h, &peers);
    if (n == -1) {
      return 1;
    }

    for (int i = 0; i < n; i++) {
      struct in_addr addr;
      addr.s_addr = peers[i].ip;
      printf("%s:%d\n", inet_ntoa(addr), ntohs(peers[i].port));
    }

    torrent_close(h);
    return 0;
  }

  if (strcmp(command, "handshake") == 0) {
    if (argc != 4) {
      fprintf(stderr, "invalid number of arguments\n");
      return 1;
    }

    struct in_addr ip;
    assert(inet_aton(strtok(argv[3], ":"), &ip));

    TPeer peer;
    peer.ip = ip.s_addr;
    peer.port = htons(atoi(strtok(NULL, ":")));

    THandle h = torrent_open(argv[2]);
    assert(h);

    TInfo info = {0};
    assert(torrent_get_info(h, &info) == 0);

    torrent_do_handshake(h, peer, info.info_hash);

    torrent_close(h);
    return 1;
  }
  fprintf(stderr, "Unknown command: %s\n", command);
  return 1;
}

int main(int argc, char *argv[]) {
  // fprintf(stderr, "%lu\n", sizeof(char *));
  int errcode = start(argc, argv);
  fflush(stderr);
  fflush(stdout);
  return errcode;
}
