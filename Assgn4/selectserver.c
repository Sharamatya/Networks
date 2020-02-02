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

const int MAX_SIZE = 100;
const int MAXLINE = 100;

void error( char *msg ){
		perror( msg );
		exit(0);
}

int max(int a, int b){
	return (a>b)?a:b;
}

void main(){

	int udp_socket_fd,tcp_socket_fd, new_socket_fd, portno, clilen,port;

	printf("Enter Port:");scanf("%d", &port);
	
	struct sockaddr_in server_addr, client_addr;

	bzero((char *)&server_addr, sizeof(server_addr));
	bzero((char *)&client_addr, sizeof(client_addr));
	// creat socket with ip = local m/c , scoket type = tcp
	// and check if its created
	tcp_socket_fd = socket(AF_INET, SOCK_STREAM, 0);
	if( tcp_socket_fd < 0){
		error ( " tcp Socket creation failed ");
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
		error( "tcp Binding failed");
		
	}
	listen ( tcp_socket_fd, 10); 
	
	printf( " Server Running....\n");
	udp_socket_fd = socket(AF_INET, SOCK_DGRAM, 0);
	if( udp_socket_fd < 0){
		error ( " udp Socket creation failed ");
	}
	if( bind(
			udp_socket_fd, 
			(struct sockaddr *)&server_addr, 
			sizeof(server_addr)) < 0
		){
		close(udp_socket_fd);
		error( "udp Binding failed");
		
	}

	// listen to max 5 clients
	

	clilen = sizeof(client_addr);

	fd_set fds;

	FD_ZERO(&fds);

	int maxfd;
	maxfd = max(tcp_socket_fd, udp_socket_fd) + 1;


	while(1){

		
		FD_SET(tcp_socket_fd, &fds);

		FD_SET(udp_socket_fd, &fds);
		// printf("\n 4\n");
			
		int n_fd = select ( maxfd, &fds, NULL, NULL, NULL );
		if( FD_ISSET( tcp_socket_fd, &fds) )
		{

			int pid;
			clilen = sizeof(client_addr);
			char message[MAXLINE],buffer[MAXLINE];
			new_socket_fd = accept( tcp_socket_fd, (struct sockaddr*)&client_addr, &clilen);
			// close(tcp_socket_fd);
			printf("\n TCP SERVER RUNNING \n");
			if( (pid = fork()) == 0){
				close(tcp_socket_fd); 
				///////////////////////////////////////////////////// TCP Code ////////////////////////////////////////////////

				char buffer[MAX_SIZE];
				int nbytes,i;

				//recieve file name from server
				if( (nbytes = recv(new_socket_fd, buffer, MAX_SIZE, 0))<=0){
					close(tcp_socket_fd);
					close(new_socket_fd);
					error(" Invalid Directory Name recieved");
				}
				buffer[nbytes] = '\0';
				char addr[]= "image/";
				strcat(addr, buffer);
				printf("Directory : %s \n", addr);

				struct dirent *de;
				DIR *dr = opendir(addr);
				// check for errors in opening directory
				if ( dr == NULL){
					char errr[] = "error";			///  buffer to send if there's an error opening the directory
					send( new_socket_fd, errr, sizeof(errr), 0);
					error("FAILED TO OPEN THE Directory");
				}

				char file[MAX_SIZE],direc[MAX_SIZE],acknw[MAX_SIZE];
				
				char c[] ="END";
				strcpy(direc, addr);
				strcat(direc,"/");

				// read and send all files
				while (( de = readdir(dr))){
					strcpy( file, direc);
					strcat( file, de->d_name);
					// printf("file : %s\n", file);
					if( de->d_name[0] == '.')continue;

					int file_fd;
					if( ( file_fd = open(file,O_RDONLY, 0)) < 0){
							close(new_socket_fd);
							close(tcp_socket_fd);
							error( "File open failed");
					} 

					for(i=0;i<MAX_SIZE;i++)
							buffer[i] = '\0';
					
					while( ( nbytes = read(file_fd, buffer, sizeof(buffer))) >0 ){
						send( new_socket_fd, buffer, sizeof(buffer), 0); 	// send whole buffer
						for(i=0;i<MAX_SIZE;i++)
							buffer[i] = '\0';
					}
					
					printf("SENT : %s \n",file);
				
					char d[] = "password";
					send(new_socket_fd,d,sizeof(d),0);					/// send file delimiter to client
					recv(new_socket_fd, acknw, sizeof(acknw), 0);		/// wait till client ackwnoledges
				}
				
			    send(new_socket_fd,c,sizeof(c),0);						// send finalising message
			    printf("SENT : %s\n", c);

				///////////////////////////////////////////////////// TCP Code ////////////////////////////////////////////////
				close(new_socket_fd);
				exit(0);
			}
			close(new_socket_fd);
		}

		

		if( FD_ISSET( udp_socket_fd, &fds) ){

			///////////////////////////////////////////////////// DNS Code ////////////////////////////////////////////////


		    printf("\n UDP Server Running....\n");
		  
		    int n,errflag = 0;			// flag to set to 1 if there is any error
		    socklen_t len;
		    char buffer[MAXLINE]; 
		 
		    len = sizeof(client_addr);
		    n = recvfrom(udp_socket_fd, (char *)buffer, MAXLINE, 0, 
					( struct sockaddr *) &client_addr, &len);    // blocking call
		    buffer[n] = '\0'; 
		    printf("UDP recieved: %s\n", buffer); 

		    struct hostent *ip;
		    ip = gethostbyname(buffer); 
		    // check if name is valid
		    if( ip == NULL){
		    	
		    	char err[] = "error";

		    	sendto(udp_socket_fd, (const char *)err, strlen(err), 0, 
					(const struct sockaddr *) &client_addr, sizeof(client_addr)); 
		    	printf("INVALID HOSTNAME\n");
		    	errflag = 1; 		// flag set to 1 ; will be used as a condition not to send any message furthur
		    }

		    int count = 0;

		    char *array;
		    if(errflag == 0){
			    do{
			    	if(!(ip->h_addr_list[count]))break;
			    	array = inet_ntoa(*((struct in_addr*) 
			                           ip->h_addr_list[count])); 
			    	if(!array)
			    		break;
			    	printf("sent IP %d : %s\n",++count,array);

			    	sendto(udp_socket_fd, (const char *)array, strlen(array), 0, 
						(const struct sockaddr *) &client_addr, sizeof(client_addr)); 

			    }while ( array != NULL); 
			    char end[] = "END";
			    sendto(udp_socket_fd, (const char *)end, sizeof(end), 0, 
						(const struct sockaddr *) &client_addr, sizeof(client_addr)); 
		    }
		    // printf("\n 3\n");
			///////////////////////////////////////////////////// DNS Code ////////////////////////////////////////////////

		}



	}


}