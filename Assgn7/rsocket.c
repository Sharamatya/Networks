#include "rsocket.h"

// ----------------------------  strcutures and global vars ---------------------------- 
socklen_t recv_addr_len = 0;
int MRP_fd = -1, seq_num = 0, recv_flags = 0, begin_recvbuf = 0, end_recvbuf = 0, size_buf = 0, begin_sendbuf = 0, end_sendbuf = 0, total_transmitted = 0, total_retransmitted =0, dup_msgs =0, dup_ACKS=0;
struct sockaddr_in recv_source_addr;

//sender buffer contains the application messages that need to be sent
struct send_buffer_item
{
    int seq_number;
    char buffer[MSG_SIZE];
    size_t len;
    struct sockaddr_in destination_address;
    socklen_t address_length;
    int flags;
    int socket_fd;
} * send_buffer;

/* contains the details of all messages that have been sent but not yet 
acknowledged by the receiver, along with the last sending time of the message.
used to decide which messages need to be retransmitted if no acknowledgement is received. 
                                            */
typedef struct 
{
    int seq_number;
    char msg[MSG_SIZE];
    size_t msg_len;
    time_t time;
    int flags;
    struct sockaddr_in destination_address;
    socklen_t address_length;
    int num_retransmit;
}unacknowledged_message_entry;
unacknowledged_message_entry * unacknowledged_message_table;

//receive buffer contains the application messages that are received
struct recieve_buffer_item
{
    char buffer[MSG_SIZE];
    struct sockaddr_in source_address;
} * recieve_buffer;

// contains all distinct message ids that are received in the socket so far
// used to detect duplicate messages
struct recieved_messgae_id
{
    int seq_number;
    struct sockaddr_in source_addr;
    socklen_t address_length;
} * recieved_messgae_id_table;

// ----------------------------  internal function declarations ---------------------------- 
int GetSeqNumber();
size_t concatSeqNum(int seq_number, char *buffer, int len);
int getSeqNumFromMesage(char *buffer);

int HandleACKMsgRecv(int seq_number);
int HandleAppMsgRecv(int seq_number, char *buffer, struct sockaddr_in source_addr, socklen_t addr_len) ;

int getEmptyPlaceRecvIdTable();
int getEmptyPlaceSendBuffer();
unacknowledged_message_entry *getEmptyPlaceUnackTable();

int removeUnacknowledgedTableEntry(int seq_number);
int removeSendBufferEntry();

int HandleReceive();
int HandleRetransmit();
int HandleTransmit();

int run();
void signal_handlr(int sig);

ssize_t sendACK(int seq_number, struct sockaddr_in addr, socklen_t addr_len);


// ----------------------------  functions directed to implement ---------------------------- ------------------

// ----------------------------  MRP socket creation and binding and closing ---------------------------- 

int r_socket(int domain, int type, int protocol)
{
    /*        r_socket – This function opens an UDP socket with the socket call. It also
        dynamically allocates space for all the tables, and initializes them. It sets the timer by
        calling the setitimer() function. The parameters to these are the same as the
        normal socket( ) call, except that it will take only SOCK_MRP as the socket type.           */
                                                                                                
    if (type != SOCK_MRP) return -1;

    if ((MRP_fd = socket(domain, SOCK_DGRAM, protocol)) < 0) return MRP_fd;

    // allocate memories dynamically
    send_buffer = (struct send_buffer_item *) malloc(BUF_SIZE * sizeof(struct send_buffer_item));
    recieved_messgae_id_table = (struct recieved_messgae_id *) malloc(TABLE_SIZE * sizeof(struct recieved_messgae_id));
    recieve_buffer = (struct recieve_buffer_item *) malloc(BUF_SIZE * sizeof(struct recieve_buffer_item));
    unacknowledged_message_table = (unacknowledged_message_entry *) malloc(TABLE_SIZE * sizeof(unacknowledged_message_entry));

    // intialize send buffer, recieve buffer, unack table, recved table
    for (int i = 0; i < TABLE_SIZE; i++)
    {
        unacknowledged_message_table[i].seq_number = -1;
        unacknowledged_message_table[i].num_retransmit = 10;
        send_buffer[i].seq_number = -1;
        recieved_messgae_id_table[i].seq_number = -1;
    }

    // call the routine scheduler
    run();
    return MRP_fd;
}

int r_bind(int sockfd, const struct sockaddr *addr, socklen_t addrlen)
{
    /*  r_bind – Binds the UDP socket with the specified address-port using the bind call. */
    return bind(sockfd, addr, addrlen);
}

// ----------------------------  functions for MRP sending ---------------------------- 

