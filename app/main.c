#include "bencode.h"
#include "debug.h"
#include "torrent.h"
#include <arpa/inet.h>
#include <assert.h>
#include <curl/curl.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

int start(int argc, char *argv[]) {
  if (argc < 3) {
    fprintf(stderr, "Usage: your_bittorrent.sh <command> <args>\n");
    return 1;
  }

  int opt;
  char *command = NULL;
  char *output_file = NULL;
  char *torrent_file = NULL;
  // Process command-line options
  while ((opt = getopt(argc, argv, "o:")) != -1) {
    switch (opt) {
    case 'o':
      output_file = optarg;
      break;
    default:
      fprintf(stderr, "Usage: %s <command> <args>\n", argv[0]);
      return 1;
    }
  }
  // Access non-option arguments
  if (optind < argc) {
    command = argv[optind++];
  }
  if (optind < argc) {
    torrent_file = argv[optind++];
  }
  // Validate the command and options
  if (command == NULL || torrent_file == NULL) {
    fprintf(stderr, "Usage: %s main.o <cmd> [-o output] file_name\n", argv[0]);
    return 1;
  }

  if (strcmp(command, "download_piece") == 0 && output_file == NULL) {
    fprintf(stderr, "Usage: %s <command> <args>\n", argv[0]);
    return 1;
  }
  // Use the parsed

  if (strcmp(command, "decode") == 0) {
    const char *encoded_str = torrent_file;
    bencode *b = decode_bencode(encoded_str);
    bencode_json(b);
    printf("\n");
    bencode_free(b);
    return 0;
  }

  if (strcmp(command, "info") == 0) {
    THandle h = torrent_open(torrent_file);
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
    THandle h = torrent_open(torrent_file);
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

    if (optind >= argc) {
      fprintf(stderr, "Usage: %s <handshake> <torrent_file> <ip:port>\n",
              argv[0]);
    }
    char *peer_addr = argv[optind++];

    struct in_addr ip;
    assert(inet_aton(strtok(peer_addr, ":"), &ip));

    TPeer peer;
    peer.ip = ip.s_addr;
    peer.port = htons(atoi(strtok(NULL, ":")));

    THandle h = torrent_open(torrent_file);
    assert(h);

    TInfo info = {0};
    assert(torrent_get_info(h, &info) == 0);

    uint8_t peer_id[20];
    assert(torrent_do_handshake(h, peer, info.info_hash, &peer_id) == 0);

    printf("Peer ID: ");
    for (int i = 0; i < 20; i++) {
      printf("%02x", peer_id[i]);
    }
    printf("\n");

    torrent_close(h);
    return 0;
  }

  if (strcmp(command, "download_piece") == 0) {
    THandle h = torrent_open(torrent_file);
    assert(h);

    long buffer_size = 1 << 16;
    unsigned char buffer[buffer_size];
    int n = torrent_download_piece(h, 0, buffer, buffer_size);
    assert(n > 0);

    printf("%s\n", output_file);

    int fd = open(output_file, O_RDWR | O_CREAT, 0644);
    if (fd < 0) {
      perror("error opening output_file");
      return 1;
    }

    write(fd, buffer, n);

    close(fd);

    torrent_close(h);
    return 0;
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
