#include <stdio.h> 
#include <stdlib.h> 
#include <unistd.h> 
#include <string.h> 
#include <sys/types.h> 
#include <sys/socket.h> 
#include <arpa/inet.h> 
#include <netinet/in.h> 
const int MAX_SIZE=100;
int main()
{
	struct sockaddr_in server_addr,client_addr;
	memset(&server_addr,0,sizeof(server_addr));
	memset(&client_addr,0,sizeof(client_addr));

	int s= socket(AF_INET, SOCK_DGRAM, 0);
	if(s<=-1)
	{		
		printf("socket failed");
		exit(EXIT_FAILURE);
	}
	
	server_addr.sin_family = AF_INET;
	server_addr.sin_addr.s_addr = INADDR_ANY;
	server_addr.sin_port = htons(8181);
	int status = bind(s,(const struct sockaddr *) &server_addr, sizeof(server_addr));
	if(status<=-1)
	{		
		printf("bind failed");
		exit(EXIT_FAILURE);
	}
	
	char buff[MAX_SIZE];
	char read[MAX_SIZE];
	printf("Server is running ...\n");
	socklen_t len = (socklen_t) sizeof(client_addr);
	int n = recvfrom(s,(char *) buff, MAX_SIZE, 0, (struct sockaddr *) &client_addr, &len);
	buff[n] = '\0';
	printf("Recieved %s \n",buff);

	FILE * f = fopen (buff, "r");
	if(f==NULL)
	{
		char fail[50] = "NOTFOUND ";
		strcat(fail,buff);
		sendto(s,fail,strlen(fail) , 0, (struct sockaddr *) &client_addr,len);
		printf("SENT: %s",fail);
		exit(EXIT_FAILURE);
	}
	
	while(fscanf(f, "%s", read) != EOF)
	{
		// test
		printf("Read From File: %s\n", read);

		sendto(s,read, strlen(read), 0, (struct sockaddr *) &client_addr, len);
		printf("Sent :%s\n",read);
		if(strcmp(read, "END") == 0)
			break ;
		int m = recvfrom(s,(char *) buff, MAX_SIZE, 0, (struct sockaddr *) &client_addr, &len);
		buff[m] = '\0';
		printf("Recieved: %s\n", buff);
		
	}
	fclose(f);
	close(s);
	return 0;
}	
