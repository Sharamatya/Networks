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
	int client_socket_fd, new_socket_fd, portno, clilen;
	
	struct sockaddr_in server_addr;

	bzero((char *)&server_addr, sizeof(server_addr));
	// creat socket with ip = local m/c , scoket type = tcp
	// and check if its created
	client_socket_fd = socket(AF_INET, SOCK_STREAM, 0);
	if( client_socket_fd < 0){
		error ( " Socket creation failed ");
	}

	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(8084);
	server_addr.sin_addr.s_addr = INADDR_ANY;


	// connect to server
	if( connect (client_socket_fd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0 ){
		close(client_socket_fd);
		error ("Connection Failed");

	}

	char buffer[MAX_SIZE];
	int nbytes=0;
	
	// read and send file name to server
	scanf( "%s", buffer);
	// strcpy(buffer, "test.txt");
	size_t filelen = strlen(buffer)+1;
	send( client_socket_fd, buffer, filelen, 0);
	
	int read_fd = open("output.txt",O_WRONLY | O_CREAT | O_TRUNC, 0644);
	int del =0,count =0,words=0,byte_rec=0,delim=1,j=0;
	if(read_fd < 0)
	{
		perror("[ERROR] Can't create a file\n");
        close(client_socket_fd);
        exit(1);	
	}

	while ((byte_rec = recv(client_socket_fd, buffer, MAX_SIZE, 0))>0)
    {
        // writing data to the file
        j=0;
        write(read_fd,buffer,strlen(buffer));
        printf("%s", buffer);
        while (j < byte_rec&&buffer[j] != '\0')
        {
            nbytes++;
            // count the number of bytes and words
            if (buffer[j] == ' ' || buffer[j] == '\t' || 
            buffer[j] == '\n' || buffer[j] == ';' || buffer[j] == ',' || 
            buffer[j] == ':' || buffer[j] == '.')
            delim=1;
            
            else
            {
                if(delim==1)
                {
                    words++;
                    delim=0;
                }
            }
            j++;
        }
        // for (int i = 0; i < MAX_SIZE; i++) buffer[i] = '\0';
    }
    if(byte_rec <= 0&& nbytes == 0)
    {
    	printf("File not found");
    	exit(1);
    }
	printf("\nThe file transfer is successful. Words: %d, Bytes: %d\n", words, nbytes );
	close(client_socket_fd);
	close(read_fd);
}