ssize_t r_sendto(int sockfd, const void *buf, size_t len, int flags, const struct sockaddr *dest_addr, socklen_t addrlen)
{
    /*   r_sendto – Adds a message id at the beginning of the message and puts it to the
    send buffer if it is not full. If the send buffer is full, this function will (spin) block the
    process until there is some space available to put this message                                       */
    int empty_index;
    if (sockfd != MRP_fd) return -1;
    
    while((empty_index = getEmptyPlaceSendBuffer()) == -1);     // busy wait

    // set values to the entry
    strcpy(send_buffer[empty_index].buffer,(char *)buf);
    send_buffer[empty_index].seq_number = GetSeqNumber();
    send_buffer[empty_index].len = len;
    send_buffer[empty_index].address_length = addrlen;
    send_buffer[empty_index].flags = flags;
    send_buffer[empty_index].destination_address = *(struct sockaddr_in* )dest_addr;
    send_buffer[empty_index].socket_fd = sockfd;

    return len;
}

int HandleTransmit()
{
    /*
    HandleTransmit() :-  checks if there is an empty space in the
    unacknowledged-message table. Returns without doing anything if there is no
    space available. Otherwise, iterates over the send buffer until send buffer is
    empty or unacknowledged-message table is full and do the followings:
    ○ Pick the message with the lowest id from the send buffer;
    send it immediately by calling the sendto() system call with
    MSG_DONTWAIT flag. If the sendto() call executes successfully,
    remove the message from the send buffer and puts it to the
    unacknowledged-message table with the current timestamp, the
    message id and the IP-port.                                     */
    int i;
    size_t msg_len;
    unacknowledged_message_entry *unacknowledged_message_entry;
    while(((unacknowledged_message_entry = getEmptyPlaceUnackTable()) != NULL)&&((i = removeSendBufferEntry()) != -1) )
    {
        strcpy(unacknowledged_message_entry->msg, send_buffer[i].buffer);
        unacknowledged_message_entry->time = time(NULL);
        unacknowledged_message_entry->seq_number = send_buffer[i].seq_number;
        
        msg_len = concatSeqNum(unacknowledged_message_entry->seq_number, unacknowledged_message_entry->msg, send_buffer[i].len);

        assert(msg_len == send_buffer[i].len + sizeof(unacknowledged_message_entry->seq_number));

        unacknowledged_message_entry->address_length = send_buffer[i].address_length;
        unacknowledged_message_entry->destination_address = send_buffer[i].destination_address;
        unacknowledged_message_entry->flags = MSG_DONTWAIT;     // send it immediately by calling the sendto() system call with MSG_DONTWAIT flag
        unacknowledged_message_entry->msg_len = msg_len;

        send_buffer[i].seq_number = -1;                 // message transmitted

        ssize_t r = sendto(send_buffer[i].socket_fd, 
                           unacknowledged_message_entry->msg,
                           unacknowledged_message_entry->msg_len,
                           MSG_DONTWAIT,
                           (struct sockaddr *)&unacknowledged_message_entry->destination_address,
                           unacknowledged_message_entry->address_length);
        if(errno == EINTR)
            if(DEBUG ==1) printf("SIGNAL INTERRUPT\n");
        total_transmitted++;
    }    
    return 0;
}

int HandleRetransmit()
{
    /*
    handles any retransmission of
    application messages as needed due to message loss. The function scans the
    unacknowledged-message table to see if any of the messages’ timeout period
    (T) is over (from the difference between the time in the table entry for a message
    and the current time). If yes, it retransmits that message and resets the time in
    that entry of the table to the new sending time. If not, no action is taken. This is
    repeated for all the messages in the table. At the end of scanning all the
    messages, the function returns
            */
    time_t time_now = time(NULL);
    int i;
    ssize_t ret;
    for (i = 0; i < TABLE_SIZE; i++)
    {
        if (unacknowledged_message_table[i].seq_number != -1 && unacknowledged_message_table[i].time + TIMEOUT <= time_now)
        {
            ret = sendto(   MRP_fd, 
                            unacknowledged_message_table[i].msg,
                            unacknowledged_message_table[i].msg_len,
                            MSG_DONTWAIT,
                            (struct sockaddr *)&unacknowledged_message_table[i].destination_address,
                            unacknowledged_message_table[i].address_length
                        );
            if(errno == EINTR)
                if( DEBUG == 1) printf("SIGNAL INTERRUPT\n");
            unacknowledged_message_table[i].time = time_now;
            if( DEBUG == 1) printf("Retransmit %d\n", unacknowledged_message_table[i].seq_number);
            total_retransmitted++;
            total_transmitted++;
            if (ret < 0)
                return (int)ret;
        }
    }
    return 0;
}

