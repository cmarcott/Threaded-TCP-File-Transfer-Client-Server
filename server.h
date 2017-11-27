#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <stdbool.h>
#include <pthread.h>
#include <time.h>
 
#define MAXBUFFER 4096

typedef struct TransferThreadData {
    pthread_t transfer_thread;
    struct sockaddr_in dest;
    uint64_t total_file_size;
    uint64_t chunk_size;
    uint64_t current_downloaded;
    char file_name[21];
    char dest_file_name[255];
    int consocket;
    bool finished;
} TransferThreadData;

typedef struct TransferNode {
    TransferThreadData thread_info;
    int node_id;
    struct TransferNode* next;
} TransferNode;

typedef struct TransferQueue {
    TransferNode* head;
    TransferNode* tail;
    int size;
    int main_socket;
    int listen_socket;
    int port_number;
    bool exit_signal;
    bool soft_exit;
    bool final_close_UI;
    pthread_mutex_t mutex;
    pthread_cond_t cond;
} TransferQueue;

typedef struct ThreadArgs {
    TransferNode* cur_node;
    TransferQueue* t_queue;
} ThreadArgs;

int CheckPort(int argc, char *argv[]);
void FileTransfer(int node_id, TransferQueue *t_queue);
TransferQueue* InitializeTransferQueue();
void InitiateTransferThread(TransferQueue *t_queue, struct sockaddr_in dest, int consocket, int node_id);
void *ThreadedTransfer(void *thread_data);
TransferQueue* AddTransferRecordToQueue(TransferQueue *t_queue, TransferNode *t_node);
void *UserInterfaceMenu(void* t_queue_voided);
void PrintQueue(TransferQueue *t_queue);
void *QueueRemover(void* t_queue_voided);
TransferNode* FindProperNode(int node_id, TransferQueue* t_queue);
void RemoveFinished(TransferQueue* t_queue);
void SignalMainSocket(TransferQueue* t_queue);
void WaitForTransfers(TransferQueue* t_queue);
void* UserInterfaceMenuFinal (void* t_queue_voided);




