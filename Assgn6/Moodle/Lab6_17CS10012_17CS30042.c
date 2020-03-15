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
#include <signal.h>
#define RESET 0
#define CONNECT 1
#define GET 2
#define POST 3
#define SELF 4

const int MAX_SIZE = 6000;
const int MAX_CON = 500;
#define STDIN 0

void error( char *msg )
{
	perror( msg );
	exit(0);
}
int max(int a, int b)
{
	return (a>b)?a:b;
}

void debug(char* flag)
{
	printf("%s\n",flag );
}


// fsm to the http header
int parsr(char* request,int* method_type, struct hostent* ip,int* port, char* charip, char* path_ptr, char* head, int* is10)//,  int* pathsze)
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
	char* path;
	char* modify;
	// GET THE METHOD
	 if( (method = strstr(request, "POST")) != NULL )
	{
		*method_type = POST;						// 0=GET, 1=POST
		while(*(method)!= ' ') method+=1;		// get to the method value
		path = method + 1;

		if( (method = strstr( request, "http://"))!= NULL)
		{
			for(int k=0; k<7; k++)
			{
				method++;
			}
			while( (*method) != '/' )method++;

			path = method;

		}

		path_size = 0;
		while(*(path+path_size) != ' ' && *(path+path_size)!= '\n' && *(path+path_size)!= '\r' ) path_size++;

		if( strncpy( path_ptr, path, path_size) == NULL) perror("ERROR IN STRING CPY OPERATION 1");

		if(strstr(request, "http://10.")!= NULL)
		{

			char end[20];
			for(int i=0;i<20;i++)
				end[i]='\0';
			path_size++;
			int end_size=path_size;
			while(*(path+path_size) != ' ' && *(path+path_size)!= '\n' && *(path+path_size)!= '\r' ) path_size++;
			if( strncpy( end, path+end_size, path_size-end_size) == NULL) perror("ERROR IN STRING CPY OPERATION 1");  
			strcpy(head, "POST ");
			strcat( head, path_ptr);
			strcat(head," ");
			strcat(head, end );
			head[6+path_size] = '\r';
			head[7+path_size] = '\n';
		}
		else *is10 = 0;

	}
	else if( (method = strstr(request, "GET")) != NULL )
	{
		*method_type = GET;						// 0=GET, 1=POST
		while(*(method)!= ' ') method+=1;		// get to the method value
		path = method + 1;

		if( (method = strstr( request, "http://"))!= NULL)
		{
			for(int k=0; k<7; k++)
			{
				method++;
			}
			while( (*method) != '/' )method++;

			path = method;
		}


		path_size = 0;
		while(*(path+path_size) != ' ' && *(path+path_size)!= '\n' && *(path+path_size)!= '\r' ) path_size++;

		if( strncpy( path_ptr, path, path_size) == NULL) perror("ERROR IN STRING CPY OPERATION 1");

		if(strstr(request, "http://10.")!=NULL)
		{

			char end[20];
			path_size++;
			for(int i=0;i<20;i++)
				end[i]='\0';
			int end_size=path_size;
			while(*(path+path_size) != ' ' && *(path+path_size)!= '\n' && *(path+path_size)!= '\r' ) path_size++;
			if( strncpy( end, path+end_size, path_size-end_size) == NULL) perror("ERROR IN STRING CPY OPERATION 1");  
			strcpy(head, "GET ");
			strcat( head, path_ptr);
			strcat(head," ");
			strcat(head, end );
			head[5+path_size] = '\r';
			head[6+path_size] = '\n';
		}
		else *is10 = 0;

	}
	else 
	{
		*method_type = -1;
		return -1;
	}

	if( (method = strstr(request, "Host")) == NULL )
		return -1;
	while(*(method)!= ' ') method++;		// get to the method value
	method++;								// ptr to method value
	host_ip = method;
	int port_flag=0;						// if port is in request or not
	while( *(method+hostip_size)!= ':' && *(method+hostip_size)!= ' '&& *(method+hostip_size)!= '\r'&& *(method+hostip_size)!= '\n')
	{
		hostip_size++;			// get the host value size
	}
	if(*(method+hostip_size)== ':') port_flag = 1;	
	if(port_flag == 1)
	{
		host_port = host_ip + hostip_size + 1;
		while( host_port[hostport_size]!= '\n' && host_port[hostport_size]!= '\r' && host_port[hostport_size]!= ' ') 
		{
			hostport_size++;			// get the host port size
		}
		char hostip[100], hostport[20];
		if( strncpy( hostip, host_ip, hostip_size) == NULL) perror("ERROR IN STRING CPY OPERATION 1"); 
		if( strncpy( hostport, host_port, hostport_size) == NULL) perror("ERROR IN STRING CPY OPERATION 2"); 
		hostip[hostip_size]='\0';
		hostport[hostport_size]='\0';
		printf("hostip:%s \n",hostip);
		if( (ip = gethostbyname(hostip) ) == NULL) 
		{
			perror("ERROR IN GETTING IP");
			return -1;
		} 
		*port = atoi(hostport);
		char *charip1;
		charip1 = inet_ntoa(*((struct in_addr*) ip->h_addr_list[0])); 
		if(!(charip1))exit(1);
		strcpy(charip, charip1);
	}
	else
	{
		char hostip[100];
		if( strncpy( hostip, host_ip, hostip_size) == NULL) perror("ERROR IN STRING CPY OPERATION 1"); 
		hostip[hostip_size]='\0';
		if( (ip = gethostbyname(hostip) ) == NULL)
		{
			perror("ERROR IN GETTING IP");
			return -1;	
		}  
		*port =  80;
		printf("hostip:%s \n",hostip);

		char *charip1;
		charip1 = inet_ntoa(*((struct in_addr*) ip->h_addr_list[0])); 
		if(!(charip1)) printf("charip null \n");
		strcpy(charip, charip1);
	}
	return 0;
}