// ----------------------------  functions for MRP recieving ---------------------------- 

ssize_t r_recvfrom(int sockfd, char *buf, size_t len, int flags, struct sockaddr *src_addr, socklen_t *addrlen)
{
    /*
        r_recvfrom – Looks up the received-message table to see if any message is
    already received in the underlying UDP socket or not. If yes, it returns the first
    message and deletes that message from the table. If not, it blocks the call. To block
    the r_recvfrom call, you can use a sleep call to wait for some time and then see
    again if a message is received. r_recvfrom, similar to recvfrom, is a blocking call by
    default and returns to the user only when a message is available.
                                                                */
    if (sockfd != MRP_fd) return -1;

    if(len<0) return -1;
  
    while (size_buf <= 0) sleep(.001);         // sleep till there's no message in recieved buffer
 
    size_buf = size_buf - 1;
    *src_addr = *(struct sockaddr *)&recv_source_addr;
    *addrlen = recv_addr_len;
    recv_flags = flags;
    strcpy(buf, recieve_buffer[begin_recvbuf].buffer);

    recieve_buffer[begin_recvbuf].source_address = recv_source_addr;
    begin_recvbuf = (begin_recvbuf + 1) % BUF_SIZE;

    if (len < strlen(buf)) buf[len] = '\0';
   
    return strlen(buf);
}

int HandleReceive()
{
    /*
        Here the program will try to receive
    the next message from the udp socket by calling recvfrom() function with flag
    MSG_DONTWAIT. MSG_DONTWAIT makes the recvfrom() call non-blocking,
    so the program will not wait if there is no message available. The
    HandleReceive() function calls recvfrom() function till there is some message
    available and performs actions accordingly. 
                                                                                                */
    // ******** HandleReceive() function calls recvfrom() function till there is some message available and performs actions accordingly.
    struct sockaddr_in source_addr;
    socklen_t addr_len = sizeof(source_addr);

    char buffer[BUF_SIZE];
    memset(buffer, '\0', sizeof(buffer));
    
    int nbytes;
    if( (nbytes = recvfrom(MRP_fd, buffer, BUF_SIZE, MSG_DONTWAIT, (struct sockaddr *)&source_addr, &addr_len)) == -1) return -1;

    if (dropMessage(P)) return 0;

    buffer[nbytes] = '\0';
    int seq_number = getSeqNumFromMesage(buffer);                        // remove the sequqnce number 

    if( nbytes > 0 && strstr(buffer, "ACK")== NULL ) 
    {   
        return HandleAppMsgRecv(seq_number, buffer, source_addr, addr_len); // If it is an application message, call a function HandleAppMsgRecv(),
    }
    else if( nbytes > 0 ) 
    {
        return HandleACKMsgRecv(seq_number);       // If it is an ACK message, call a function HandleACKMsgRecv()
    }
}

int HandleAppMsgRecv(int seq_number, char *buf, struct sockaddr_in src_addr, socklen_t addrlen)
{   
    /* check the received-message-id table if the id has already been
    received (duplicate message). If it is a duplicate message, the
    message is dropped, but an ACK is still sent. If it is not a duplicate
    message, the message is added to the receive buffer (without the
    id) including source IP-port, and an ACK is sent. The function
    returns after that.
    */

    int i, duplicate_msg = 0;

    for (i = 0; i < TABLE_SIZE; i++)
    {
        if (recieved_messgae_id_table[i].seq_number == seq_number)
            {
                duplicate_msg = 1;
                break;
            }
    }
    if (duplicate_msg)
    {
        sendACK(seq_number, src_addr, addrlen);
        dup_msgs++;
        return 0;
    }
        size_buf++;

    recv_source_addr = src_addr;
    strcpy(recieve_buffer[end_recvbuf].buffer, buf);
    recv_addr_len = addrlen;
    
    end_recvbuf = (end_recvbuf + 1) % BUF_SIZE;

    if ((i = getEmptyPlaceRecvIdTable()) < 0 ) return -1;

    recieved_messgae_id_table[i].address_length = addrlen;
    recieved_messgae_id_table[i].source_addr = src_addr;
    recieved_messgae_id_table[i].seq_number = seq_number;   
    
    sendACK(seq_number, src_addr, addrlen);
    return 0;
}

int HandleACKMsgRecv(int seq_number)
{
    /*
    If the message is found in the unacknowledged-message table,
    it is removed from the table. If not (duplicate ACK), it is ignored.
    The function returns after that.
                                                                        */
    if( DEBUG == 1) printf("Recieved ACK %d\n", seq_number);
    return removeUnacknowledgedTableEntry(seq_number);
}

