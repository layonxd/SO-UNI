//Pedro Dioniso Mendes 2021218522
//Rodrigo Rodrigues 2021217624




#include "common.h"
//#include "txgen.c"



int main(int argc, char const *argv[]) {
    if (argc != 3) {
        fprintf(stderr, "Usage: %s <value> <sleep_time_ms>\n", argv[0]);
        return 1;
    }


    signal(SIGINT, handle_sigint);
    //signal(SIGINT, handle_sigusr1);

   
    config = read_config("config.cfg");
    printf("Generator: Pool size from config = %d\n", config.pool_size);



  
    
    tx_pool= attach_to_existing_pool();
    
    if (tx_pool == NULL) {
        fprintf(stderr, "Generator: FATAL: tx_pool is NULL after init!\n");
        exit(EXIT_FAILURE);
    }
    

    printf("Generator: Successfully attached to tx_pool at %p\n", tx_pool);

    int value = atoi(argv[1]); 
    int sleep_time_ms = atoi(argv[2]); 
    int counter = 0;
    while (!shutdown_requested) {
        Transaction tx = create_transaction(counter, value);
        
        if (add_transaction_to_pool(tx) == 0) {
         
            printf("Generator: Added TX ID %d (Value: %d) to pool\n", 
                   tx.id, value);
            counter++;
        } else {
            printf("Generator: Transaction pool is full\n");
        }

    
        usleep(sleep_time_ms * 1000); 
    }



    if (tx_pool) {
        shmdt(tx_pool);
    }
    printf("Generator: Shutting down");
    return 0;
}