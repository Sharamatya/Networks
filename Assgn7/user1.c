#include <stdio.h> 
#include <string.h>
#include <arpa/inet.h>
 
#include "rsocket.h"

#define MAXLINE 100 
#define ROLLNO 10053

struct sockaddr_in m2_addr; 
socklen_t len; 
char buffer[MAXLINE] ;

int main(int argc,  char **argv ) { 
  
    // Creating socket file descriptor 
    // printf("dej\n");
    int sockfd = r_socket(AF_INET, SOCK_MRP, 0);
    if ( sockfd < 0 ) { 
        perror("socket creation failed"); 
        exit(1); 
    } 
    // printf("cdc\n");
    scanf("%s",buffer);
    memset(&m2_addr, 0, sizeof(m2_addr)); 

    m2_addr.sin_family = AF_INET; 
    m2_addr.sin_port = htons(5000+2*ROLLNO); 
    m2_addr.sin_addr.s_addr = INADDR_ANY; 
    // printf("done\n");
    // printf("Strlen=  %d\n", strlen(buffer));
    for(int i = 0;i<strlen(buffer);i++){  
        len = sizeof(m2_addr);
        printf("[user1] len = %d\n", len);
        int t = r_sendto(sockfd, (const char *)(buffer+i), 1, MSG_DONTWAIT,(const struct sockaddr *) &m2_addr, len);
        // printf("%d \n",t);
    }
    // printf("bas\n");
    r_close(sockfd); 
    return 0; 
} 
