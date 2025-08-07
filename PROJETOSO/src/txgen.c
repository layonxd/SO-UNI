//Pedro Dioniso Mendes 2021218522
//Rodrigo Rodrigues 2021217624




#include "common.h"
#include "pow.h"
TransactionPool* tx_pool = NULL;

void lock_pool() {
    pthread_mutex_lock(&tx_pool->mutex);
}

void unlock_pool() {
    pthread_mutex_unlock(&tx_pool->mutex);
}

void lock_chain() {
    pthread_mutex_lock(&blockchain->mutex);
}

void unlock_chain() {
    pthread_mutex_unlock(&blockchain->mutex);
}

Transaction create_transaction(int counter, int value) {
    Transaction tx;
    tx.id = getpid() + counter;
    tx.value = value;
    tx.sender_id = getpid();
    tx.receiver_id = rand();
    tx.quant = (rand() % 10) + 1;
    tx.timestamp = time(NULL);
    tx.age = 0;
    return tx;
}

void init_transaction_pool(int capacity) {
    size_t total_size = sizeof(TransactionPool) + (capacity * sizeof(Transaction));
    int shmid = shmget(SHM_KEY, total_size, IPC_CREAT | 0666);
    if (shmid == -1) {
        perror("shmget failed");
        exit(EXIT_FAILURE);
    }

    tx_pool = (TransactionPool*)shmat(shmid, NULL, 0);
    if (tx_pool == (void*)-1) {
        perror("shmat failed");
        exit(EXIT_FAILURE);
    }

    


    pthread_mutexattr_t attr;
    pthread_mutexattr_init(&attr);
    pthread_mutexattr_setpshared(&attr, PTHREAD_PROCESS_SHARED);
    pthread_mutex_init(&tx_pool->mutex, &attr);
    pthread_mutexattr_destroy(&attr);


    tx_pool->transactions = (Transaction*)((char*)tx_pool + sizeof(TransactionPool));
    tx_pool->size = 0;
    tx_pool->capacity = capacity;
    tx_pool->blockid = 0;

    for (int i = 0; i < capacity; i++) {
        tx_pool->transactions[i].id = -1; 
    }

    
}

TransactionPool* attach_to_existing_pool() {
    
    int shmid = shmget(SHM_KEY, 0, 0666);
    if (shmid == -1) {
        char error_msg[256];
        snprintf(error_msg, sizeof(error_msg), 
               "Failed to locate shared memory (key=0x%x): %s",
               SHM_KEY, strerror(errno));
        log_message(error_msg);
        return NULL;
    }

    TransactionPool* pool = (TransactionPool*)shmat(shmid, NULL, 0);
    if (pool == (void*)-1) {
        char error_msg[256];
        snprintf(error_msg, sizeof(error_msg),
               "shmat failed: %s", strerror(errno));
        log_message(error_msg);
        return NULL;
    }

    
    pool->transactions = (Transaction*)((char*)pool + sizeof(TransactionPool));

    if (pool->capacity <= 0 || pool->capacity > 100000) {
        log_message("Warning: Pool capacity value looks invalid");
    }

    char log_msg[256];
    snprintf(log_msg, sizeof(log_msg),
           "Attached to existing pool at %p (capacity=%d)",
           pool, pool->capacity);
    log_message(log_msg);

    return pool;
}

int add_transaction_to_pool(Transaction tx) {
    lock_pool();
    
 
    Transaction* transactions = (Transaction*)((char*)tx_pool + sizeof(TransactionPool));

    if (tx_pool->size >= tx_pool->capacity) {
        unlock_pool();
        return -1;
    }

    for (int i = 0; i < tx_pool->capacity; i++) {
        if (transactions[i].id == -1) {
            transactions[i] = tx;
            tx_pool->size++;
            unlock_pool();
            return 0;
        }
    }

    unlock_pool();
    return -1;
}

