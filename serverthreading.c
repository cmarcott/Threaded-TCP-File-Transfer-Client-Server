#include "server.h"

 /**
 * InitializeTransferQueue
 * Allocates memory for and initializes values for a TransferQueue struct
 */
TransferQueue* InitializeTransferQueue() {

    TransferQueue* t_queue = (TransferQueue*)malloc(sizeof(TransferQueue));
    t_queue->head = t_queue->tail = NULL;
    t_queue->size = 0;
    t_queue->exit_signal = false;
    t_queue->soft_exit = false;
    t_queue->final_close_UI = false;
    pthread_mutex_init(&t_queue->mutex, NULL);
    
    //Initialize the condition variable
    pthread_cond_init(&t_queue->cond, NULL);
    return t_queue;
}

 /**
 * InitiateTransferThread
 * Called from main to initiate a transfer thread
 * Creates the thread for a transfer
 * @param TransferQueue* t_queue
 * @param struct sockaddr_in dest
 * @param int consocket
 * @param int node_id
 */
void InitiateTransferThread(TransferQueue *t_queue, struct sockaddr_in dest, int consocket, int node_id) {
    
    // Initializing Node Data for Transfer Thread
    TransferNode *b_node = (TransferNode*)malloc(sizeof(TransferNode));

    //Create a ThreadArgs struct to send as a void* to the thread function
    ThreadArgs* thread_args = malloc(sizeof(ThreadArgs));
    thread_args->cur_node = b_node;
    thread_args->t_queue = t_queue;

    //Initialize TransferNode variables
    b_node->thread_info.total_file_size = 0;
    b_node->thread_info.chunk_size = 0;
    b_node->thread_info.current_downloaded = 0;
    b_node->thread_info.dest = dest;
    b_node->thread_info.consocket = consocket;
    b_node->thread_info.finished = false;
    b_node->node_id = node_id;
    b_node->next = NULL;
    
    //Create the transfer thread
    if (pthread_create( &b_node->thread_info.transfer_thread , NULL , ThreadedTransfer , thread_args) < 0) {
        fprintf(stderr, "%s %d\n", "There was an error creating thread id", b_node->node_id);
    }
}

 /**
 * ThreadedTransfer
 * The function for a transfer thread
 * @param void* thread_data
 */
void *ThreadedTransfer(void* thread_data) {

    ThreadArgs *thread_args = (ThreadArgs*)thread_data;
    TransferNode *node_instance = thread_args->cur_node;
    TransferQueue *t_queue = thread_args->t_queue;

    //Add the thread information to the TransferQueue
    t_queue = AddTransferRecordToQueue(t_queue, node_instance);

    //Perform the actual transfer
    FileTransfer(node_instance->node_id, t_queue);

    //Signal that the file has finished downloading and can be removed
    pthread_cond_signal(&t_queue->cond);
    pthread_exit(NULL);

    return NULL;

}

 /**
 * AddTransferRecordToQueue
 * Function to append the transfer node to the main queue
 * @param TransferQueue* t_queue
 * @param TransferNode* t_node
 */
TransferQueue* AddTransferRecordToQueue(TransferQueue *t_queue, TransferNode *t_node) {

    pthread_mutex_lock(&t_queue->mutex);
    if (t_queue->tail != NULL) {
        t_queue->tail->next = t_node;       // append after tail
        t_queue->tail = t_node;
    } else {
        t_queue->tail = t_queue->head = t_node;   // first node
    }
    
    t_queue->size += 1;
    pthread_mutex_unlock(&t_queue->mutex);

    return t_queue;

}

 /**
 * UserInterfaceMenu
 * The UI Thread Function
 * @param void* t_queue_voided
 */
void* UserInterfaceMenu(void* t_queue_voided) {

    TransferQueue *t_queue = (TransferQueue*)t_queue_voided;
    char user_choice;

    system("clear");

    while(1) {
        
        //The main display output, refreshes each user input
        fprintf(stdout, "%s\n", "Server Controls:\n");
        fprintf(stdout, "%s\n", "'d' - Display/Refresh Active Transfers         'q' - Shutdown Server");
        scanf(" %c", &user_choice);
        if (user_choice == 'd') {
            pthread_mutex_lock(&t_queue->mutex);
            PrintQueue(t_queue);
            pthread_mutex_unlock(&t_queue->mutex);
        } else if (user_choice == 'q') {
            system("clear");
            fprintf(stdout, "%s", "Exiting server...\n\n");
            t_queue->exit_signal = true;
        } else {
            system("clear");
        }

        if (t_queue->exit_signal) {
            //Locks the mutex to temporarily stop downloads while the user decides
            pthread_mutex_lock(&t_queue->mutex);
            fprintf(stdout, "%s", "\n--\n[SHUTDOWN] Would you like to perform a soft shutdown?\n");
            fprintf(stdout, "%s", "Please choose 'y' for soft shut down, or 'n' for hard exit of the server\n\n");
            fprintf(stdout, "%s", "[y/n]\n--\n");
            user_choice = 'a';
            while ((user_choice != 'y') && (user_choice != 'n')) {
                scanf(" %c", &user_choice);
                //Close the listening socket and advance the main thread to wait until all downloads have finished
                if (user_choice == 'y') {
                    t_queue->soft_exit = true;
                    close(t_queue->listen_socket);
                    pthread_mutex_unlock(&t_queue->mutex);
                    SignalMainSocket(t_queue);
                    return NULL;
                //Close all sockets and signal the main thread to exit server completely
                } else if (user_choice == 'n') {
                    close(t_queue->main_socket);
                    SignalMainSocket(t_queue);
                    pthread_mutex_unlock(&t_queue->mutex);
                    return NULL;
                } else {
                    system("clear");
                    fprintf(stdout, "%s", "\n\n**Please choose 'y' for soft shut down, or 'n' for hard shut down\n");
                }
            }
        }
    }

    return NULL;
}

 /**
 * SignalMainSocket
 * Signals the main socket to advance, a parameter in main is set to break from the main loop
 * and to proceed with soft shut down
 * @param TransferQueue* t_queue
 */
