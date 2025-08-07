//Pedro Dioniso Mendes 2021218522
//Rodrigo Rodrigues 2021217624




#ifndef COMMON_H
#define COMMON_H

#define SHM_KEY 0x1234//?
#define SEM_KEY 0x1235  
//extern pthread_mutex_t pool_mutex;//?
#define BLOCKCHAIN_KEY 0x1236 
#define HASH_SIZE 65  // SHA256_DIGEST_LENGTH * 2 + 1
#include <signal.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <time.h>
#include <sys/shm.h>
#include <sys/msg.h>
#include <fcntl.h>
#include <semaphore.h>
#include <pthread.h>
#include <sys/wait.h>
#include <openssl/sha.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/sem.h>  

#include <openssl/sha.h>


#define POW_MAX_OPS 10000000

//------------------------------
#define MSGQ_KEY 0x1237
#define MAX_MINERS 100

typedef enum {
    MSG_VALID_BLOCK,
    MSG_INVALID_BLOCK
} MessageType;

typedef struct {
    long mtype;  
    MessageType msg_type;
    int miner_id;
    int credits_earned;
    double verification_time;
} StatsMessage;

typedef struct {
    int valid_blocks[MAX_MINERS];
    int invalid_blocks[MAX_MINERS];
    int total_credits[MAX_MINERS];
    double total_verification_time;
    int verification_count;
    int total_blocks_validated;
    pthread_mutex_t mutex;
} Statistics;

void handle_sigusr1(int sig);
void statistics_process();
void print_statistics();

//------------------------------












typedef struct {
    int num_miners;
    int pool_size;
    int transactions_per_block;
    int blockchain_blocks;
} Config;

extern Config config;

// Estruturas de dados principais
typedef struct {
    int id;//pid do generator + counter
    int value;// 1 a 3 (++quando fica mod 50 , 50 100 150 etc...)
    int sender_id;//pid of process
    int receiver_id;//random id
    int quant;//quantidade???
    time_t timestamp;//quando foi criado , bom para verificar com o tempo de criacao do bloco
    int age;//aumenta quando validator toca na transaction pool
} Transaction;

typedef struct {
    Transaction* transactions;
    int blockid;
    int size;//SIZE ATUAL
    int capacity;//tx_pool_size (config)
    pthread_mutex_t mutex;  // Replace sem_id with mutex
} TransactionPool;//processos pendentes?

extern TransactionPool* tx_pool;
/*
PARAMETROS DA TRANSACTION POOl
current_block_id: holds the value of the current block (TXB_ID), enabling miners to
utilize it in the mining process.
○ transactions_pending_set: Current transactions awaiting validation. The Transaction
Pool size defines the number of elements in this structure. It is defined in the
configuration file by the variable TX_POOL_SIZE.
○ Each entry on this set must have the following:
■ empty: field indicating whether the position is available or not
■ age: field that starts with zero and is incremented every time the Validator
touches the Transaction Pool*/




typedef struct Block {
    int id;
    Transaction* transactions;
    time_t timestamp;
    int nonce;
    char previous_hash[HASH_SIZE]; // Hash of previous block
} Block;

typedef struct {
    Block* blocks;
    int size;//size atual
    int capacity;//ta na config (BLOCKCHAIN_BLOCKS)
    pthread_mutex_t mutex;
} Blockchain;
extern Blockchain* blockchain;
// Protótipos de funções
Config read_config(const char* filename);
void log_message(const char* message);
void *miner(void* idp);


//txgen
Transaction create_transaction(int counter,int value);
int add_transaction_to_pool(Transaction tx);
void init_transaction_pool(int capacity);
void gen();
TransactionPool* attach_to_existing_pool();//P?
int get_transactions_from_pool(Transaction* dest, int count);//P?


Block make_block(Transaction* transactions, int pow_value);

#define VALIDATOR_PIPE_NAME "VALIDATOR_INPUT"

void start_validator_process() ;

void validator() ;

typedef struct {
    Block block;
    int miner_id;
    int tx_ids[15];//cd
} BlockSubmission;

void delete_from_pool(int ids[],int num_got);
int delete_block_from_pool(BlockSubmission *submission, int num_transactions,int pool_size);


void init_blockchain(int capacity);
Blockchain* attach_to_existing_blockchain();
void add_block_to_chain(Block block);


char* get_last_block_hash();

int get_next_block_id();

extern volatile sig_atomic_t shutdown_requested;
extern void handle_sigint(int sig);

void cleanup();

void lock_chain();
void unlock_chain();
void lock_pool();
void unlock_pool();
int get_reward(BlockSubmission *submission, int num_transactions, int pool_size);

void* monitor_validators(void* arg);

#endif