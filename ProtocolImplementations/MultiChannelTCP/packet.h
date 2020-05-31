//Jithin Kallukalam Sojan
//2017A7PS0163P
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define PACKET_SIZE 100
#define PORT1 8881
//PORT of the server
#define BUFFER_SIZE 1000

#define PDR 10
//Packet drop rate.

#define RETRANS 2
//The waiting time till timeout

typedef struct packet{
    int seq_no;
    int size;
    bool isLast;
    bool isAck;
    bool Channel;
    char msg[PACKET_SIZE];
}packet;
