// A Simple Client Implementation
#include <stdio.h> 
#include <stdlib.h> 
#include <unistd.h> 
#include <string.h> 
#include <sys/types.h> 
#include <sys/socket.h> 
#include <arpa/inet.h> 
#include <netinet/in.h> 
  const int MAX_SIZE=100;
void main() { 
    int s; 
    struct sockaddr_in servaddr; 
  
    // Creating socket file descriptor 
    s = socket(AF_INET, SOCK_DGRAM, 0);
    if ( s < 0 ) { 
        perror("socket creation failed"); 
        exit(EXIT_FAILURE); 
    } 
  
    memset(&servaddr, 0, sizeof(servaddr)); 
      
    // Server information 
    servaddr.sin_family = AF_INET; 
    servaddr.sin_port = htons(8181); 
    servaddr.sin_addr.s_addr = INADDR_ANY; 
      
    int n;
    socklen_t len = (socklen_t) sizeof(servaddr);
    char filename[50];
    scanf("%s",filename); 
    char buff[20];
    sendto(s, (const char *)filename, strlen(filename), 0, 
			(const struct sockaddr *) &servaddr, sizeof(servaddr));
    printf("Sent: %s\n",filename); 

    int m = recvfrom(s,(char *) buff, MAX_SIZE, 0, (struct sockaddr *) &servaddr, &len);       
    buff[m] = '\0';
    printf("Recieved: %s \n",buff);
    if(strcmp("HELLO",buff)!=0)
    {
        printf("FILE %s Not Found \n",filename);
        exit(EXIT_FAILURE);
    }
    FILE * fout;
    fout = fopen("output.txt","w+");
    int i=1;
    while(1)
    {
        char ret[20]="WORD";

        char num[20];
        sprintf(num, "%d", i++);
        strcat(ret, num);       
        sendto(s,ret, strlen(ret), 0, (struct sockaddr *) &servaddr, sizeof(servaddr));
        printf("Sent : %s\n", ret);

        int m = recvfrom(s,(char *) buff, MAX_SIZE, 0, (struct sockaddr *) &servaddr, &len);
        buff[m] = '\0';
        printf("Recieved: %s \n",buff);
        fprintf(fout,"%s \n",buff);
        if(strcmp(buff, "END") == 0)
            break ;
    }
    fclose(fout);

    close(s); 
} 