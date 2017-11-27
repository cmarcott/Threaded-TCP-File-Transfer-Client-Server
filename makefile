all:
	gcc -g -pedantic -Wall server.c serverthreading.c -o server -lpthread
	gcc -g -pedantic -Wall client.c -o client
	make folders

clean:
	rm -r -f client.dSYM
	rm -r -f server.dSYM
	rm -f server
	rm -f client
	rm -r -f DownloadedFiles
	
folders:
	rm -r -f DownloadedFiles
	mkdir DownloadedFiles
	chmod 777 DownloadedFiles
	
remake:
	rm -f server
	rm -f client
	rm -r -f client.dSYM
	rm -r -f server.dSYM
	make all
