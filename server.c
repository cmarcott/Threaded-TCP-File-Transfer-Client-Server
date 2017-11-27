 /************************************
 * server.c
 * TODO UPDATE THIS DISCRIPTION
 * This file allows tcp clients to connect and send text data
 * it will output this data to stdout and return it to the client
 ***********************************/

#include "server.h"

#define DEBUG false
 
 /**
 * CheckPort
 * Finds the port number input on the command line
 * @param int argc
 * @param char * argv[]
 */
int CheckPort(int argc, char *argv[]) {

	//Ensures a port number has been specified
	if (argc < 2) {
		fprintf(stderr, "%s\n", "Please specify a port number...");
		return EXIT_FAILURE;
	}
	char * portnum = argv[1];
	//Ensures the port number is an integer
	if (!isdigit(*portnum)) {
		fprintf(stderr, "%s\n", "Please enter an integer as the port number");
		return EXIT_FAILURE;
	}
	return atoi(portnum);
}

 /**
 * FileTransfer
 * Recieves incoming client data and saves it to file
 * @param int argc
 * @param char * argv[]
 */
void FileTransfer(int node_id, TransferQueue *t_queue) {

	int len;
	char buffer[MAXBUFFER + 1];
	uint64_t param_buffer;

  	int total_bytes_sent = 0;
	int total_bytes_recieved = 0;
	char filename[21];
	char dest_filename_orig[255];

	strcpy(dest_filename_orig, "\0");
	TransferNode* node_instance;
	
	if((node_instance = FindProperNode(node_id, t_queue)) == NULL) {
		fprintf(stderr, "%s %d", "Problem finding transfer node", node_id);
	}
	int consocket = node_instance->thread_info.consocket;

	//Receive data from the client

	//Recieve file size information from the client
	len = recv(consocket, &param_buffer, sizeof(uint64_t), 0);
 	if ((len == 0) || (len == -1)) {
 		printf("\n**Error recieving file size\n");
 		//break;
 	}
 	
 	//Decode the file size
	 param_buffer = ntohl(param_buffer);
	 
	//Save the file size information to record
	pthread_mutex_lock(&t_queue->mutex);
	node_instance->thread_info.total_file_size = param_buffer;
	pthread_mutex_unlock(&t_queue->mutex);

	//Recieve chunk size information from the client
	len = recv(consocket, &param_buffer, sizeof(uint64_t), 0);
 	if ((len == 0) || (len == -1)) {
 		printf("\n**Error recieving Chunk size\n");
 		//break;
 	}

 	//Decode the chunk size
	param_buffer = ntohl(param_buffer);

	//Save the chunk size information to record
	pthread_mutex_lock(&t_queue->mutex);
	node_instance->thread_info.chunk_size = param_buffer;
	pthread_mutex_unlock(&t_queue->mutex);

	//Recieve file name information from the client
	len = recv(consocket, &filename, sizeof(char)*21, 0);
 	if ((len == 0) || (len == -1)) {
 		printf("\n**Error recieving File Name\n");
 		//break;
 	}

	//Save the file name
	pthread_mutex_lock(&t_queue->mutex);
	strcpy(node_instance->thread_info.file_name, filename);
	pthread_mutex_unlock(&t_queue->mutex);
	if (DEBUG) {
		printf("Recieved %d bytes for File Name Size\n", len);
	}

	//Create new file name to save
	FILE * destination_file = NULL;

	sprintf(dest_filename_orig, "DownloadedFiles/%s", filename);
	char dest_filename[255];
	int i;
	
	for(i = 1; ; i++) {
		sprintf(dest_filename, "%s(%d", dest_filename_orig, i);
		strcat(dest_filename, ").txt");

		destination_file = fopen(dest_filename, "r");
		if (destination_file == NULL) {
			destination_file = fopen(dest_filename, "w");
			break;
		} else {
			fclose(destination_file);
		}
	}
	char * token;
	token = strtok(dest_filename, "/");
	token = strtok(NULL, "/");

	//Save what the destination filename is going to be
	pthread_mutex_lock(&t_queue->mutex);
	strcpy(node_instance->thread_info.dest_file_name, token);
	pthread_mutex_unlock(&t_queue->mutex);

	//This loop allows all data from the client to be recieved before closing the connection
	 do {

		//Recieve Data from Client
 		len = recv(consocket, buffer, MAXBUFFER, 0);
		total_bytes_recieved += len;
		 
		//Save the current amount downloaded to struct variable
		pthread_mutex_lock(&t_queue->mutex);
		node_instance->thread_info.current_downloaded = total_bytes_recieved;
		pthread_mutex_unlock(&t_queue->mutex);
 		if ((len == 0) || (len == -1)) {
 			break;
 		}
 		buffer[len] = '\0';
		//if (!DEBUG) printf("%s", buffer);
		fprintf(destination_file, "%s", buffer);
		if (DEBUG) printf("Recieved %d bytes\n", len);
		
		//Send data to client
		int sendsize;
		if((sendsize = send(consocket, &len, sizeof(int), 0)) > 0) {
			total_bytes_sent += len;
			if (DEBUG) printf("Returned confirmation of %d bytes\n", len);
		} else {
			if (DEBUG) printf("No bytes were returned to client\n");
		}

	} while (len != -1 && len != 0);

	pthread_mutex_lock(&t_queue->mutex);
	node_instance->thread_info.finished = true;
	pthread_mutex_unlock(&t_queue->mutex);

	//close(mysocket);
	//return EXIT_SUCCESS;

}

 /**
 * main
 * @param int argc
 * @param char * argv[]
 */