int get_transactions_from_pool(Transaction* dest, int count) {

    lock_pool();
    
    Transaction* transactions = (Transaction*)((char*)tx_pool + sizeof(TransactionPool));
    int collected = 0;

    if (tx_pool->size >= count) {
        for (int i = 0; i < tx_pool->capacity && collected < count; i++) {

            if (transactions[i].id != -1) {
              
                int is_duplicate = 0;
                for (int j = 0; j < collected; j++) {
                    if (dest[j].id == transactions[i].id) {
                        is_duplicate = 1;
                        break;
                    }
                }
                
                if (!is_duplicate) {
                    dest[collected++] = transactions[i];
                    
                }
            }
        }
        usleep(100000);
    }

    unlock_pool();
    return collected; 
}



void delete_from_pool(int ids[], int num_got) {
    if (tx_pool == NULL) {
        log_message("delete_from_pool: ERROR: tx_pool is NULL");
        return;
    }
    lock_pool();
    Transaction* pool_transactions = tx_pool->transactions;
    int removed = 0;
    char log_msg[256];
    


    for (int j = 0; j < tx_pool->capacity; j++) {

        for (int i = 0; i < num_got; i++) {
            if (pool_transactions[j].id != -1){
                if (pool_transactions[j].id == ids[i]) {
                    pool_transactions[j].id = -1;  
                    tx_pool->size--;
                    removed++;
                    break; 
                }
            }
        }

        
        if (pool_transactions[j].id != -1) {
            pool_transactions[j].age++;
        }
    }

    
    snprintf(log_msg, sizeof(log_msg),
             "Removed %d transactions (new size: %d)",
             removed, tx_pool->size);
    log_message(log_msg);

    unlock_pool();

    
}


int delete_block_from_pool(BlockSubmission *submission, int num_transactions, int pool_size) {
    if (tx_pool == NULL || submission == NULL || num_transactions <= 0) {
        log_message("delete_block_from_pool: Invalid parameters");
        return 0;
    }

    lock_pool();
   
    Transaction* pool_transactions = (Transaction*)((char*)tx_pool + sizeof(TransactionPool));
    int removed_count = 0;
    char log_msg[256];

  
    for (int i = 0; i < pool_size; i++) {
      
        
        if (pool_transactions[i].id == -1) continue;
        
       
        for (int j = 0; j < num_transactions; j++) {
            if (pool_transactions[i].id == submission->tx_ids[j]) {
                
                pool_transactions[i].id = -1;
                tx_pool->size--;
                removed_count++;
                break; 
            }
        }
        
        
        if (pool_transactions[i].id != -1) {
            pool_transactions[i].age++;
        }
    }

    snprintf(log_msg, sizeof(log_msg),
             "Removed %d/%d transactions (pool size now: %d)",
             removed_count, num_transactions, tx_pool->size);
    log_message(log_msg);

    unlock_pool();
    return removed_count;
}

int get_reward(BlockSubmission *submission, int num_transactions, int pool_size) {
    if (tx_pool == NULL || submission == NULL || num_transactions <= 0) {
        log_message("delete_block_from_pool: Invalid parameters");
        return 0;
    }

    lock_pool();
   
    Transaction* pool_transactions = (Transaction*)((char*)tx_pool + sizeof(TransactionPool));
    int reward = 0;
    char log_msg[256];

    
    for (int i = 0; i < pool_size; i++) {
      
        
        if (pool_transactions[i].id == -1) continue;
        
        
        for (int j = 0; j < num_transactions; j++) {
            if (pool_transactions[i].id == submission->tx_ids[j]) {
                
                reward=reward+pool_transactions[i].value;
                break; 
            }
        }
    }

    snprintf(log_msg, sizeof(log_msg),
             "reward %d from %d transactions ",
             reward, num_transactions);
    log_message(log_msg);

    unlock_pool();
    return reward;
}