int main(int argc,char *argv[])
{
	sigaction(SIGPIPE, &(struct sigaction){SIG_IGN}, NULL);

	int udp_socket_fd,tcp_socket_fd, portno, clilen,port,opt = 1;
	char connec[MAX_CON][MAX_SIZE];
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
	if( tcp_socket_fd == -1){
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


	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(port);
	server_addr.sin_addr.s_addr = htonl(INADDR_ANY);

	// bind socket to server address
	if( bind(tcp_socket_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
	{
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
		insti_fd[i]=-1;
		browser_fd[i]=-1;
	}

	
	int max =0 , conn_count =0 ;		// conn_count is the number of connections till now
	char input[MAX_SIZE];
	int method_type[MAX_CON],conn_established[MAX_CON];
	
	for(int i=0; i< MAX_CON; i++)
	{
		conn_established[i]= -1;
	}

	struct timeval timeout;
    timeout.tv_sec  = 0;
    timeout.tv_usec = 1;

	while(conn_count < MAX_CON-1)
	{
		FD_ZERO(&fd_read);
		FD_ZERO(&fd_send);
		FD_SET( STDIN, &fd_read);
		FD_SET( tcp_socket_fd, &fd_read);
		max = tcp_socket_fd;
		for(int i = 0; i < MAX_CON ; i++)
		{
			if(conn_established[i]> -1)
			{
				FD_SET( insti_fd[i], &fd_read);
				FD_SET( browser_fd[i], &fd_read);
				FD_SET( insti_fd[i], &fd_send);
				FD_SET( browser_fd[i], &fd_send);
				max = ( max >  insti_fd[i]) ? max : insti_fd[i] ;
				max =(max > browser_fd[i])? max : browser_fd[i] ;
			}
			
		}

		max++;				// max file descriptor
		if(select(max, &fd_read, &fd_send, NULL, &timeout)<0)
			perror("Select failed");
		if(FD_ISSET(STDIN, &fd_read))
		{
			scanf("%s", input);
			if( strcmp(input,"exit") == 0)
			{		
				break;
			}
		}

		else if(FD_ISSET(tcp_socket_fd, &fd_read) && conn_count < MAX_CON)
		{
			clilen = sizeof(client_addr);
			browser_fd[conn_count] = accept( tcp_socket_fd, (struct sockaddr *)&client_addr, &clilen );
			if( browser_fd[ conn_count] == -1 )
			{		
				perror("ACCEPT FAILED");
				break;
			}

			char str[100]; 
            inet_ntop(AF_INET, &(client_addr.sin_addr), str, 100);
			printf("Connection accepted from %s:%d \n",str,(int)ntohs(client_addr.sin_port));
			conn_established[conn_count] = 0;
			for( int j = 0; j<MAX_SIZE; j++)connec[conn_count][j] = '\0';		// initialize the buffer to recieve the request from connection in

			conn_count++;
		}

		for( int i = 0; i< conn_count ; i++)
		{ 
			if(FD_ISSET(browser_fd[i], &fd_read))
			{
				char charip[MAX_SIZE], path[MAX_SIZE], message[MAX_SIZE];
				char head[MAX_SIZE];
				struct hostent ip;
				int port,msg_len, path_size, is10 = 1;

				for( int j = 0; j<MAX_SIZE; j++)message[j] = '\0';				// intialize buffer to null
				msg_len = read( browser_fd[i], message, sizeof(message));	// recv from browser i
				strcat(connec[i],message);										

				if( strstr(message, "\r\n\r\n")==NULL)
				{
					continue; 					// if the complete request hasn't been read yet loop again over the outer for loop

				}
				is10 = 1;
				
			    for( int j = 0; j<MAX_SIZE; j++)path[j] = '\0';			// intialize path to al null, to send as an argument to parser()
				for( int j = 0; j<MAX_SIZE; j++)head[j] = '\0';	
				int ret = parsr(connec[i], &(method_type[i]), &ip, &port, charip, path, head, &is10);

				if( is10 == 1)
				{
					char temp[MAX_SIZE];
					strcpy(temp, head);
					int index=0;
					while(connec[i][index]!= '\n' && connec[i][index]!= '\r')
					{
						index++;
					}
					// index++;
					strcat(temp, connec[i]+index);

					strcpy(connec[i],temp);
				}

				if(ret == -1)
				{
					printf("CONNECTION %d : INVALID METHOD TYPE REQUESTED\n", i);
					// close(browser_fd[i]);		// if the method request is not one of the GET/POST methods, close the connection and continue listening to others
					// conn_established[i]=-2;
					continue;
				}
				if (inet_pton(AF_INET, charip, &intiserver.sin_addr.s_addr) <= 0)
			    {	
			    	printf("CONNECTION %d : ", i);
			        perror("INVALID IP ADDRESS \n"); 				// if invalid ip is requested close the browser connection and continue listening to others
			        continue;
			    }

				intiserver.sin_family = AF_INET;
				intiserver.sin_port = htons(port);
				// establish corresponding new connection to inti_fd				
				insti_fd[i] = socket(AF_INET, SOCK_STREAM, 0);
				if( insti_fd[i] < 0)
				{
					printf("CONNECTION %d : ", i);
					perror ( "SOCKET CREATION FAILED \n");			// if socket to server can't be is requested close the browser connection and continue listening to others
			        continue;

				}
			    // set non-blocking
				if (fcntl(insti_fd[i], F_SETFL, O_NONBLOCK) == -1) 
				{
					printf("CONNECTION %d : ", i);
					perror ("NON-BLOCKING FAILED \n");				// if socket can't be set to non-blocking close the browser connection and continue listening to others
			        continue;
			    }
			    if (setsockopt(insti_fd[i], SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)))
			    {
					printf("CONNECTION %d : ", i);
					perror ( "SETTING SOCKET PARAMS FAILED \n");// if socket parameters can't be set close the browser connection and continue listening to others
			        continue;
			    }
				connect( insti_fd[i], (struct sockaddr *)&intiserver, sizeof(intiserver) );
				switch(method_type[i])
				{
					case GET : printf("GET ip: %s, port : %d, path : %s \n", charip, port, path);break;
					case POST : printf("POST ip: %s, port : %d, path : %s \n", charip, port, path);break;
					default : printf("Can't handle other than GET/POST");
				}
				conn_established[i] = 1;			// change to next state on succesful completion

			}
			if(FD_ISSET(insti_fd[i],&fd_send))
			{
				int err = send( insti_fd[i], connec[i], strlen(connec[i]),0);
				for(int j = 0; j<MAX_SIZE; j++)connec[i][j] = '\0';	
				if(err==-1&&errno == EPIPE)
					continue;
			}
			if(FD_ISSET(browser_fd[i], &fd_send)&&FD_ISSET(insti_fd[i],&fd_read))
			{
				char message[MAX_SIZE];
				for( int i = 0; i<MAX_SIZE; i++)message[i] = '\0';	
				int msg_len=0, len;
				msg_len = read( insti_fd[i], message, sizeof(message));	// recv from browser i
				if(msg_len>0)
				{
					int err;
					if( err=send( browser_fd[i], message, msg_len, 0) == -1)
					{
						if(errno == EPIPE)
						{
							close(insti_fd[i]);
							continue;
						}
					}
				}
			}		
		}		
	}
	for(int i=0;i<conn_count;i++)
	{
		if(conn_established[i]>-1)
		{
			close(browser_fd[i]);
		}	
		
	}
}

