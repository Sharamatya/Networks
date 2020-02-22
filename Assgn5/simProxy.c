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

const int MAX_SIZE = 10;

#define STDIN 0

void error( char *msg ){
		perror( msg );
		exit(0);
}

int max(int a, int b){
	return (a>b)?a:b;
}

int main()
{

	int udp_socket_fd,tcp_socket_fd, portno, clilen,port;

	printf("Enter Port:");scanf("%d", &port);
	
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

	// set server address
	intiserver.sin_family = AF_INET;
	intiserver.sin_port = htons(8080);
	intiserver.sin_addr.s_addr = INADDR_ANY;			// *******************

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
		error( "tcp Binding failed");
		
	}
	listen ( tcp_socket_fd, MAX_SIZE); 			// listen to at max MAX_SIZE connections
	
	printf( " Server Running....\n");

	int insti_fd[MAX_SIZE];	// decalre array to store fds of outgoing connections to intitute server [same]
	int browser_fd[MAX_SIZE];	// declare fds for incoming browser connections
	fd_set fd;			// set of fds

	FD_ZERO(&fd);
	for(int i = 0; i < MAX_SIZE ; i++)
	{
		insti_fd[i]= -1;
		browser_fd[i]=-1;
	}

	
	int max =0 , conn_count =0 ;		// conn_count is the number of connections till now
	char input[MAX_SIZE];
	while(1)
	{

		FD_SET( STDIN, &fd);
		FD_SET( tcp_socket_fd, &fd);

		max = tcp_socket_fd;

		for(int i = 0; i < MAX_SIZE ; i++)
		{
		FD_SET( insti_fd[i], &fd);
		FD_SET( browser_fd[i], &fd);
		max = ( max >  insti_fd[i]) ? max : insti_fd[i] ;
		max =(max > browser_fd[i])? max : browser_fd[i] ;

		}

		max++;				// max file descriptor

		select(max, &fd, NULL, NULL, NULL);

		if(FD_ISSET(STDIN, &fd))
		{
			scanf("%s", input);
			if( strcmp(input,"exit") == 0)break;
		}

		if(FD_ISSET(tcp_socket_fd, &fd))
		{

			browser_fd[conn_count] = accept( tcp_socket_fd, (struct sockaddr *)&client_addr, &clilen );
			if( browser_fd[ conn_count] == -1 )
			{		
				perror("ACCEPT FAILED");
			}

			// if browser connection is established
			// establish corresponding new connection to inti_fd

			insti_fd[conn_count] = connect( tcp_socket_fd, (struct sockaddr *)&intiserver, sizeof(intiserver) );

			if ( insti_fd[conn_count] < 0)
			{
				perror("INSTI SERVER CONNECTION FAILED");
			}

			conn_count++;

		}

		for( int i = 0; i< conn_count ; i++)
		{  // iterate over already establised connecitons	
			if(FD_ISSET(browser_fd[i], &fd))
			{
				/* 
					do fcntl recv, send to inti_fd[i]
				*/

				// recv from browser i

				char buffer[MAX_SIZE];
				for( int i = 0; i<MAX_SIZE; i++)buffer[i] = '\0';			// intialize buffer to null
				int msg_len = recv( browser_fd[i], buffer, sizeof(buffer), 0);	// recv from browser i

				while( msg_len >= 0)
				{
					
					// send to inti server i 
					if (send( insti_fd[i], buffer, msg_len, 0) == -1);						// send to inti connection
						{
							perror("SENDING FAILED TO INSTITUTE FAILED");
						}
					// for( int i = 0; i<MAX_SIZE; i++)buffer[i] = '\0';			// intialize buffer to null
					if(msg_len == 0 )
					{											// if connection closed
						printf("conneciton Closed on browser : %d\n", i);
						break;
					}

					for( int i = 0; i<MAX_SIZE; i++)buffer[i] = '\0';			// intialize buffer to null

					msg_len = recv( browser_fd[i], buffer, sizeof(buffer), 0);
				}


				
			}

			if(FD_ISSET(insti_fd[i], &fd))
			{
				/* 
					do fcntl recv, send to browser_fd[i]
				*/

				char buffer[MAX_SIZE];
				for( int i = 0; i<MAX_SIZE; i++)buffer[i] = '\0';			// intialize buffer to null
				int msg_len = recv( insti_fd[i], buffer, sizeof(buffer), 0);	// recv from browser i

				while( msg_len >= 0)
				{
					
					// send to inti server i 
					if (send( browser_fd[i], buffer, msg_len, 0) == -1);						// send to inti connection
						{
							perror("SENDING FAILED TO INSTITUTE FAILED");
						}
					// for( int i = 0; i<MAX_SIZE; i++)buffer[i] = '\0';			// intialize buffer to null
					if(msg_len == 0 )
					{											// if connection closed
						printf("conneciton Closed on browser : %d\n", i);
						break;
					}

					for( int i = 0; i<MAX_SIZE; i++)buffer[i] = '\0';			// intialize buffer to null

					msg_len = recv( insti_fd[i], buffer, sizeof(buffer), 0);
				}
			}

		}
	}
}