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
#include <dirent.h>
#include <sys/wait.h>
#include <netdb.h>
#include <errno.h>

const int MAX_SIZE = 100000;
const int MAX_CON = 1000;

#define STDIN 0

void error( char *msg ){
		perror( msg );
		exit(0);
}

int max(int a, int b){
	return (a>b)?a:b;
}

int main(int argc,char *argv[])
{

	int udp_socket_fd,tcp_socket_fd, portno, clilen,port,opt = 1;
	if(argc < 4)
	{
		printf("Insufficient arguments provided\n");
		exit(1);
	}
	port = atoi(argv[1]);	
	struct sockaddr_in server_addr, client_addr, intiserver;

	memset((char *)&server_addr,0, sizeof(server_addr));
	memset((char *)&client_addr,0, sizeof(client_addr));
	memset((char *)&intiserver,0, sizeof(intiserver));
	// creat socket with ip = local m/c , scoket type = tcp
	// and check if its created
	tcp_socket_fd = socket(AF_INET, SOCK_STREAM, 0);
	if( tcp_socket_fd < 0){
		error ( " tcp Socket creation failed ");
	}
	if (fcntl(tcp_socket_fd, F_SETFL, O_NONBLOCK) == -1) {
        perror("Could not make non-blocking socket\n");
        exit(EXIT_FAILURE);
    }
    if (setsockopt(tcp_socket_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)))
    {
        perror("Setsockopt failure for socket in\n");
        exit(EXIT_FAILURE);
    }
	// set server address

	if (inet_pton(AF_INET, argv[2], &intiserver.sin_addr.s_addr) <= 0)
    {
        perror("Invalid proxy server address\n");
        exit(EXIT_FAILURE);
    }
	intiserver.sin_family = AF_INET;
	intiserver.sin_port = htons(atoi(argv[3]));

	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(port);
	server_addr.sin_addr.s_addr = INADDR_ANY;

	// bind socket to server address
	if( bind(
			tcp_socket_fd, 
			(struct sockaddr *)&server_addr, 
			sizeof(server_addr)) < 0
		){
		close(tcp_socket_fd);
		perror( "tcp Binding failed");
		
	}
	listen ( tcp_socket_fd, MAX_CON); 			// listen to at max MAX_SIZE connections
	
	printf( " Server Running....\n");

	int insti_fd[MAX_CON];	// decalre array to store fds of outgoing connections to intitute server [same]
	int browser_fd[MAX_CON];	// declare fds for incoming browser connections
	fd_set fd_read;			// set of fds
    fd_set fd_send;
	FD_ZERO(&fd_read);
	FD_ZERO(&fd_send);
	for(int i = 0; i < MAX_CON ; i++)
	{
		insti_fd[i]= -1;
		browser_fd[i]=-1;
	}
		printf("Hello from the other side\n");

	
	int max =0 , conn_count =0 ;		// conn_count is the number of connections till now
	char input[MAX_SIZE];
	while(1)
	{

		FD_SET( STDIN, &fd_read);
		FD_SET( tcp_socket_fd, &fd_read);
		printf("Hello from the other side\n");

		max = tcp_socket_fd;
		printf("12\n");
		for(int i = 0; i < MAX_CON ; i++)
		{
			FD_SET( insti_fd[i], &fd_read);
			FD_SET( browser_fd[i], &fd_read);
			FD_SET( insti_fd[i], &fd_send);
			FD_SET( browser_fd[i], &fd_send);
			max = ( max >  insti_fd[i]) ? max : insti_fd[i] ;
			max =(max > browser_fd[i])? max : browser_fd[i] ;

		}

		max++;				// max file descriptor
		printf("2\n");
		if(select(max, &fd_read, &fd_send, NULL, NULL)<=0)
			perror("Select failed");
		printf("3\n");
		if(FD_ISSET(STDIN, &fd_read))
		{
			scanf("%s", input);
			if( strcmp(input,"exit") == 0)break;
		}

		if(FD_ISSET(tcp_socket_fd, &fd_read) && conn_count < MAX_CON)
		{
			printf("9\n");
			browser_fd[conn_count] = accept( tcp_socket_fd, (struct sockaddr *)&client_addr, &clilen );
			if( browser_fd[ conn_count] == -1 )
			{		
				perror("ACCEPT FAILED");
				// exit(1);
			}

			// if browser connection is established
			// establish corresponding new connection to inti_fd
			insti_fd[conn_count] = socket(AF_INET, SOCK_STREAM, 0);
			if( insti_fd[conn_count] < 0){
				perror ( " tcp Socket creation failed ");
			}
			if (fcntl(insti_fd[conn_count], F_SETFL, O_NONBLOCK) == -1) {
		        perror("Could not make non-blocking socket\n");
		        exit(EXIT_FAILURE);
		    }
		    printf("10\n");

		    if (setsockopt(insti_fd[conn_count], SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)))
		    {
		        perror("Setsockopt failure for socket in\n");
		    }

			if(connect( insti_fd[conn_count], (struct sockaddr *)&intiserver, sizeof(intiserver) )<0)
				perror("INSTI SERVER CONNECTION FAILED");
			printf("4\n");
			conn_count++;

		}

		for( int i = 0; i< conn_count ; i++)
		{  // iterate over already establised connecitons	
			if(FD_ISSET(browser_fd[i], &fd_read)&&FD_ISSET(insti_fd[i],&fd_send))
			{
				/* 
					do fcntl recv, send to inti_fd[i]
				*/

				// recv from browser i
				printf("8\n");
				char buffer[MAX_SIZE];
				for( int i = 0; i<MAX_SIZE; i++)buffer[i] = '\0';			// intialize buffer to null
				int msg_len = read( browser_fd[i], buffer, sizeof(buffer));	// recv from browser i
				if(msg_len < 0 )
					perror("Error: Reading failed from browser");
				if(msg_len==0)
				{
					perror("Empty Message");
					continue;
				}
				// while( msg_len >= 0)
				// {
					
				// 	// send to inti server i 
					
				// 	// for( int i = 0; i<MAX_SIZE; i++)buffer[i] = '\0';			// intialize buffer to null
				// 	if(msg_len == 0 )
				// 	{											// if connection closed
				// 		printf("conneciton Closed on browser : %d\n", i);
				// 		break;
				// 	}

				// 	for( int i = 0; i<MAX_SIZE; i++)buffer[i] = '\0';			// intialize buffer to null

				// 	msg_len = recv( browser_fd[i], buffer, sizeof(buffer), 0);
				// }

				send( insti_fd[i], buffer, msg_len, 0);				// send to inti connection
				// {
				// 	perror("SENDING FAILED TO INSTITUTE");
				// }
				if(errno == EPIPE)
					continue;
				printf("5\n");
			}

			if(FD_ISSET(browser_fd[i], &fd_send)&&FD_ISSET(insti_fd[i],&fd_read))
			{
				/* 
					do fcntl recv, send to browser_fd[i]
				*/
				printf("7\n");
				char buffer[MAX_SIZE];
				for( int i = 0; i<MAX_SIZE; i++)buffer[i] = '\0';			// intialize buffer to null
				int msg_len = read( insti_fd[i], buffer, sizeof(buffer));	// recv from browser i
				if(msg_len < 0 )
					perror("Error: Reading failed from institue");
				printf("11\n");
				if(msg_len==0)
				{
					perror("Empty Message");
					continue;
				}
				printf("%d %d\n",conn_count, i);
				// while( msg_len >= 0)
				// {
					
				// 	// send to inti server i 
					
				// 	// for( int i = 0; i<MAX_SIZE; i++)buffer[i] = '\0';			// intialize buffer to null
				// 	if(msg_len == 0 )
				// 	{											// if connection closed
				// 		printf("conneciton Closed on browser : %d\n", i);
				// 		break;
				// 	}

				// 	for( int i = 0; i<MAX_SIZE; i++)buffer[i] = '\0';			// intialize buffer to null

				// 	msg_len = recv( browser_fd[i], buffer, sizeof(buffer), 0);
				// }

				send( browser_fd[i], buffer, msg_len, 0);
				if(errno == EPIPE)
					continue;
				printf("6\n");
			}

		}
	}
}