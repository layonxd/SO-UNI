//Pedro Dioniso Mendes 2021218522
//Rodrigo Rodrigues 2021217624



#include "common.h"
#include "pow.h"


int main(int argc, char* argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <config_file>\n", argv[0]);
        exit(EXIT_FAILURE);
    }
    //le a config
    config = read_config(argv[1]);
    log_message("Controller: Config loaded");

 
    if (signal(SIGINT, handle_sigint) == SIG_ERR) {
    perror("signal");
    exit(EXIT_FAILURE);
}

    

    init_transaction_pool(config.pool_size);//test
    
      if (tx_pool == NULL) {
          fprintf(stderr, "Generator: FATAL: tx_pool is NULL after init!\n");
          exit(EXIT_FAILURE);
      }
  
      printf("controler: Successfully creeated tx_pool at %p\n", tx_pool);
  
  
      mkfifo(VALIDATOR_PIPE_NAME, 0666);
      
      init_blockchain(config.blockchain_blocks);
      if (blockchain == NULL) {
          fprintf(stderr, "Generator: FATAL: tx_pool is NULL after init!\n");
          exit(EXIT_FAILURE);
      }


    

    pid_t validator_pid,stats_pid;

    if ((validator_pid = fork()) == 0) {
      //init_transaction_pool(config.pool_size);
      //init_blockchain(config.blockchain_blocks);
     
      validator();

      exit(0);
      }
      else if ((stats_pid = fork()) == 0) {
        statistics_process();
        exit(0);
}
      else {
        int i;
        pthread_t my_thread[config.num_miners];//?
        int id[config.num_miners];//?
    
      for (i = 0; i < config.num_miners; i++) {
        id[i] = i+1;
        pthread_create(&my_thread[i], NULL, miner, &id[i]);
      }

      pthread_t validator_monitor;
        pthread_create(&validator_monitor, NULL, monitor_validators, NULL);

    
    
      while (!shutdown_requested) {
            sleep(1);
        }
        //system("pkill -SIGINT TxGen");

        pthread_cancel(validator_monitor);
        pthread_join(validator_monitor, NULL);
        
        // Cleanup
        log_message("Controller: Shutting down miners...");
        for (int i = 0; i < config.num_miners; i++) {
            pthread_cancel(my_thread[i]);
            pthread_join(my_thread[i], NULL);
        }
        
        log_message("Controller: Shutting down validator...");
        kill(validator_pid, SIGINT);
        waitpid(validator_pid, NULL, 0);



        
        
        log_message("Controller: Shutting down statistics...");
        kill(stats_pid, SIGINT);
        waitpid(stats_pid, NULL, 0);
        

        
        
        unlink(VALIDATOR_PIPE_NAME);
        
       
        
        

    
   

       
    }

    if (tx_pool) shmdt(tx_pool);
    if (blockchain) shmdt(blockchain);
    int shmid = shmget(SHM_KEY, 0, 0666);
    if (shmctl(shmid, IPC_RMID, NULL) == -1) {
        perror("shmctl(IPC_RMID) failed for tx_pool");
    }
    int shmid2 = shmget(BLOCKCHAIN_KEY, 0, IPC_CREAT | 0666);
    if (shmctl(shmid2, IPC_RMID, NULL) == -1) {
        perror("shmctl(IPC_RMID) failed for tx_pool");
    }
    /*
    int msgqid = msgget(MSGQ_KEY, 0);
        if (msgqid != -1) {
            msgctl(msgqid, IPC_RMID, NULL);
         }
        msgctl(msgqid, IPC_RMID, NULL);
    */
    log_message("Controller: Shutdown complete");





    return 0;
}