void SignalMainSocket(TransferQueue* t_queue) {

	int mysocket;
	struct sockaddr_in dest;
 
	mysocket = socket(AF_INET, SOCK_STREAM, 0);
  
	memset(&dest, 0, sizeof(dest));
	dest.sin_family = AF_INET;
	dest.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
	dest.sin_port = htons(t_queue->port_number); 
 	
	// Signal the server to continue
	connect(mysocket, (struct sockaddr *)&dest, sizeof(struct sockaddr_in));
 
	close(mysocket);
}

 /**
 * PrintQueue
 * Prints output to the UI Thread
 * @param TransferQueue* t_queue
 */
void PrintQueue(TransferQueue *t_queue) {

    float percent_downloaded = 0;

    system("clear");
    if (t_queue->head == NULL) {
        fprintf(stdout, "%s", "\n--\n**There are no current active transfers to be displayed**\n--\n");
        return;
    }

    TransferNode* iteration_node = t_queue->head;
    printf("\nActive Transfers:\n-----------");
    while(iteration_node != NULL) {
        printf("\n");
        printf("Transfer id: %d", iteration_node->node_id);
        printf("   Original Filename: %s", iteration_node->thread_info.file_name);
        printf("  ||  Destination Filename: %s\n", iteration_node->thread_info.dest_file_name);
        //MacOS and Linux have different compiling rules for uint64_t types
        #ifdef __APPLE__
        printf("Downloaded: %llu/%llu bytes", iteration_node->thread_info.current_downloaded, iteration_node->thread_info.total_file_size);
        percent_downloaded = (((float)iteration_node->thread_info.current_downloaded/(float)iteration_node->thread_info.total_file_size)*100);
        printf("    Completed: %.02f%%", percent_downloaded);
        printf("    Chunk Size: %llu\n", iteration_node->thread_info.chunk_size);
        #endif
        #ifdef __linux__
        printf("Downloaded: %lu/%lu bytes", iteration_node->thread_info.current_downloaded, iteration_node->thread_info.total_file_size);
        percent_downloaded = (((float)iteration_node->thread_info.current_downloaded/(float)iteration_node->thread_info.total_file_size)*100);
        printf("    Completed: %.02f%%", percent_downloaded);
        printf("    Chunk Size: %lu\n", iteration_node->thread_info.chunk_size);
        #endif

        iteration_node = iteration_node->next;
        printf("-----------\n");
    }
    printf("\n");
    return;
}

 /**
 * QueueRemover
 * The worker thread function for removing from the queue
 * @param void* t_queue_voided
 */
void* QueueRemover(void* t_queue_voided) {
    TransferQueue *t_queue = (TransferQueue*)t_queue_voided;
    int i, initial_size;

    while(1){
        pthread_mutex_lock(&t_queue->mutex);
        pthread_cond_wait(&t_queue->cond, &t_queue->mutex);

        if (t_queue->exit_signal) {
            pthread_mutex_unlock(&t_queue->mutex);
            pthread_exit(NULL);
        }
        initial_size = t_queue->size;
        for(i=0; i<initial_size; i++) {
            RemoveFinished(t_queue);
        }
        
        pthread_mutex_unlock(&t_queue->mutex);

    }
    

}

 /**
 * RemoveFinished
 * Actualy removes any nodes set to finished
 * @param TransferQueue* t_queue
 */
void RemoveFinished(TransferQueue* t_queue) {

    TransferNode* cur_node, *prev_node;

    cur_node = t_queue->head;
    prev_node = NULL;

    while (cur_node != NULL) {
        if (cur_node->thread_info.finished == true) {
            if (prev_node == NULL) {
                t_queue->head = cur_node->next;
                t_queue->tail = cur_node->next;
            } else {
                prev_node->next = cur_node->next;
            }
            free(cur_node);
            t_queue->size -= 1;
            break;
        }
        prev_node = cur_node;
        cur_node = cur_node->next;
    }
}

 /**
 * FineProperNode
 * Finds and returns a node based on it's node_id
 * @param int node_id
 * @param TransferQUeue* t_queue
 */
TransferNode* FindProperNode(int node_id, TransferQueue* t_queue) {

    TransferNode* iteration_node;
    iteration_node = t_queue->head;
    pthread_mutex_lock(&t_queue->mutex);
    while(iteration_node != NULL) {
        if (iteration_node->node_id == node_id) {
            pthread_mutex_unlock(&t_queue->mutex);
            return iteration_node;
        } else {
            iteration_node = iteration_node->next;
        }
    }
    pthread_mutex_unlock(&t_queue->mutex);
    return NULL;
}


