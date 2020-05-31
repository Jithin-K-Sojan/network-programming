//Jithin Kallukalam Sojan
//2017A7PS0163P
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

#define PACKET_SIZE 100
#define WINSIZE 5
#define SEQNO 10 
//(10 means seq_no 0 to 9)
//Theoretical Minimum of SEQNO = 2 * WINSIZE

#define PDR 20
//Packet drop rate
#define RETRANS 2
//Value of timeout

#define PORTD 8889
//destination port

struct packet{
    int seq_no;
    bool isLast;
    bool isAck;
    int size;
    char msg[PACKET_SIZE];
}__attribute__((packed));
//Byte alligning the structure to transfer over socket

typedef struct packet packet;