// ----------------------------  functions for MRP socket closing ---------------------------- 

int r_close(int sockfd)
{
    if (sockfd != MRP_fd) return -1;
    int flag=1;

    while (flag)
    {
        flag = 0;
        for (int i = 0; i < TABLE_SIZE; i++)
        {
            if ((unacknowledged_message_table[i].seq_number != -1 && unacknowledged_message_table[i].num_retransmit  >=0 ) || (send_buffer[i].seq_number != -1))
            {
                // unacknowledged_message_table[i].num_retransmit--;
                flag = 1;
            }
        }

    }

    if( DEBUG == 1)
    {
        printf("-----------Delivery Report--------------\n");
        printf("# Total Retransmissions = %d\n", total_retransmitted);
        printf("# Total Transmissions = %d\n", total_transmitted);
        printf("# Duplicate Messages  = %d\n", dup_msgs);
        printf("# Duplicate ACKs  = %d\n", dup_ACKS);
    }

    // free allocations
    free(send_buffer);
    free(unacknowledged_message_table);
    free(recieved_messgae_id_table);
    free(recieve_buffer);

    struct itimerval it_val;
    it_val.it_value.tv_sec = 0;
    it_val.it_value.tv_usec = 0;  
    it_val.it_interval = it_val.it_value;
    setitimer(ITIMER_REAL, &it_val, NULL);

    return close(sockfd);
}

// ----------------------------  helpers  ---------------------------- 
int removeUnacknowledgedTableEntry(int seq_number)
{   
    int i = 0;
    while(i<TABLE_SIZE)
    {
        if (unacknowledged_message_table[i].seq_number == seq_number)
        {
            unacknowledged_message_table[i].seq_number = -1;
            return 0;
        }
        i++;
    }

    dup_ACKS++;
    return -1;
}

int removeSendBufferEntry()
{
    int min = INTMAX,min_id = -1;
    for(int i=0; i < TABLE_SIZE; i++)
    {
        if(min >= send_buffer[i].seq_number && send_buffer[i].seq_number != -1)
        {
           min = send_buffer[i].seq_number;
           min_id = i;
        }
    }
    return min_id;
}

int getEmptyPlaceSendBuffer()
{
    for(int i=0; i < TABLE_SIZE; i++)
    {
        if(send_buffer[i].seq_number == -1)
            return i;
    }
    return -1;
} 


unacknowledged_message_entry *getEmptyPlaceUnackTable()
{
    int i=0;
    unacknowledged_message_entry *ret = NULL;

    for (i = 0; i < TABLE_SIZE; i++)
    {
        if (unacknowledged_message_table[i].seq_number == -1)
        { 
            ret = &unacknowledged_message_table[i];
            break;
        }
    }
    return ret;
}

int getEmptyPlaceRecvIdTable()
{
    int ret = -1;
    for (int i = 0; i < TABLE_SIZE; i++)
    {
        if (recieved_messgae_id_table[i].seq_number == -1)
        {
            ret = i;
            break;
        } 
    }
    return ret;
}

size_t concatSeqNum(int seq_number, char *buffer, int len)
{
    if (len == -1) len = strlen(buffer);
    size_t newlen =  len + sizeof(seq_number), i= (size_t)len;
    while(i<newlen)buffer[i++]='\0';
    len++;
    strcat(buffer + len, (char *)&seq_number);
    return newlen;
}

int run()
{
    struct itimerval it_val;
    signal(SIGALRM, signal_handlr);
    it_val.it_value.tv_sec = 0;
    it_val.it_value.tv_usec = 10000;  
    it_val.it_interval = it_val.it_value;
    setitimer(ITIMER_REAL, &it_val, NULL);
    return 0;
}

void signal_handlr(int sig)
{
    HandleReceive();
    HandleRetransmit();
    HandleTransmit();
}

int getSeqNumFromMesage(char *buffer)
{
    return (int )(buffer[(int)strlen(buffer) + 1]);
        
}

int GetSeqNumber()
{
    return ++seq_num;
}

int dropMessage(float p)
{
    return ( ( (float)rand() / ((float)RAND_MAX) ) < p );
}

ssize_t sendACK(int seq_number, struct sockaddr_in addr, socklen_t addr_len)
{
    char ACK[BUF_SIZE];
    memset(ACK, '\0', sizeof(ACK));
    strcpy(ACK, "ACK");
    size_t ret = concatSeqNum(seq_number, ACK, -1);
    size_t r = sendto(MRP_fd, ACK, ret, MSG_DONTWAIT, (struct sockaddr *)&addr, addr_len);
    return r;
}
