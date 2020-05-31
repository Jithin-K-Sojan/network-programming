//Jithin Kallukalam Sojan
//2017A7PS0163P
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <errno.h>
#include <sys/time.h>
#include <stdbool.h>
#include <signal.h>
#include <time.h>
#include "packet.h"


char* timeStamp(){
    //Function to produce string of timestamp
    time_t ltime;
    struct tm result;
    struct timeval t1;
    char* timeSt = (char*)malloc(sizeof(char)*16);
    char milliSec[7];

    ltime = time(NULL);
    localtime_r(&ltime,&result);
    gettimeofday(&t1,NULL);

    int a = strftime(timeSt,16,"%H:%M:%S.",&result);

    sprintf(milliSec,"%.06ld",t1.tv_usec);
    timeSt = strcat(timeSt,milliSec);
    return timeSt;
}

int main(int argc, char* argv[]){

    int drop = 0;
    int delay = 0;

    struct sockaddr_in si_other;
    struct sockaddr_in si_otherd;
    struct sockaddr_in si_me;
    int slen;

    int sd;
    int s;

    int PORT = atoi(argv[1]);
    //PORT of the executing RELAY(Either RELAY1 or RELAY2)
    int opt1 = 1;
    
    slen = sizeof(si_other);

    fd_set rdset;
    int maxSd,selRes;
    int recv_len;

    packet p;
    packet ack;

    int tag = 0;
    char relay[7];
    //Auxiliary variables

    if ((s=socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1)
    {
        printf("Error: Creating socket 1.\n");
    }

    if(setsockopt(s,SOL_SOCKET,SO_REUSEADDR,(char*)&opt1,sizeof(opt1))<0){
        printf("Error: Could not set masterSocket 1 Opt.\n");
        return 1;
    }

    memset((char *) &si_me, 0, sizeof(si_me));
    si_me.sin_family = AF_INET;
    si_me.sin_port = htons(PORT);
    si_me.sin_addr.s_addr = htonl(INADDR_ANY);
    //Binding to port of the RELAY

    if( bind(s , (struct sockaddr*)&si_me, sizeof(si_me) ) == -1)
    {
        printf("Error: Binding of socket 1.\n");
    }

    if ((sd=socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1)
    {
        printf("Error: Creating socket 1.\n");
    }

    memset((char*)&si_otherd,0,sizeof(si_otherd));
    si_otherd.sin_family = AF_INET;
    si_otherd.sin_port = htons(PORTD);
    si_otherd.sin_addr.s_addr = inet_addr("127.0.0.1");
    //Address of the SERVER

    p.size = -1;
    //Signal to the SERVER; to mention this RELAY's address
    if (sendto(sd,(char*)&p, sizeof(p), 0, (struct sockaddr*) &si_otherd, slen) == -1)
    {
        printf("Error: Sending signal to server.\n");
    }

    printf("%-7s %-5s %-17s %-5s %-3s %-7s %-7s\n","Node","Event","Timestamp","PType","Seq","Source","Dest");


    if(s>maxSd)maxSd = s;
    if(sd>maxSd)maxSd = sd;

    srand(5);
    //Setting the seed for the random function.

    while(true){
        FD_ZERO(&rdset);

        FD_SET(s,&rdset);

        FD_SET(sd,&rdset);

        selRes = select(maxSd+1,&rdset,NULL,NULL,NULL);

        if((selRes<0) && (errno!=EINTR)){
            printf("Select error.\n");
            return 1;
        }

        if(FD_ISSET(s,&rdset)){
            //Accepting packets from CLIENT.
            if ((recv_len = recvfrom(s, (char*)&p, sizeof(p), 0, (struct sockaddr *) &si_other,&slen)) == -1)
            {
                printf("Recieve error.\n");
                return 1;
            }

            if(tag==0){
                //Figuring out whether current relay is the odd relay or the even relay.
                if(p.seq_no%2==1){
                    strcpy(relay,"RELAY1");
                }
                else{
                    strcpy(relay,"RELAY2");
                }
                tag = 1;
            }

            drop = (rand()%100)+1;
            drop/=PDR;
            if(drop==0){
                //Packet dropped.
                printf("%-7s %-5s %-17s %-5s %-3d %-7s %-7s\n",relay,"D",timeStamp(),"DATA",p.seq_no,"---","---");
            }
            else{
                printf("%-7s %-5s %-17s %-5s %-3d %-7s %-7s\n",relay,"R",timeStamp(),"DATA",p.seq_no,"CLIENT",relay);

                drop = 0;
                delay = rand()%2000;

                usleep(delay);
                //Transmitted to server after delay.
                if (sendto(sd,(char*)&p, sizeof(p), 0, (struct sockaddr*) &si_otherd, slen) == -1)
                {
                    printf("Error: Sending data to dest.\n");
                }

                printf("%-7s %-5s %-17s %-5s %-3d %-7s %-7s\n",relay,"S",timeStamp(),"DATA",p.seq_no,relay,"SERVER");
            }
        }

        if(FD_ISSET(sd,&rdset)){
            //Accepting ACKs that the server sent along the same route.
            if ((recv_len = recvfrom(sd, (char*)&ack, sizeof(ack), 0, (struct sockaddr *) &si_otherd,&slen)) == -1)
            {
                printf("Recieve error.\n");
                return 1;
            }

            if(ack.size==-1){
                //Signal from the server that the process is over, the relay can shut down.
                break;
            }

            printf("%-7s %-5s %-17s %-5s %-3d %-7s %-7s\n",relay,"R",timeStamp(),"ACK",ack.seq_no,"SERVER",relay);

            //Sending the ACK to the client.
            if (sendto(s,(char*)&ack, sizeof(ack), 0, (struct sockaddr*) &si_other, slen) == -1)
            {
                printf("Error: Sending data to client.\n");
            }

            printf("%-7s %-5s %-17s %-5s %-3d %-7s %-7s\n",relay,"S",timeStamp(),"ACK",ack.seq_no,relay,"CLIENT");

        }
    }

    close(sd);
    close(s);
}
