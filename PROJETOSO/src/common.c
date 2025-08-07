//Pedro Dioniso Mendes 2021218522
//Rodrigo Rodrigues 2021217624


#include "common.h"
//#include "txgen.c"
#include "pow.h"

Config config;


Config read_config(const char* filename) {
    Config cfg = {0};
    
    FILE* file = fopen(filename, "r");
    if (!file) {
        log_message("Controller: Erro ao abrir arquivo de configuração");
        perror("fopen");
        exit(EXIT_FAILURE);
    }
    
    if (fscanf(file, "%d %d %d %d", 
              &cfg.num_miners,
              &cfg.pool_size,
              &cfg.transactions_per_block,
              &cfg.blockchain_blocks) != 4) {
        log_message("Controller: Formato inválido do arquivo de configuração");
        fclose(file);
        exit(EXIT_FAILURE);
    }
    
    fclose(file);
    

    char log_msg[256];
    snprintf(log_msg, sizeof(log_msg),
             "Configuração carregada: Miners=%d, Pool=%d, Tx/Bloco=%d, Blocos=%d",
             cfg.num_miners, cfg.pool_size, 
             cfg.transactions_per_block, cfg.blockchain_blocks);

    log_message(log_msg);
    
    return cfg;
}


//-----------------------------


volatile sig_atomic_t shutdown_requested = 0;

void handle_sigint(int sig) {
    (void)sig;
    shutdown_requested = 1;
    log_message("Controller: Received SIGINT, initiating shutdown...");
}


//-----------------------------


void cleanup() {
    shmdt(tx_pool);
    shmdt(blockchain);
    exit(0);
}




void log_message(const char* message) {
    time_t now = time(NULL);
    char time_str[20];
    strftime(time_str, sizeof(time_str), "%Y-%m-%d %H:%M:%S", localtime(&now));

    // Escreve no console
    printf("[%s] %s\n", time_str, message);

    // Escreve no arquivo (append)
    FILE* log_file = fopen("DEIChain_log.txt", "a");
    if (log_file) {
        fprintf(log_file, "[%s] %s\n", time_str, message);
        fclose(log_file);
    }
}

void *miner(void* idp) {
    int my_id = *((int *)idp);
    Transaction transactions[config.transactions_per_block];
    char log_msg[256];
    
    snprintf(log_msg, sizeof(log_msg), "miner number %d", my_id);
    log_message(log_msg);

    int fd;
    if ((fd = open(VALIDATOR_PIPE_NAME, O_WRONLY)) < 0) {
    perror("Cannot open pipe for writing: ");
    exit(0);
    }



    while(!shutdown_requested) {
        


        pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
        pthread_setcanceltype(PTHREAD_CANCEL_DEFERRED, NULL);

        
        int num_got = get_transactions_from_pool(transactions, config.transactions_per_block);
        
        if (num_got == config.transactions_per_block) {
          
            char block_msg[256];
            snprintf(block_msg, sizeof(block_msg), 
                   "Miner %d: Got full block of %d transactions", 
                   my_id, num_got);
            log_message(block_msg);
            /*
            for (int i = 0; i < num_got; i++) {
                printf("TX %d: from %d to %d, value %d, miner %d\n", 
                       transactions[i].id,
                       transactions[i].sender_id,
                       transactions[i].receiver_id,
                       transactions[i].value,
                       my_id);
            }
            */

            //


            
            Block candidate_block = make_block(transactions, 0);
            
            char* last_hash=get_last_block_hash();

            strncpy(candidate_block.previous_hash, last_hash, HASH_SIZE);

            

            candidate_block.id=get_next_block_id();


            
           



            //-----------------------------------------
            PoWResult pow_result = proof_of_work(&candidate_block);


          
            


            if (pow_result.error) {
                char log_msg[256];
                snprintf(log_msg, sizeof(log_msg),
                       "Miner %d: Found valid nonce (%d) after %.2f seconds (%d ops)",
                       my_id, candidate_block.nonce, 
                       pow_result.elapsed_time, pow_result.operations);
                log_message(log_msg);}
            else {
            //-----------------------------------------
            printf("Block ID: %d\n", candidate_block.id);
            printf("Previous Hash:\n%s\n", candidate_block.previous_hash);
            printf("Block Timestamp: %ld\n", candidate_block.timestamp);
            printf("Nonce: %d\n", candidate_block.nonce);

             
             BlockSubmission submission;
             submission.block = candidate_block;
             submission.miner_id = my_id;
            
             for (int i = 0; i < num_got; i++) {
                submission.tx_ids[i] = transactions[i].id;  
                //printf("Miner %d: TX ID %d\n", my_id, submission.tx_ids[i]);    
            }
            


             
             if (write(fd, &submission, sizeof(submission)) != sizeof(submission)) {
                 perror("Failed to write full block submission to pipe");
             } else {
                 snprintf(log_msg, sizeof(log_msg),
                        "Miner %d: Sent block to validator",
                        my_id);
                 log_message(log_msg);
             }
            }


             
         
            sleep( (rand() % 10) + 5);

        
        




        } else {
           
            snprintf(log_msg, sizeof(log_msg),
                   "Miner %d: Only %d/%d transactions available - waiting",
                   my_id, num_got, config.transactions_per_block);
            log_message(log_msg);
            sleep( (rand() % 10) + 5); 
        }
    }


    close(fd);
    log_message("Miner: Shutting down");
    pthread_exit(NULL);
    return NULL;
}

