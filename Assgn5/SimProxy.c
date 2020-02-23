#include <unistd.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <signal.h>

#define PORT 8080
#define BUFF_SIZE 1024

int max(int a, int b) {
    return a>b? a : b;
}

int main(int argc, char *argv[]) {

    signal(SIGPIPE, SIG_IGN);

    if (argc != 4) {
        printf("Incorrect command line arguments passed.\nCorrect usage:\n./SimProxy <listen port> <institute_proxy_IP> <institute_proxy_port>\n");
        return 0;
    }

    struct sockaddr_in proxyaddr, inpaddr, cliaddr;
    socklen_t len, len_cli;

    int sock_in, maxfd, nready;

    bzero(&inpaddr, sizeof(inpaddr));
    bzero(&proxyaddr, sizeof(proxyaddr));

    int opt = 1;

    char buff[BUFF_SIZE];
    bzero(&buff, sizeof(buff));

    struct timeval timeout;
    timeout.tv_sec  = 0;
    timeout.tv_usec = 1;

    // Create tcp socket file descriptor
    if ((sock_in = socket(AF_INET, SOCK_STREAM, 0)) == 0)
    {
        perror("Socket in creation failed\n");
        exit(EXIT_FAILURE);
    }

    if (fcntl(sock_in, F_SETFL, O_NONBLOCK) == -1) {
        perror("Could not make non-blocking socket\n");
        exit(EXIT_FAILURE);
    }

    // Set socket options 
    if (setsockopt(sock_in, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)))
    {
        perror("Setsockopt failure for socket in\n");
        exit(EXIT_FAILURE);
    }

    /*
    if (setsockopt (sock_in, SOL_SOCKET, SO_NOSIGPIPE, &opt, sizeof (opt)) < 0) {
        perror ("Unable to set socket to ignore closed sockets\n");
    }
    */

    inpaddr.sin_family = AF_INET;
    inpaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    inpaddr.sin_port = htons(atoi(argv[1]));

    proxyaddr.sin_family = AF_INET;
    proxyaddr.sin_port = htons(atoi(argv[3]));

    // check ip address
    if (inet_pton(AF_INET, argv[2], &proxyaddr.sin_addr) <= 0)
    {
        perror("Invalid proxy server address\n");
        exit(EXIT_FAILURE);
    }


    // Attach socket to port 8080
    if (bind(sock_in, (struct sockaddr*)&inpaddr, sizeof(inpaddr)))
    {
        perror("In socket Bind failure\n");
        exit(EXIT_FAILURE);
    }


    int n = 1000;
    // Marks server_fd as a passive socket, now ready to accept incoming connections using connect()
    if (listen(sock_in, n) < 0)
    {
        perror("Sock in Listen failure\n");
        exit(EXIT_FAILURE);
    }

    fd_set read_set;
    fd_set write_set;
    FD_ZERO(&read_set);
    FD_ZERO(&write_set);

    int infd[n];
    int outfd[n];
    int num_connections = 0;

    while (1) 
    {

        // printf("\rnum_connections: %d", num_connections);
        FD_ZERO(&read_set);
        FD_ZERO(&write_set);
        FD_SET(0, &read_set);
        maxfd = 1;
        FD_SET(sock_in, &read_set);
        maxfd = sock_in;
        // add all connections to FD SET
        for (int i = 0; i < num_connections; i++) 
        {
            FD_SET(infd[i], &read_set);
            FD_SET(outfd[i], &read_set);
            FD_SET(infd[i], &write_set);
            FD_SET(outfd[i], &write_set);
            maxfd = max(maxfd, infd[i]);
            maxfd = max(maxfd, outfd[i]);
        }
        maxfd = maxfd + 1;

        
        nready = select(maxfd, &read_set, &write_set, NULL, &timeout);
        if (nready > 0) 
        {
            char buff[1024];
            int a, b;
            if(FD_ISSET(sock_in, &read_set))
            {
                if (num_connections < n && (infd[num_connections] = accept(sock_in, (struct sockaddr *)&cliaddr, &len_cli)) >= 0) 
                {
                    char str[100]; 
                    inet_ntop(AF_INET, &(cliaddr.sin_addr), str, 100);
                    int clientServerPort = (int) ntohs(cliaddr.sin_port);        
                    printf("Connection accepted from %s:%d\n", str, clientServerPort);
                    // printf("Connection created : %s\n", )

                    if ((outfd[num_connections] = socket(AF_INET, SOCK_STREAM, 0)) == -1) 
                    {
                        fprintf(stdout, "socket() failed: %s\n", strerror(errno));
                        exit(0);
                    }

                    /*
                    if (setsockopt (outfd[num_connections], SOL_SOCKET, SO_NOSIGPIPE, &opt, sizeof (opt)) < 0) {
                        perror ("Unable to set socket to ignore closed sockets\n");
                    }
                    */

                    if (fcntl(outfd[num_connections], F_SETFL, O_NONBLOCK) == -1) 
                    {
                        fprintf(stdout, "fcntl() failed: %s\n", strerror(errno));
                        exit(EXIT_FAILURE);
                    }

                    // cannot connect immediately, needs some time
                    connect(outfd[num_connections], (struct sockaddr *)&proxyaddr, sizeof(proxyaddr));

                    num_connections++;
                }
                // FD_SET(infd[num_connections-1], &read_set);
                // FD_SET(outfd[num_connections-1], &read_set);
                // FD_SET(infd[num_connections-1], &write_set);
                // FD_SET(outfd[num_connections-1], &write_set);
            }

            else if (FD_ISSET(0, &read_set)) 
            {
                memset(buff, 0, sizeof(buff));
                a = read(0, buff, sizeof(1024));
                printf("\ntyped: %s\n", buff);
                if (strcmp(buff, "exit") == 0) 
                {

                    for (int i = 0; i < num_connections; i++) 
                    {
                        close(infd[i]);
                        close(outfd[i]);
                    }
                    close(sock_in);
                    return 0;
                } 
            }
            for (int i = 0; i < num_connections; i++) 
            {

                char buff[1024];
                int a, b;

                if(FD_ISSET(infd[i], &read_set) && FD_ISSET(outfd[i], &write_set)) 
                {
                    memset(buff, 0, sizeof(buff));
                    a = read(infd[i], buff, sizeof(1024));
                    b = send(outfd[i], buff, a, 0);
                    // if (b == -1) {
                    //     switch (errno) 
                    //     {
                    //         case EPIPE:
                    //             // close(infd[i]);
                    //             // close(outfd[i]);
                    //             num_connections--;
                    //     }
                    // }
                    if (errno == EPIPE) 
                    {
                        continue;
                    }
                }

                if(FD_ISSET(outfd[i], &read_set) && FD_ISSET(infd[i], &write_set)) 
                {
                    memset(buff, 0, sizeof(buff));
                    a = read(outfd[i], buff, sizeof(1024));
                    b = send(infd[i], buff, a, 0);
                    // if (b == -1) {
                    //     switch (errno) 
                    //     {
                    //         case EPIPE:
                    //             // close(infd[i]);
                    //             // close(outfd[i]);
                    //             num_connections--;
                    //     }
                    // }
                    if (errno == EPIPE) 
                    {
                        continue;
                    }
                }

            }
        }

    }

    return 0;
}


        // if (num_connections < n && (infd[num_connections] = accept(sock_in, (struct sockaddr *)&cliaddr, &len_cli)) >= 0) {



        //     if ((outfd[num_connections] = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        //         fprintf(stdout, "socket() failed: %s\n", strerror(errno));
        //         exit(0);
        //     }

        //     /*
        //     if (setsockopt (outfd[num_connections], SOL_SOCKET, SO_NOSIGPIPE, &opt, sizeof (opt)) < 0) {
        //         perror ("Unable to set socket to ignore closed sockets\n");
        //     }
        //     */

        //     if (fcntl(outfd[num_connections], F_SETFL, O_NONBLOCK) == -1) {
        //         fprintf(stdout, "fcntl() failed: %s\n", strerror(errno));
        //         exit(EXIT_FAILURE);
        //     }

        //     // cannot connect immediately, needs some time
        //     connect(outfd[num_connections], (struct sockaddr *)&proxyaddr, sizeof(proxyaddr));

        //     num_connections++;
        // }

        // FD_ZERO(&read_set);
        // FD_ZERO(&write_set);
        // FD_SET(0, &read_set);
        // maxfd = 1;
        // // add all connections to FD SET
        // for (int i = 0; i < num_connections; i++) {
        //     FD_SET(infd[i], &read_set);
        //     FD_SET(outfd[i], &read_set);
        //     FD_SET(infd[i], &write_set);
        //     FD_SET(outfd[i], &write_set);
        //     maxfd = max(maxfd, infd[i]);
        //     maxfd = max(maxfd, outfd[i]);
        // }
        // maxfd = maxfd + 1;


        // nready = select(maxfd, &read_set, &write_set, NULL, &timeout);
        // if (nready > 0) {
        //     char buff[1024];
        //     int a, b;