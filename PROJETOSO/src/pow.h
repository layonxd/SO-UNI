

/* pow.h */
#ifndef POW_H
#define POW_H
//#define HASH_SIZE SHA256_DIGEST_LENGTH


#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <time.h>
#include <signal.h>
#include <sys/shm.h>
#include <sys/msg.h>
#include <fcntl.h>
#include <semaphore.h>
#include <pthread.h>
#include <sys/wait.h>
#include <openssl/sha.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/sem.h>  // NEW: Added for semaphores
#include "common.h"
#include <openssl/sha.h>

#define POW_MAX_OPS 10000000

#define INITIAL_HASH \
  "00006a8e76f31ba74e21a092cca1015a418c9d5f4375e7a4fec676e1d2ec1436"

#ifndef DEBUG
#define DEBUG 0
#endif

/* Definition of Difficulty Levels */
typedef enum { EASY = 1, NORMAL = 2, HARD = 3 } DifficultyLevel;

typedef struct {
  char hash[HASH_SIZE];
  double elapsed_time;
  int operations;
  int error;
} PoWResult;

void compute_sha256(const Block *input, char *output);
PoWResult proof_of_work(Block *block);
int verify_nonce(const Block *block);
int check_difficulty(const char *hash, const int reward);
DifficultyLevel getDifficultFromReward(const int reward);



Block generate_random_block(const char *prev_hash,
                                       int block_number,int transactions_per_block);
Transaction generate_random_transaction(int tx_number);



#endif
/* POW_H */