Block make_block(Transaction* transactions, int pow_value) {
    Block new_block;
    
    
    new_block.id = -1;
    new_block.timestamp = time(NULL);
    new_block.nonce = pow_value;


    memset(new_block.previous_hash, 0, HASH_SIZE);

  
    new_block.transactions = malloc(config.transactions_per_block * sizeof(Transaction));
    if (!new_block.transactions) {
        perror("malloc failed");
        exit(EXIT_FAILURE);
    }
    memcpy(new_block.transactions, transactions, 
           config.transactions_per_block * sizeof(Transaction));

    return new_block;
}

Blockchain* blockchain = NULL;

void init_blockchain(int capacity) {
    size_t total_size = sizeof(Blockchain) + (capacity * sizeof(Block));
    int shmid = shmget(BLOCKCHAIN_KEY, total_size, IPC_CREAT | 0666);
    if (shmid == -1) {
        perror("shmget failed for blockchain");
        exit(EXIT_FAILURE);
    }

    blockchain = (Blockchain*)shmat(shmid, NULL, 0);
    if (blockchain == (void*)-1) {
        perror("shmat failed for blockchain");
        exit(EXIT_FAILURE);
    }

    

    pthread_mutexattr_t attr;
    pthread_mutexattr_init(&attr);
    pthread_mutexattr_setpshared(&attr, PTHREAD_PROCESS_SHARED);
    pthread_mutex_init(&blockchain->mutex, &attr);
    pthread_mutexattr_destroy(&attr);

    blockchain->blocks = (Block*)((char*)blockchain + sizeof(Blockchain));
    blockchain->size = 0;
    blockchain->capacity = capacity;

    
    if (blockchain->size == 0) {
        //Block genesis_block;
        Block genesis_block=generate_random_block(INITIAL_HASH, 0, config.transactions_per_block);

        printf("Genesis block created with ID: %d\n,nonce %d\n,ts %ld,hash%s\n", genesis_block.id,genesis_block.nonce,genesis_block.timestamp,genesis_block.previous_hash);

        blockchain->blocks[0] = genesis_block;
        blockchain->size = 1;
    }
}

Blockchain* attach_to_existing_blockchain() {
    int shmid = shmget(BLOCKCHAIN_KEY, 0, 0666);
    if (shmid == -1) {
        log_message("Failed to locate blockchain shared memory");
        return NULL;
    }

    Blockchain* chain = (Blockchain*)shmat(shmid, NULL, 0);
    if (chain == (void*)-1) {
        perror("shmat failed for blockchain");
        return NULL;
    }

    chain->blocks = (Block*)((char*)chain + sizeof(Blockchain));
    return chain;
}



//stats

Statistics stats;

void print_statistics() {
    lock_chain();
    int blockchain_size = blockchain->size;
    unlock_chain();
    
    lock_pool();
    int pending_transactions = tx_pool->size;
    unlock_pool();
    char log_msg[256];
    snprintf(log_msg, sizeof(log_msg),
                       "\n=== Blockchain Statistics ===\n");
                log_message(log_msg);
    snprintf(log_msg, sizeof(log_msg),
                       "Total blocks in blockchain: %d\n", blockchain_size);
                log_message(log_msg);
    snprintf(log_msg, sizeof(log_msg),
                       "Pending transactions: %d\n", pending_transactions);
                log_message(log_msg);
    snprintf(log_msg, sizeof(log_msg),
                       "Total blocks validated: %d\n", stats.total_blocks_validated);
                log_message(log_msg);
    snprintf(log_msg, sizeof(log_msg),
                       "\nMiner Statistics:\n");
                log_message(log_msg);
    snprintf(log_msg, sizeof(log_msg),
                       "%-10s %-15s %-15s %-15s\n", "Miner ID", "Valid Blocks", "Invalid Blocks", "Total Credits");
                log_message(log_msg);



    
    for (int i = 0; i < MAX_MINERS; i++) {
        if (stats.valid_blocks[i] > 0 || stats.invalid_blocks[i] > 0) {
            snprintf(log_msg, sizeof(log_msg),
                       "%-10d %-15d %-15d %-15d\n", 
                   i, 
                   stats.valid_blocks[i], 
                   stats.invalid_blocks[i],
                   stats.total_credits[i]);
                log_message(log_msg);
        }
    }
    
    if (stats.verification_count > 0) {
        snprintf(log_msg, sizeof(log_msg),
                       "\nAverage verification time: %.2f seconds\n", 
               stats.total_verification_time / stats.verification_count);
                log_message(log_msg);
    }
    
    printf("============================\n");
}