void validator() {
    log_message("Validator: Starting validator process");

    signal(SIGINT, SIG_DFL);

    if (mkfifo(VALIDATOR_PIPE_NAME, 0666) == -1) {
        if (errno != EEXIST) {
            perror("mkfifo failed");
            exit(EXIT_FAILURE);
        }
    }
    
    int pipe_fd = open(VALIDATOR_PIPE_NAME, O_RDONLY);
    if (pipe_fd == -1) {
        perror("open pipe failed");
        exit(EXIT_FAILURE);
    }

    config = read_config("config.cfg");
    printf("Validator: Pool size from config = %d\n", config.pool_size);

  

    while (!shutdown_requested) {
        BlockSubmission submission;
        printf("Validator: Waiting for block submission...\n");
        ssize_t bytes_read = read(pipe_fd, &submission, sizeof(submission));
        
        if (bytes_read == sizeof(submission)) {
            char log_msg[256];
            snprintf(log_msg, sizeof(log_msg),
                   "Validator: Received block from miner %d (PoW: %d)",
                   submission.miner_id, submission.block.nonce);
            log_message(log_msg);


           



            int valid = 0;
            lock_chain();
            printf("\n%d=%d\n", submission.block.id,blockchain->size);
            if (blockchain->size == (submission.block.id)){
                valid=1;
            }
            else{
                valid = 0;
            }
            unlock_chain();
            
            //int valid = verify_nonce(&submission.block);
            //erro no validator a ler as cenas
            if (valid) {
                log_message("Validator: Block VALID");

                lock_chain();
                if (blockchain->size < blockchain->capacity) {
                    blockchain->blocks[blockchain->size++] = submission.block;

                    //Transaction* block_transactions = (&(submission.block))->transactions; 
                    int total_reward=0; 
                    total_reward = get_reward(&submission, config.transactions_per_block,config.pool_size);
                    
                    
                    
                    int msgqid = msgget(MSGQ_KEY, 0666);
                    if (msgqid != -1) {
                        StatsMessage msg;
                        msg.mtype = submission.miner_id;  
                        msg.msg_type = MSG_VALID_BLOCK;
                        msg.miner_id = submission.miner_id;
                        msg.credits_earned = total_reward;
                        msg.verification_time = difftime(time(NULL), submission.block.timestamp);//considerar start_time em vez de submission.block.timestamp
                        
                        if (msgsnd(msgqid, &msg, sizeof(msg) - sizeof(long), 0) == -1) {
                            perror("msgsnd failed");
                        }
                    }






                    




                }
                else {
                    log_message("Validator: Blockchain is full, cannot add block");
                    kill(getppid(), SIGINT);
                    //sleep(1);
  

                }
                unlock_chain();

                log_message("ADDED TO CHAIN , NOW DELETING FROM POOL");
                delete_block_from_pool(&submission, config.transactions_per_block,config.pool_size);


            }
            else {
                log_message("Validator: Block INVALID");



                int msgqid = msgget(MSGQ_KEY, 0666);
                if (msgqid != -1) {
                    StatsMessage msg;
                    msg.mtype = submission.miner_id + 1;
                    msg.msg_type = MSG_INVALID_BLOCK;
                    msg.miner_id = submission.miner_id;
                    
                    if (msgsnd(msgqid, &msg, sizeof(msg) - sizeof(long), 0) == -1) {
                        perror("msgsnd failed");
                    }}










            }
            
                
        } 
        
        else if (bytes_read == -1) {
            perror("read error");
            break;
            
        }
        sleep(3);//temp
    }

    close(pipe_fd);
    log_message("Validator: Shutting down");
    exit(0);
}




char* get_last_block_hash() {
    static char last_hash[HASH_SIZE];
    
    lock_chain();
    compute_sha256(&blockchain->blocks[blockchain->size - 1], last_hash);

   
    
    unlock_chain();
    
    return last_hash;
}

int get_next_block_id() {
    int next_id = 0; 
    
    lock_chain();
    
    if (blockchain != NULL && blockchain->size > 0) {
        next_id = blockchain->blocks[blockchain->size - 1].id + 1;
    }
    
    unlock_chain();
    
    return next_id;
}