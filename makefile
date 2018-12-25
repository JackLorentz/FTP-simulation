# target : dependencies
all : server.c client.c FTP_operation.c
	gcc -std=c99 -g -Wall -o server server.c FTP_operation.c
	gcc -std=c99 -g -Wall -o client client.c FTP_operation.c 
clean :
	rm -f server client