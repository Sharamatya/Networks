/* TCP server program */

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




void main(){
	int server_socket_fd, new_socket_fd, portno, clilen;
	
	struct sockaddr_in server_addr, client_addr;

	bzero((char *)&server_addr, sizeof(server_addr));
	bzero((char *)&client_addr, sizeof(client_addr));
	// creat socket with ip = local m/c , scoket type = tcp
	// and check if its created
	server_socket_fd = socket(AF_INET, SOCK_STREAM, 0);
	if( server_socket_fd < 0){
		error ( " Socket creation failed ");
	}


	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(8084);
	server_addr.sin_addr.s_addr = INADDR_ANY;
	// bind socket to server address
	if( bind(
			server_socket_fd, 
			(struct sockaddr *)&server_addr, 
			sizeof(server_addr)) < 0
		){
		close(server_socket_fd);
		error( "Binding failed");
		
	}

	printf( " Server Running....\n");
	// listen to max 5 clients
	listen ( server_socket_fd, 5); 

	clilen = sizeof(client_addr);
	// accept connection from client, 
	// saves clilen , client address in respective
	// structures 
	// NOTE : don't use COnst in accept parameters	
	if ( (new_socket_fd = accept( server_socket_fd, (struct sockaddr *)&client_addr, &clilen ) ) < 0 ){
		close(server_socket_fd);
		error ( "Accept Failure" );

	}


	char buffer[MAX_SIZE];
	int nbytes;

	//recieve file name from server
	if( (nbytes = recv(new_socket_fd, buffer, MAX_SIZE, 0))<=0){
		close(server_socket_fd);
		close(new_socket_fd);
		error(" Invalid File Name recieved");
	}

	//nbytes = recieve_all(new_socket_fd, buffer);

	buffer[nbytes] = '\0';
	// printf("recv.len = %d, len = %d, buffer[] = %s \n",nbytes,strlen(buffer),buffer);
	int file_fd;
	if( (file_fd = open(buffer,O_RDONLY, 0)) < 0){
			close(new_socket_fd);
			close(server_socket_fd);
			error( "File open failed");
	} 

	// read until EOF or Error
	bzero(buffer, sizeof(buffer));
	while( ( nbytes = read(file_fd, buffer, MAX_SIZE)) > 0 ){
		send( new_socket_fd, buffer, MAX_SIZE, 0); 	// send whole buffer
		bzero(buffer, sizeof(buffer));
	}
	close(server_socket_fd);
	close(new_socket_fd);
	close(file_fd);


}