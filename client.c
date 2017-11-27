 /************************************
 * client.c
 * This file connects to the specified server
 * and sends the contents of a file to the server
 ***********************************/

#include "client.h"

#define DEBUG false

/**
 * CheckInput
 * Takes in the command line arguments for processing
 * @param char ** full_server_name
 * @param char ** server_name
 * @param char ** specified_port_number
 * @param int argc
 * @param char * argv[]
 * @param int * buffer_len
 * @param char ** filename
 */
int CheckInput(char ** full_server_name, char ** server_name, char ** specified_port_number, int argc, char * argv[], int * buffer_len, char ** filename) {

	char * base_file = NULL;

	//Get server name and port number from the user
 	*full_server_name = argv[1];

 	//The following code block is error checking the command line arguments
 	if (*full_server_name == NULL) {
 		ArgErrorPrint();
 		return FAILURE;
 	} else {
 		*server_name = strtok(*full_server_name, ":");
 		*specified_port_number = strtok(NULL, ":");
 		//Error checking for port number
 		if (*specified_port_number == NULL) {
			ArgErrorPrint();
			return FAILURE;
 		} else {
 			//Double check port number is actually integer
 			if (!(isdigit(**specified_port_number))) {
 				fprintf(stderr, "%s", "**Please make sure the port number is an integer\n**Exiting...\n");
 				return FAILURE;
 			}
 		}
 	}

 	//Checking for the first argument slot
 	if (argc > 2) {
 		if (strcmp(argv[2], "-b") == 0) {
 			if (argc > 3) {
 				//Checks if the buffer is an integer
		 		if (!(isdigit(*argv[3]))) {
		 			fprintf(stderr, "%s", "Please enter an integer for the -b optional buffer flag\n");
		 			return FAILURE;
		 		} else {
		 			*buffer_len = atoi(argv[3]);
		 		}
		 	} else {
		 		fprintf(stderr, "%s", "You have specified the -b optional buffer flag but not specified a buffer value\n");
		 		return FAILURE;
		 	}
	 	} else if (strcmp(argv[2], "-f") == 0) {
	 		if (argc > 3) {
	 			base_file = basename(argv[3]);
	 			if (FilenameTooLong(base_file)) {
	 				fprintf(stderr, "%s\n", "The filename you have entered is too long");
	 				return FAILURE;
	 			} else {
	 				*filename = argv[3];
	 			}
	 		} else {
	 			fprintf(stderr, "%s", "You have specified the -f optional filename flag but have not specified a filename\n");
	 			return FAILURE;
	 		}
	 	}
 	}

 	// Checking for the second argument slot
 	if (argc > 4) {
 		if (strcmp(argv[4], "-b") == 0) {
 			if (argc > 5) {
		 		if (!(isdigit(*argv[5]))) {
		 			fprintf(stderr, "%s", "Please enter an integer for the -b optional buffer flag\n");
		 			return FAILURE;
		 		} else {
		 			*buffer_len = atoi(argv[5]);
		 		}
		 	} else {
		 		fprintf(stderr, "%s", "You have specified the -b optional buffer flag but not specified a buffer value\n");
		 		return FAILURE;
		 	}
	 	} else if (strcmp(argv[4], "-f") == 0) {
	 		if (argc > 5) {
	 			base_file = basename(argv[5]);
	 			if (FilenameTooLong(base_file)) {
	 				fprintf(stderr, "%s\n", "The filename you have entered is too long");
	 				return FAILURE;
	 			} else {
	 				*filename = argv[5];
	 			}
	 		} else {
	 			fprintf(stderr, "%s", "You have specified the -f optional filename flag but have not specified a filename\n");
	 			return FAILURE;
	 		}
	 	}
 	}

 	return SUCCESS;
}

/**
 * FilenameTooLong
 * Checks if the filename entered by the user is under 20 characters
 * @param char * filename
 */
int FilenameTooLong(char * filename) {
	if (strlen(filename) > 20) {
		return 1;
	} else {
		return 0;
	}
}

/**
 * ConnectToServer
 * Uses parameters from CheckInput() to connect to the correct TCP server
 * @param int argc
 * @param char * argv[]
 */
