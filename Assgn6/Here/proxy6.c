/* TCP server program */
/* Authors: 
		A. Sharma 
		D. Mittal
		*/
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
#define RESET 0
#define CONNECT 1
#define GET 2
#define POST 3
#define SELF 4

const int MAX_SIZE = 10000;
const int MAX_CON = 100;
#define STDIN 0

void error( char *msg ){
		perror( msg );
		exit(0);
}

int max(int a, int b){
	return (a>b)?a:b;
}


// fsm to the http header
int parsr(char* request,int* method_type, struct hostent* ip,int* port, char* charip)
{
	// states_type = 
	// 		0 : RESET
	//		1 : CONNECT
	// 		2 : GET
	// 		3 : POST
	// 		4 : SELF LOOP
	int path_size = 0, hostip_size = 0, hostport_size=0;
	char* host_ip;
	char* host_port;
	char* method;
	// GET THE METHOD
	// if( (method = strstr(request, "CONNECT")) != NULL )
	// {
	// 	*method_type = 1;						// 0=GET, 1=POST
		
	// }
	if( (method = strstr(request, "POST")) != NULL )
	{
		*method_type = 3;						// 0=GET, 1=POST
		while(*(method)!= ' ') method+=1;		// get to the method value
	
	}
	else if( (method = strstr(request, "GET")) != NULL )
	{
		*method_type = 2;						// 0=GET, 1=POST
		while(*(method)!= ' ') method+=1;		// get to the method value

	}
	// else if( (method = strstr(request, "\r\n\r\n")) != NULL )*method_type=0;
	else 
	{
		*method_type = 3;
		return -1;
	}
	if( (method = strstr(request, "Host")) == NULL )
		return -1;
	while(*(method)!= ' ') 
		method+=1;		// get to the method value
	method++;								// ptr to method value
	host_ip = method;
	while( *(method+hostip_size)!= ':')
		hostip_size++;			// get the host value size
	if(*method_type == GET)
	{
		host_port = host_ip + hostip_size + 1;
		while( host_port[hostport_size]!= '\n' && host_port[hostport_size]!= '\r' && host_port[hostport_size]!= ' ') 
		{
			hostport_size++;			// get the host port size
		}
		char hostip[100], hostport[20];
		struct hostent* ip;
		printf("hostip_size: %d Method: %d hostport_size:%d\n",hostip_size,*method_type,hostport_size);
		if( strncpy( hostip, host_ip, hostip_size) == NULL) perror("ERROR IN STRING CPY OPERATION 1"); 
		if( strncpy( hostport, host_port, hostport_size) == NULL) perror("ERROR IN STRING CPY OPERATION 2"); 
		hostip[hostip_size]='\0';
		hostport[hostport_size]='\0';
		// printf("hostip:%s  hostport:%s \n",hostip,hostport);
		if( (ip = gethostbyname(hostip) ) == NULL) perror("ERROR IN GETTING IP"); 

		*port = atoi(hostport);

		// char *charip = ip->h_addr_list[0];
		char *charip1;
		charip1 = inet_ntoa(*((struct in_addr*) ip->h_addr_list[0])); 

		if(!(charip1))exit(1);
		// printf("sent IP %s\n",charip1);

		strcpy(charip, charip1);
	}
	else
	{
		char hostip[100];
		struct hostent* ip;
		printf("hostip_size: %d Method: %d hostport_size:%d\n",hostip_size,*method_type,hostport_size);
		if( strncpy( hostip, host_ip, hostip_size) == NULL) perror("ERROR IN STRING CPY OPERATION 1"); 
		hostip[hostip_size]='\0';
		// printf("hostip:%s  hostport:%s \n",hostip,hostport);
		if( (ip = gethostbyname(hostip) ) == NULL) perror("ERROR IN GETTING IP"); 
		*port = 8080;
		// char *charip = ip->h_addr_list[0];
		char *charip1;
		charip1 = inet_ntoa(*((struct in_addr*) ip->h_addr_list[0])); 
		if(!(charip1))exit(1);
		// printf("sent IP %s\n",charip1);
		strcpy(charip, charip1);
	}
	return 0;
}

