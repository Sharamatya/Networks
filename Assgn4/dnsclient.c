// A Simple Client Implementation
#include <stdio.h> 
#include <stdlib.h> 
#include <unistd.h> 
#include <string.h> 
#include <sys/types.h> 
#include <sys/socket.h> 
#include <arpa/inet.h> 
#include <netinet/in.h> 

#define MAXLINE 1024 

void error( char *msg ){
        perror( msg );
        exit(1);
}
  
int main() { 
    int sockfd, port;
    struct sockaddr_in servaddr; 
    printf("Enter PORT and hostname: ");
    scanf("%d", &port);
    // Creating socket file descriptor 
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if ( sockfd < 0 ) { 
        perror("socket creation failed"); 
        exit(EXIT_FAILURE); 
    } 
  
    memset(&servaddr, 0, sizeof(servaddr)); 
      
    // Server information 
    servaddr.sin_family = AF_INET; 
    servaddr.sin_port = htons(port); 
    servaddr.sin_addr.s_addr = INADDR_ANY; 
      
    int n;
    socklen_t len; 
    len =  sizeof(servaddr);
    // char hello[] = "www.google.com";
    char hello[MAXLINE];
    scanf("%s",hello);
      
    sendto(sockfd, (const char *)hello, strlen(hello), 0, 
			(const struct sockaddr *) &servaddr, sizeof(servaddr)); 

    // printf("%s \n",hello); 
    int count=0;
    char buffer[MAXLINE] = "hi";

    while( 1 ){

            n = recvfrom(sockfd, (char *)buffer, MAXLINE, 0, 
            ( struct sockaddr *) &servaddr, &len);    // blocking call
            if( strcmp("error", buffer) == 0){
                error("INVALID HOSTNAME");
                break;

            }
            if(strcmp(buffer,"END")==0)break; 			// if end message is recieved from server side stop recieving	
            printf("IP %d : %s\n",++count,buffer);
    }
     
    // close(sockfd	); 
    return 0; 
} 
