#include <stdio.h> 
#include <string.h> 
#include <sys/types.h>
#include <arpa/inet.h>

#include "rsocket.h"


#define MAXLINE 100 
#define ROLLNO 10012


int main(int argc,  char **argv ) { 
  
    struct sockaddr_in addr_1, addr_2; 
    socklen_t len; 
    char buffer[MAXLINE] ;

    memset(&addr_1, 0, sizeof(addr_1)); 
    memset(&addr_2, 0, sizeof(addr_2)); 

    // Creating socket file descriptor 
    int sockfd = r_socket(AF_INET, SOCK_MRP, 0);
    if ( sockfd < 0 ) { 
        perror("socket creation failed"); 
        exit(EXIT_FAILURE); 
    } 

    addr_2.sin_addr.s_addr = INADDR_ANY; 
    addr_2.sin_family = AF_INET;
    addr_2.sin_port = htons(5000 + 2 * ROLLNO);
   

    if (r_bind(sockfd, (const struct sockaddr *)&addr_2, sizeof(addr_2)) < 0 ) { 
        perror("bind failed"); 
        r_close(sockfd);
        exit(EXIT_FAILURE); 
    }
  
    len = sizeof(addr_1);
    for(int i = 0;i<MAXLINE;i++){  
        int n = r_recvfrom(sockfd, (char *)buffer, MAXLINE, 0,  ( struct sockaddr *) &addr_1, &len);
        printf("%s\n", buffer); 
    }
    r_close(sockfd);
    return 0; 
} 