int main(int argc,char *argv[])
{

	int udp_socket_fd,tcp_socket_fd, portno, clilen,port,opt = 1;
	
	if(argc < 2)
	{
		printf("Insufficient arguments provided\n");
		exit(1);
	}
	port = atoi(argv[1]);	
	struct sockaddr_in server_addr, client_addr, intiserver, instiserver[MAX_CON];

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
	if( listen ( tcp_socket_fd, MAX_CON) == -1 ) perror("LISTEN ERROR:"); 			// listen to at max MAX_SIZE connections
	
	printf( " Server Running....\n");
	printf("Proxy Running on port %d \n",(int)(ntohs(server_addr.sin_port)));
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

	
	int max =0 , conn_count =0 ;		// conn_count is the number of connections till now
	char input[MAX_SIZE], connec[MAX_CON][MAX_SIZE];
	int method_type[MAX_CON],conn_established[MAX_CON];
	
	for(int i=0; i< MAX_CON; i++)
	{
		conn_established[i]= 0;
	}

	while(1)
	{
		// printf("jfjg\n");
		FD_SET( STDIN, &fd_read);
		FD_SET( tcp_socket_fd, &fd_read);
		max = tcp_socket_fd;
		for(int i = 0; i < conn_count ; i++)
		{
			FD_SET( insti_fd[i], &fd_read);
			FD_SET( browser_fd[i], &fd_read);
			FD_SET( insti_fd[i], &fd_send);
			FD_SET( browser_fd[i], &fd_send);
			max = ( max >  insti_fd[i]) ? max : insti_fd[i] ;
			max =(max > browser_fd[i])? max : browser_fd[i] ;

		}

		max++;				// max file descriptor
		if(select(max, &fd_read, &fd_send, NULL, NULL)<=0)
			perror("Select failed");
		if(FD_ISSET(STDIN, &fd_read))
		{
			scanf("%s", input);
			if( strcmp(input,"exit") == 0)break;
		}

		if(FD_ISSET(tcp_socket_fd, &fd_read) && conn_count < MAX_CON)
		{
			clilen = sizeof(client_addr);

			printf("New Connection to be accepted!!\n");

			browser_fd[conn_count] = accept( tcp_socket_fd, (struct sockaddr *)&client_addr, &clilen );
			if( browser_fd[conn_count] == -1 )
			{		
				perror("ACCEPT FAILED");
				// exit(1);
			}

			if (fcntl(browser_fd[conn_count], F_SETFL, O_NONBLOCK) == -1) {
		        perror("Could not make non-blocking socket\n");
		        // exit(EXIT_FAILURE);
		    }
		    if (setsockopt(browser_fd[conn_count], SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)))
		    {
		        perror("Setsockopt failure for socket in\n");
		        // exit(EXIT_FAILURE);
		    }

			char str[100]; 
            inet_ntop(AF_INET, &(client_addr.sin_addr), str, 100);
			printf("Connection accepted from %s:%d \n",str,(int)ntohs(client_addr.sin_port));
			conn_count++;
			continue;


		
		}

		for( int i = 0; i< conn_count ; i++)
		{ 
			 // iterate over already establised connecitons	
			
			// printf("heyaa!!!\n");

			if(conn_established[i]==0&&FD_ISSET(browser_fd[i], &fd_read))
			{
				printf("enter 0\n");

				char buffer[MAX_SIZE], charip[MAX_SIZE];
				struct hostent ip;
				int port,msg_len;

				for( int j = 0; j<MAX_SIZE; j++)connec[i][j] = '\0';			// intialize connec to  to null

				msg_len = read( browser_fd[i], connec[i], sizeof(connec[i]));	// recv  connect packet from browser i

				if(msg_len < 0)
				{
					perror("Error: Reading failed from browser");
					continue;													// if error move on to next connection available
				}
				else if(msg_len==0)
				{
					perror("Empty Message");
					continue;													// move on to next connection
				}

				printf("%s\n",buffer);											// CONNECT REQUEST

				parsr(buffer, &(method_type[i]), &ip, &port, charip);

				strcpy(charip, "172.16.2.30");									// hardcode the ip
				port = 8080;			

				memset((char *)&(instiserver[i]),0, sizeof(instiserver[i]));		

				if (inet_pton(AF_INET, charip, &(instiserver[i]).sin_addr.s_addr) <= 0)
			    {
			        perror("Invalid proxy server address\n");
			        // exit(EXIT_FAILURE);
			        continue;
			    }

				(instiserver[i]).sin_family = AF_INET;
				(instiserver[i]).sin_port = htons(port);


				// establish corresponding new connection to insti_fd			
				insti_fd[i] = socket(AF_INET, SOCK_STREAM, 0);
			    // printf("1\n");
				if(connect( insti_fd[i], (struct sockaddr *)&intiserver, sizeof(intiserver) )<0 && errno != EINPROGRESS)
				{
					perror("INSTI SERVER CONNECTION FAILED");
					continue;
				}

				// set nonblocking after seting up the connection
				if( insti_fd[i] < 0)
				{
					perror ( " tcp Socket creation failed ");
					continue;
				}
				if (fcntl(insti_fd[i], F_SETFL, O_NONBLOCK) == -1) 
				{
			        perror("Could not make non-blocking socket\n");
			        continue;
			        // exit(EXIT_FAILURE);
			    }
			    if (setsockopt(insti_fd[i], SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)))
			    {
			        perror("Setsockopt failure for socket in\n");
			        continue;
			    }

				printf("Succesfully  connected to ip: %s, port : %d \n", charip, port);
				conn_established[i] = 1;
				printf("done 0 \n");
				break;

			}

			if(conn_established[i]==1&&FD_ISSET(insti_fd[i],&fd_send))
			{
				printf("enter 1\n");
				int err;	

				// send to inti connection
				if( err = send( insti_fd[i], connec[i], sizeof(connec[i]), 0) == -1)
				{
					perror("CONNECTION REQUEST NOT FORWARDED\n");
					continue;
				}
				printf("enter 1.1\n");
				conn_established[i] = 2;
				printf("done 1 %s\n",connec[i]);
				break;
			}

			if(conn_established[i]==2&&FD_ISSET(browser_fd[i], &fd_send)&&FD_ISSET(insti_fd[i],&fd_read))
			{
				printf("enter 2\n");

				char buffer[MAX_SIZE], buffer1[MAX_SIZE];
				
				for( int j = 0; j<MAX_SIZE; j++)buffer[j] = '\0';			// intialize buffer to null

				int msg_len=0, len;
				msg_len = read( insti_fd[i], buffer, sizeof(buffer));	// recv from browser i

				// while(1)
				// {	for( int i = 0; i<MAX_SIZE; i++)buffer1[i] = '\0';
				// 	if(len = read( insti_fd[i], buffer1, sizeof(buffer1))<0) perror("ERROR IN READING FROM BROWSER");	// recv from browser i
				// 	buffer1[len]='\0';
				// 	msg_len+=len;
				// 	strcat(buffer, buffer1);

				// 	if( len < 0 || strstr(buffer, "\r\n\r\n") == NULL)break;
				// }
				if(msg_len < 0 )
				{
					perror("Error: Reading failed from institue");
					continue;
				}
				// else if(msg_len==0)
				// {
				// 	perror("Empty Message PSERVER SIDE");
				// 	break;
		
				// }
				else
				{
					printf("sending from server %d ,to  client : %d\n", i, i);
					printf("%s \n",buffer);
					if(send( browser_fd[i], buffer, MAX_SIZE, 0)==-1)perror("SEND FROM PSERVER FAILED");
					if(errno == EPIPE)
					{
						printf("ERROR IN SENDING\n");
						continue;
					}
				}

				printf("done 2\n");
				break;
			}

			if(conn_established[i] == 2&&FD_ISSET(insti_fd[i],&fd_send)&&FD_ISSET(browser_fd[i],&fd_read))
			{
				// recv from browser i
				char buffer[MAX_SIZE], buffer1[MAX_SIZE];
				printf("enter 3\n");
				// check for the state if 0,1 don't read, if 2 read
				for( int j = 0; j<MAX_SIZE; j++)buffer[j] = '\0';			// intialize buffer to null
				int msg_len=0, len;
				msg_len = read( browser_fd[i], buffer, sizeof(buffer));
				// while(1)
				// {	for( int i = 0; i<MAX_SIZE; i++)buffer1[i] = '\0';
				// 	if(len = read( browser_fd[i], buffer1, sizeof(buffer1))<0) perror("ERROR IN READING FROM BROWSER");	// recv from browser i
				// 	buffer1[len]='\0';
				// 	msg_len+=len;
				// 	strcat(buffer, buffer1);

				// 	if( len < 0 || strstr(buffer, "\r\n\r\n") == NULL)break;
				// }

				if(msg_len < 0 )
					perror("Error: Reading failed from browser");
				else if(msg_len==0)
				{
					perror("Empty Message BROWSER SIDE");
					continue;
				}
				// printf("%s\n",buffer);

				// if(msg_len < 0 )
				// 	perror("Error: Reading failed from browser");
				// else if(msg_len==0)
				// {
				// 	perror("Empty Message");
				// 	// continue;
				// }
				else
				{
					printf("SENT: %s \n",buffer);
	   			 	printf("Sending from client %d ,to  server : %d\n", i, i);

					if(send( insti_fd[i], buffer, msg_len, 0)==-1)				// send to inti connection
					{
						perror("2 send FAILED");
						continue;
					}
					if(errno == EPIPE)
					{
						perror("ERROR IN SENDING");
						continue;
					}
				}

				printf("done 3\n");
				break;
			}
		
		}		
	}
}


