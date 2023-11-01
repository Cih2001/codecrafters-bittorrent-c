#ifndef TORRENT_H__
#define TORRENT_H__

#define LARGE_BUFFER_SIZE 0x500
#define SMALL_BUFFER_SIZE 0x200
#define PIECE_BUFFER_SIZE 1 << 15

#include <openssl/sha.h>

typedef struct {
  char tracker[SMALL_BUFFER_SIZE];
  unsigned long length;
  unsigned char info_hash[SHA_DIGEST_LENGTH];
  unsigned long piece_length;
  int no_of_piece_hashes;
  unsigned char (*pieces)[SHA_DIGEST_LENGTH];
} TInfo;

#ifndef TORRENT_INTERNAL_H__
typedef void *THandle;
#endif

THandle torrent_open(const char *torrent_file_path);
void torrent_close(THandle);

int torrent_get_info(THandle handle, TInfo *result);

typedef struct {
  uint32_t ip;
  uint16_t port;
  uint16_t unussed;
} TPeer;

typedef TPeer *TPeers;

/*
 * torrent_get_peers returns fetches and returns the peers list
 *
 * the return value is -1 in case of errors or the number of peers
 * in the list in case of success.
 */
int torrent_get_peers(THandle handle, TPeers *result);

/*
 * torrent_do_handshake performs a handshake with a peer and sets
 * the socket descriptor on THandle for further communications.
 *
 * In case of any error, it will return -1.
 */
int torrent_do_handshake(THandle handle, TPeer peer,
                         uint8_t info_hash[SHA_DIGEST_LENGTH],
                         uint8_t (*peer_id)[20]);

/*
 * torrent_declare_interest declares interest to a peer for downlaoding
 * data right after handshake. This function needs to be called after handshake.
 *
 * In case of any error, it will return -1.
 */
int torrent_declare_interest(THandle handle);

/*
 * torrent_download_piece downloads a piece in a torrent file and
 * returns the number of bytes download. It internally verifies the
 * hash of the downloaded data.
 *
 * this function has to be called after a handshake and interest declaration.
 *
 * In case of any error, it will return -1.
 */
int torrent_download_piece(THandle handle, TPeer peer, int index,
                           unsigned char *output, unsigned long output_size);

/*
 * torrent_download downloads a torrent file and returns the number of
 * bytes download. It internally verifies the hash of the downloaded data.
 *
 * this function has to be called after a handshake and interest declaration.
 *
 * In case of any error, it will return -1.
 */
int torrent_download(THandle handle, unsigned char *output,
                     unsigned long output_size);

#endif /* TORRENT_H__ */