int main(int argc, char *argv[]) {
	
	struct sockaddr_in dest; 
	struct sockaddr_in serv; 
	int mysocket, consocket, PORTNUM, node_id;  
	TransferQueue *transfer_queue = NULL;
	pthread_t UI_thread, worker_thread, UI_thread2;   

	transfer_queue = InitializeTransferQueue();
	socklen_t socksize = sizeof(struct sockaddr_in);

	//Get Port number from command line
	if (((PORTNUM = CheckPort(argc, argv))) == EXIT_FAILURE) {
		return EXIT_FAILURE;
	}

	transfer_queue->port_number = PORTNUM;

	//Initialize server values
	memset(&serv, 0, sizeof(serv));           
	serv.sin_family = AF_INET;                
	serv.sin_addr.s_addr = htonl(INADDR_ANY); 
	serv.sin_port = htons(PORTNUM);           

	//Create a socket. The arguments indicate that this is an IPv4, TCP socket
	if ((mysocket = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
		fprintf(stderr, "%s", "There was an error connecting to the socket\n");
		return EXIT_FAILURE;
	}
	//Bind serv information to mysocket 								
	if (bind(mysocket, (struct sockaddr *)&serv, sizeof(struct sockaddr)) == -1) {
		fprintf(stderr, "%s", "There was an error binding to the socket\n");
		return EXIT_FAILURE;
	}

	//Start listening			
	transfer_queue->listen_socket = listen(mysocket, 10);

	//Create the UI Thread
	if (pthread_create( &UI_thread , NULL , UserInterfaceMenu , (void*)transfer_queue) < 0) {
    	fprintf(stderr, "%s\n", "There was an error creating the UI thread");
    }

	//Create a worker thread that will clean up transfers from the queue
	if (pthread_create( &worker_thread , NULL , QueueRemover , (void*)transfer_queue) < 0) {
    	fprintf(stderr, "%s\n", "There was an error creating the worker thread");
	}
	
	transfer_queue->main_socket = mysocket;
	node_id = 1;

	//Continuously accept connections												
	while ( (consocket = accept(mysocket, (struct sockaddr *)&dest, &socksize)) ) {

		if(transfer_queue->exit_signal) {
			break;
		}
		InitiateTransferThread(transfer_queue, dest, consocket, node_id);
		node_id += 1;

	}

	if ((transfer_queue->size > 0) && (transfer_queue->soft_exit)) {
		system("clear");
		fprintf(stdout, "%s", "\n\nWaiting for active transfers to finish...\n\n");
		//Create UI thread for final file transfer information
		if (pthread_create( &UI_thread2 , NULL , UserInterfaceMenuFinal , (void*)transfer_queue) < 0) {
			fprintf(stderr, "%s\n", "There was an error creating the UI thread 2");
		}
		WaitForTransfers(transfer_queue);
		pthread_join(UI_thread2, NULL);
	}

	close(consocket);
	close(mysocket);

	return EXIT_SUCCESS;
}

void WaitForTransfers(TransferQueue* t_queue) {

	TransferNode* iteration_node = t_queue->head;
	int i = 0;
	int count = 0;
	pthread_t* remaining_transfers[1000];

	pthread_mutex_lock(&t_queue->mutex);
	while (iteration_node != NULL) {
		remaining_transfers[count] = &iteration_node->thread_info.transfer_thread;
		iteration_node = iteration_node->next;
		count += 1;
	}

	pthread_mutex_unlock(&t_queue->mutex);
	for (i=0; i<count; i++) {
		pthread_join(*remaining_transfers[i], NULL);
	}
	t_queue->final_close_UI = true;

}

 /**
 * main
 * A secondary UI thread function that runs after the user has requested to quit the server
 * It loops through the remaining transfers, giving the user information about progress
 * @param void* t_queue_voided
 */
void* UserInterfaceMenuFinal (void* t_queue_voided) {

	TransferQueue *t_queue = (TransferQueue*)t_queue_voided;
	
	while (t_queue->size > 0) {
		if (t_queue->final_close_UI) {
			break;
		}
		pthread_mutex_lock(&t_queue->mutex);
		system("clear");
		fprintf(stdout, "%s", "Waiting for the following transfers to finish...\n\n");
		PrintQueue(t_queue);
		pthread_mutex_unlock(&t_queue->mutex);
		usleep(500000);
	}

	pthread_mutex_lock(&t_queue->mutex);
	PrintQueue(t_queue);
	system("clear");
	fprintf(stdout, "%s", "\n\nAll transfers have completed... Finalizing server exit...\n\n");
	pthread_mutex_unlock(&t_queue->mutex);
	//usleep(500000);
	pthread_exit(NULL);


	return NULL;

}