void handle_sigusr1(int sig) {
    (void)sig;
    printf("\nReceived SIGUSR1, printing statistics...\n");
    print_statistics();
}

void statistics_process() {
    
    memset(&stats, 0, sizeof(Statistics));
    pthread_mutex_init(&stats.mutex, NULL);
    
    signal(SIGUSR1, handle_sigusr1);
    signal(SIGINT, handle_sigint);
    
    
    int msgqid = msgget(MSGQ_KEY, IPC_CREAT | 0666);
    if (msgqid == -1) {
        perror("msgget failed");
        exit(EXIT_FAILURE);
    }
    
    log_message("Statistics: Process started");
    char stats_msg[256];
    pid_t my_pid=getpid();
    snprintf(stats_msg, sizeof(stats_msg),
                       "Statistics: Process started with id %d",my_pid);
                log_message(stats_msg);
    
    while (!shutdown_requested) {
        StatsMessage msg;
        ssize_t bytes = msgrcv(msgqid, &msg, sizeof(msg) - sizeof(long), 0, 0);
        
        




        if (bytes > 0) {
            pthread_mutex_lock(&stats.mutex);
            
            stats.total_blocks_validated++;
            
            if (msg.msg_type == MSG_VALID_BLOCK) {
                stats.valid_blocks[msg.miner_id]++;
                stats.total_credits[msg.miner_id] += msg.credits_earned;
                stats.total_verification_time += msg.verification_time;
                stats.verification_count++;
                
                char log_msg[256];
                snprintf(log_msg, sizeof(log_msg),
                       "Statistics: Miner %d earned %d credits for valid block",
                       msg.miner_id, msg.credits_earned);
                log_message(log_msg);
            } else {
                stats.invalid_blocks[msg.miner_id]++;
                
                char log_msg[256];
                snprintf(log_msg, sizeof(log_msg),
                       "Statistics: Miner %d submitted invalid block",
                       msg.miner_id);
                log_message(log_msg);
            }
            
            pthread_mutex_unlock(&stats.mutex);
        } else if (bytes == -1 && errno != EINTR) {
            perror("msgrcv failed");
            break;
        }
    }
    
    // Print final statistics
    print_statistics();



    /*
    if ((void*)tx_pool != (void*)-1 && tx_pool != NULL) {
    if (shmdt(tx_pool) == -1)
        perror("shmdt tx_pool failed");
    }
    if ((void*)blockchain != (void*)-1 && blockchain != NULL) {
        if (shmdt(blockchain) == -1)
            perror("shmdt blockchain failed");
    }
    */
    // Cleanup
    if (tx_pool) shmdt(tx_pool);
    if (blockchain) shmdt(blockchain);
    msgctl(msgqid, IPC_RMID, NULL);
    log_message("Statistics: Process shutting down");
    exit(0);
}



pid_t validator_pids[3] = {0}; // 0 = não iniciado

void* monitor_validators(void* arg) {
    (void)arg;
    while (!shutdown_requested) {
        sleep(2);  

        lock_pool();
        float occupancy = (float)tx_pool->size / tx_pool->capacity;
        unlock_pool();

        int required_validators = 1;
        if (occupancy >= 0.8) required_validators = 3;
        else if (occupancy >= 0.6) required_validators = 2;

        
        for (int i = 1; i < required_validators; i++) {
            if (validator_pids[i] == 0) {
                pid_t pid = fork();
                if (pid == 0) {
                    validator();
                    exit(0);
                }
                validator_pids[i] = pid;
                char msg[100];
                snprintf(msg, sizeof(msg), "Controller: Validator %d launched (occupancy %.2f)", i + 1, occupancy);
                log_message(msg);
            }
        }

       
        for (int i = required_validators; i < 3; i++) {
            if (validator_pids[i] > 0) {
                kill(validator_pids[i], SIGINT);
                validator_pids[i] = 0;
                char msg[100];
                snprintf(msg, sizeof(msg), "Controller: Validator %d terminated (occupancy %.2f)", i + 1, occupancy);
                log_message(msg);
            }
        }
    }

    return NULL;
}
