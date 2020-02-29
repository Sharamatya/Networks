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






// fsm to the http header
void parse(char* request,int* method_type, struct hostent* ip,int* port)
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
	if( (method = strstr(request, "CONNECT")) != NULL )
	{
		*method_type = 1;						// 0=GET, 1=POST
		while(*(method)!= ' ') method+=1;		// get to the method value
		method++;								// ptr to method value
		host_ip = method;
		while( *(method+hostip_size)!= ':')hostip_size++;			// get the host value size
		host_port = host_ip + hostip_size + 1;
		while( host_port[hostport_size]!= '\n' && host_port[hostport_size]!= '\r' && host_port[hostport_size]!= ' ') 
		{
			hostport_size++;			// get the host port size
		}
	}
	else if( (method = strstr(request, "POST")) != NULL )
	{
		*method_type = 3;						// 0=GET, 1=POST
		while(*(method)!= ' ') method+=1;		// get to the method value
		
	}
	else if( (method = strstr(request, "GET")) != NULL )
	{
		*method_type = 2;						// 0=GET, 1=POST
		while(*(method)!= ' ') method+=1;		// get to the method value
		
	}
	else if( (method = strstr(request, "\r\n\r\n")) != NULL )*method_type=0;
	else *method_type = 4;
	char hostip[100], hostport[20];
	// struct hostent* ip;
	printf("hostip_size: %d Method: %d hostport_size:%d\n",hostip_size,*method_type,hostport_size);
	if( strncpy( hostip, host_ip, hostip_size) == NULL) perror("ERROR IN STRING CPY OPERATION 1"); 
	if( strncpy( hostport, host_port, hostport_size) == NULL) perror("ERROR IN STRING CPY OPERATION 2"); 
	hostip[hostip_size]='\0';
	hostport[hostport_size]='\0';
	printf("hostip:%s  hostport:%s \n",hostip,hostport);
	if( (ip = gethostbyname(hostip) ) == NULL) perror("ERROR IN GETTING IP"); 

	*port = atoi(hostport);
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

	// if (inet_pton(AF_INET, argv[2], &intiserver.sin_addr.s_addr) <= 0)
 //    {
 //        perror("Invalid proxy server address\n");
 //        exit(EXIT_FAILURE);
 //    }
	// intiserver.sin_family = AF_INET;
	// intiserver.sin_port = htons(atoi(argv[3]));

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
	char input[MAX_SIZE];
	int method_type[MAX_SIZE];
	while(1)
	{

		FD_SET( STDIN, &fd_read);
		FD_SET( tcp_socket_fd, &fd_read);
		max = tcp_socket_fd;
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
			browser_fd[conn_count] = accept( tcp_socket_fd, (struct sockaddr *)&client_addr, &clilen );
			if( browser_fd[ conn_count] == -1 )
			{		
				perror("ACCEPT FAILED");
				// exit(1);
			}
			char str[100]; 
            inet_ntop(AF_INET, &(client_addr.sin_addr), str, 100);
			printf("Connection accepted from %s:%d \n",str,(int)ntohs(client_addr.sin_port));



			// if browser connection is established
			// parse
			char buffer[MAX_SIZE];
			read( browser_fd[conn_count], buffer, sizeof(buffer));
			printf("%s \n",buffer);
			
			struct hostent ip;
			int port;
			parse(buffer, &(method_type[conn_count]), &ip, &port);
			printf("Port:%d length:%d \n",port,ip.h_length);

			char *charip = ip.h_addr_list[0];
			printf("IP:%s  Port:%d \n",charip,port);
			if (inet_pton(AF_INET, charip, &intiserver.sin_addr.s_addr) <= 0)
		    {
		        perror("Invalid proxy server address\n");
		        exit(EXIT_FAILURE);
		    }
			intiserver.sin_family = AF_INET;
			intiserver.sin_port = htons(port);


			// establish corresponding new connection to inti_fd

			
			insti_fd[conn_count] = socket(AF_INET, SOCK_STREAM, 0);
			if( insti_fd[conn_count] < 0)
			{
				perror ( " tcp Socket creation failed ");
			}
			if (fcntl(insti_fd[conn_count], F_SETFL, O_NONBLOCK) == -1) 
			{
		        perror("Could not make non-blocking socket\n");
		        exit(EXIT_FAILURE);
		    }

		    if (setsockopt(insti_fd[conn_count], SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)))
		    {
		        perror("Setsockopt failure for socket in\n");
		    }

			if(connect( insti_fd[conn_count], (struct sockaddr *)&intiserver, sizeof(intiserver) )<0 && errno != EINPROGRESS)
				perror("INSTI SERVER CONNECTION FAILED");
			conn_count++;

		}

		for( int i = 0; i< conn_count ; i++)
		{ 
			 // iterate over already establised connecitons	
			if(FD_ISSET(browser_fd[i], &fd_read)&&FD_ISSET(insti_fd[i],&fd_send))
			{
				// recv from browser i
				char buffer[MAX_SIZE];
				for( int i = 0; i<MAX_SIZE; i++)buffer[i] = '\0';			// intialize buffer to null
				int msg_len = read( browser_fd[i], buffer, sizeof(buffer));	// recv from browser i
				if(msg_len < 0 )
					perror("Error: Reading failed from browser");
				if(msg_len==0)
				{
					// perror("Empty Message");
					continue;
				}

				send( insti_fd[i], buffer, msg_len, 0);				// send to inti connection
				if(errno == EPIPE)
					continue;
			}

			if(FD_ISSET(browser_fd[i], &fd_send)&&FD_ISSET(insti_fd[i],&fd_read))
			{
				char buffer[MAX_SIZE];
				for( int i = 0; i<MAX_SIZE; i++)buffer[i] = '\0';			// intialize buffer to null
				int msg_len = read( insti_fd[i], buffer, sizeof(buffer));	// recv from browser i
				if(msg_len < 0 )
					perror("Error: Reading failed from institue");
				if(msg_len==0)
				{
					// perror("Empty Message");
					continue;
				}
				send( browser_fd[i], buffer, msg_len, 0);
				if(errno == EPIPE)
					continue;
			}

		}
	}
}


