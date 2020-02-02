#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <fcntl.h>

const int MAX_SIZE = 100;

void error( char *msg ){
		perror( msg );
		exit(1);
}


int main(){
	int client_socket_fd, new_socket_fd, portno, clilen,port;
	printf("Enter Port and subdirectory:\n");
	scanf("%d", &port);
	
	struct sockaddr_in server_addr;

	bzero((char *)&server_addr, sizeof(server_addr));
	// creat socket with ip = local m/c , scoket type = tcp
	// and check if its created
	client_socket_fd = socket(AF_INET, SOCK_STREAM, 0);
	if( client_socket_fd < 0){
		error ( " Socket creation failed ");
	}

	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(port);
	server_addr.sin_addr.s_addr = INADDR_ANY;


	// connect to server
	if( connect (client_socket_fd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0 ){
		close(client_socket_fd);
		error ("Connection Failed");

	}

	char buffer[MAX_SIZE];
	int nbytes=0;
	char acknw[] = "DONE";
	// read and send file name to server
	scanf( "%s", buffer);
	size_t filelen = strlen(buffer)+1;
	send( client_socket_fd, buffer, sizeof(buffer), 0);
	int byte_rec,count=0;
	for (int i = 0; i < MAX_SIZE; i++) buffer[i] = '\0';
	recv(client_socket_fd, buffer, MAX_SIZE, 0);
	while (1 )
    {
        if(	strcmp(buffer, "password")==0) {		// if delimiter is indentified icrement count and send acknowledgement
        	printf("new image recieved count :%d  \n", ++count);
        	send( client_socket_fd, acknw, sizeof(acknw), 0);		// acknowledgement from client to server
    	}

    	
        if( strcmp(buffer, "error")==0){							// if server responds with an error message
        	printf("ERROR : INVALID DIRECTORY\n");
        	break;
        }

        for (int i = 0; i < MAX_SIZE; i++) buffer[i] = '\0';		// initialize buffer to all null
        if(recv(client_socket_fd, buffer, sizeof(buffer), 0) < 0)
        	error("RECIEVE FAILED");								// if recv returns error exit



   		if(buffer[0]=='E' && buffer[1]=='N' && buffer[2]=='D' )		// if server sends END message i.e. end of transmission signal
   		{
   			printf("Successfully recieved %d images\n", count);
   			break;													// break the loop and stop recieving data
   		}

    }

	// close(client_socket_fd);						// close the socket
	return 0;
}