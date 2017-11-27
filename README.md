# Threaded TCP File Transfer Program

- A client/server combination allowing users to transfer files from multiple clients to a central server

## Running the code
- You can type 'make' or 'make all' to compile the program
- You can type 'make clean' to remove all executables
- You can type 'make remake' to remove all executables and compile the program in one go

## Files Included in this Project

- client.c and server.c
	- The client and server program source code
- serverthreading.c
	- Most of the threading code
- Files folder
	- inputfile.txt is the default file if no others are specified
	- inputfile3.txt is a much longer file (around 1.2 MB) and was used for my testing
	- inputfile4.txt is an even longer file used for A2 testing
- makefile
	- compiles the code
- README.md
	- This lovely file explaining my program to you
- server.h and client.h
	- The the .h files for the server and client
- spawnmultiple.sh
	- The script I used to spawn multiple simultaneous clients to connect to the server
	- Scroll down to the 'Multiple Clients Script and Multiple Client Server Handling' section to learn more
- spawnmultiple.py
	- Python version of script for spawning multiple simultaneous clients

**IMPORTANT**
- The server saves all files to a folder named 'DownloadedFiles'
	- The makefile will attempt to create the folder and give it 777 permissions, and has worked during all my testing
	- If the folder for some reason does not get created prior to running the program, please just create it manually and give it chmod 777 permissions

## Other Specific Information
- If the filename is the first occurrence on the server, it will name it with a (1) appended, and continue to (2), (3), for each occurrence
	- eg. the first file sent to the server named 'filename.txt' will be renamed 'filename.txt(1).txt'
- All data is shared between threads through a linked list of type 'TransferQueue'
- Every time a transfer is completed, the condition variable is signalled, and a separate worker thread removes the record from the queue

## Summary

- The quick tutorial of how to run the code is:
	- The client runs with: *./client server-IP-address:port-number*
	- The server runs with : *./server port-number*
	- Look at the Client Flags documentation for information on flags for the client
	- The server has no flags other than the port number argument

## Client Information
----

You can run the client program with:

	*./client server-IP-address:port-number*

	Please see "Client Flags" section for information on flags and default values

- 'server-IP-address' can be either the hostname of the server you wish to connect to, or the actual IP address
	- for localhost you can type 'localhost' or '127.0.0.1' as ther server-IP-address
- 'port-number' is the port you are running the server on
- The client program will display an error message and exit if the first parameter is not in this format

- The client is capable of return value error handling when creating sockets or connecting to a server
	- If the filename is invalid you will recieve a warning and the program will exit
	- If the server:portnumber is not responding, the client will display a warning message, and automatically retry after 5 seconds repeatedly

- The client will output information about the amount of bytes sent and recieved once the file has been completely sent

- The client program will wait until it has recieved all the bytes that were sent before sending a new buffer
	- If the defined variable DEBUG is set to true, extra information about each buffer transfer is printed to stdout
	- This ensures return value handling as if only a portion of the buffer is recieved, the program will wait until all information has been processed

# Client Flags
----

The client program has optional parameters that can be used

- '-b' flag for setting an optional buffer size
	- EG. '-b 4096'
- '-f' flag for setting the name of the file you wish to read from
	- EG '-f filename.txt'

# Important Flag Information
----

- **These flags all must come after the initial './client server-IP-address:port-number' call**
	- They will only work at the end of the command, trying to call them before will result in errors
- All flags are in the format 'dash letter space argument'
	- EG. '-f filename.txt'
	- A full call would look like './client server-IP-address:port-number -f filename.txt'
- Up to two flags at once are supported, such as:
	- EG. ./client server-IP-address:port-number -f filename.txt -b 4096'
	- You can technically put two of the same flag, however only the second flag's value would be saved
		- Wouldn't recommend it

# Default Values
----

- You may choose to not use flags other than the initial servername and port
	- The default filename is named 'inputfile.txt'
	- The default buffer is 4096 bytes

# Flag Error Handling
----

- The flag parameters support error handling and you will recieve errors for the following:
	- Typing a flag but no value after
	- A non-integer port number
	- A non-integer buffer

- More than two flags are not supported, as there are only two possibilities

## Server Information
----

You can run the server program with:

	*./server port-number*

- ./server is the executable
- port-number is the port you wish to host on
- The default buffer size is 4096, it can only be changed through editing the code

- The server is able to handle multiple clients, and will service them sequentially
- The server will loop text information from one client until that client is finished

# Server Error Handling
----

- The server has error handling for failure to bind to a socket
	- You will recieve an error that there was a problem

- The server will quietly handle other errors such as dropped connections

## Multiple Clients Script and Multiple Client Server Handling
----

- The script I used to run multiple clients simultaneously is names 'spawnmultiple.sh'
	- You can call it by './spawnmultiple.sh' and then any flags you would like in the same format as calling a single client
		- EG. './spawnmultiple.sh -f filename.txt -b 1024'

- The output at the beginning will look a little messy, as the client script outputs a '-------------' at the beginning
- However, you will notice they all finish one at a time and both the sending and recieving happens for each before another begins

- On the server side, you will notice the text will all arrive in the correct order before beginning to service another clientF

## Acknowledgements
----

I would like to give credit to Dr. Denis Nikitenko as his sample code acted as a basis for mine
I would also like to give credit to textfiles.com for the sample text file I used for testing