int ConnectToServer(int argc, char * argv[]) {

	char * msg;
	int total_bytes_sent = 0;
	int total_bytes_recieved = 0;
	int chunk_bytes_recieved = 0;
	int tmp_chunk_bytes_recieved = 0;
	char current_char;
	int buffer_len = 4096; //The default buffer length if no other is specified
	int len, mysocket, status;
	struct addrinfo hints, *result, *res;
	FILE * input_file;
	char * base_file;

	char * full_server_name = NULL, * server_name = NULL, * specified_port_number = NULL, * filename = NULL;
	filename = "Files/inputfile.txt"; // The default filename if no other is specified

 	//Check command line input from user
 	if(!(status = CheckInput(&full_server_name, &server_name, &specified_port_number, argc, argv, &buffer_len, &filename))) {
 		return EXIT_FAILURE;
 	}

 	char in_buffer[buffer_len + 1];

	//Initializing
	strcpy(in_buffer, "\0");

 	input_file = fopen(filename, "r");
 	if (input_file == NULL) {
 		fprintf(stderr, "%s\n", "Could not find file specified...");
    	return EXIT_FAILURE;
    }

 	//Specify address information
	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;

	//Find ip address from hostname
	if ((status = getaddrinfo(server_name, specified_port_number, &hints, &result)) == -1) { 
	    fprintf(stderr, "getaddrinfo error: %s\n", gai_strerror(status)); 
	    exit(1); 
	}    
	//Create socket and connect
	int connected = 0;
	while (!connected) {
		for (res = result; res != NULL; res = res->ai_next) { 
			if (((mysocket = socket(res->ai_family, res->ai_socktype, res->ai_protocol)) == -1)) {
				continue;
			}  
			if (connect(mysocket, res->ai_addr, res->ai_addrlen) == -1) {
				close(mysocket);
				continue;
			}
			connected = 1;
			break;
		}
		//If no connection made, retry until server responds
		if (res == NULL) {
			printf("Connection could not be made, will try again in 5 seconds...\n");
			connected = 0;
			sleep(5);
		}
	}

	//Allows client side error handling
	signal(SIGPIPE, SIG_IGN);
	printf("Transferring text file...\n");

	//Find and store file size
	fseek(input_file, 0L, SEEK_END);
	int input_file_sz = ftell(input_file);
	rewind(input_file);


	//Send encoded information about the total file size to the server
	//8 byte = 64bit integer
	uint64_t file_size = htonl(input_file_sz);
	uint64_t file_size_decoded = ntohl(file_size);
	if((send(mysocket, &file_size, sizeof(uint64_t), 0)) > 0) {
		//Different unsigned integer compilers for Linux vs Macintosh
		#ifdef __linux__
		if (DEBUG) printf("Successfully sent file size of %lu\n", file_size_decoded);
		#endif
		#ifdef __APPLE__
		if (DEBUG) printf("Successfully sent file size of %llu\n", file_size_decoded);
		#endif
	} else {
		fprintf(stderr, "%s", "There was an error sending file size\n");
	}

	//Send encoded information about chunk size to the server
	uint64_t chunk_size = htonl(buffer_len);
	uint64_t chunk_size_decoded = ntohl(chunk_size);
	if((send(mysocket, &chunk_size, sizeof(uint64_t), 0)) > 0) {
		#ifdef __linux__
		if (DEBUG) printf("Successfully sent chunk size of %lu\n", chunk_size_decoded);
		#endif
		#ifdef __APPLE__
		if (DEBUG) printf("Successfully sent chunk size of %llu\n", chunk_size_decoded);
		#endif
	} else {
		fprintf(stderr, "%s", "There was an error sending chunk size\n");
	}

	//Get the base file name (Removes directory information)
 	base_file = basename(filename);
	//Send filename to server in 21 bytes (chars)
	if((send(mysocket, base_file, sizeof(char)*21, 0)) > 0) {
		if (DEBUG) printf("Successfully sent filename '%s' of size %lu\n", base_file, sizeof(char)*21);
	} else {
		fprintf(stderr, "%s", "There was an error sending the filename\n");
	}
	
	//Read from the file and send the data to the server
	do {
		while (strlen(in_buffer) < buffer_len) {
			current_char = fgetc(input_file);
			if (feof(input_file)) {
		      break;
		    }
			AppendChar(in_buffer, current_char);
		}
		

		msg = in_buffer;
		int orig_length = strlen(msg);
		int sent_length;

		if(((sent_length = send(mysocket, msg, strlen(msg), 0))) > 0) {
			if (DEBUG) printf("Sent %d bytes.\n", sent_length);
			total_bytes_sent += sent_length;
			len = 0;
			//Waits until has recieved all information before continuing
			//This section is responsible for holding sending of new information until all information has returned from server
			chunk_bytes_recieved = 0;
			while (chunk_bytes_recieved < orig_length) {
				len += recv(mysocket, &tmp_chunk_bytes_recieved, sizeof(int), 0);
				chunk_bytes_recieved += tmp_chunk_bytes_recieved;
			}
			total_bytes_recieved += chunk_bytes_recieved;
			if (DEBUG) {
				int difference = orig_length-chunk_bytes_recieved;
				if (chunk_bytes_recieved == orig_length) printf("Received %d byte confirmations.\n", chunk_bytes_recieved);
				else printf("There was a difference of %d bytes\n", difference);
			}
			
		} else {
			if (DEBUG) printf("Recieved no byte confirmations\n");
			printf("Connection lost...\n");
			return EXIT_FAILURE;
		}
		strcpy(in_buffer, "\0");

		if (feof(input_file)) {
	      break;
	    }
	} while (1);
	
	close(mysocket);

	printf("Total bytes sent during this transfer: %d\n", total_bytes_sent);
	printf("Total byte confirmations recieved during this transfer: %d\n", total_bytes_recieved);
	return EXIT_SUCCESS;
}

/**
 * AppendChar
 * Function to append a character to the end of a string
 * @param char * string_s
 * @param char c
 */
void AppendChar(char* string_s, char c) {
        int length = strlen(string_s);
        string_s[length] = c;
        string_s[length+1] = '\0';
}

/**
 * ArgErrorPring
 * Function to output error message of incorrect server name syntax
 */
void ArgErrorPrint() {

	fprintf(stderr, "%s", "\n**Please specify a server and port name on the command line in the correct format\n**An example call would be in the form './client server-IP-address:port-number\n\n'");
}
/**
 * main
 * Calls the program
 * @param int argc
 * @param char * argv[]
 */
int main(int argc, char * argv[]) {
	printf("----------------------------------------------------\n");
	int status = ConnectToServer(argc, argv);
	printf("----------------------------------------------------\n");
	return status;
	
}

