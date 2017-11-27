#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <netdb.h>
#include <errno.h>
#include <signal.h>
#include <ctype.h>
#include <stdbool.h>
#include <time.h>
#include <libgen.h>

#define SUCCESS 1;
#define FAILURE 0;

int CheckInput(char ** full_server_name, char ** server_name, char ** specified_port_number, int argc, char * argv[], int * buffer_len, char ** filename);
int ConnectToServer(int argc, char * argv[]);
void AppendChar(char* s, char c);
void ArgErrorPrint();
int FilenameTooLong(char * filename);
