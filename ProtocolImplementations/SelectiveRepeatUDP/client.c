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

    int PORTO = atoi(argv[1]);
    //PORT of RELAY 1
    int PORTE = atoi(argv[2]);
    //PORT of RELAY 2

    struct sockaddr_in si_other1;
    struct sockaddr_in si_other2;
    struct sockaddr_in si_entry;
    int slen = sizeof(si_other1);

    struct timeval t1,t2,t3;
    t1.tv_sec = RETRANS;
    t1.tv_usec = 0;
    //timeval structures for implementing the timer.

    fd_set rdset;
    int maxSd,selRes;
    int end = 0;
    int recv_len;

    int s;
    //Socket that sends messages to both relay1 and relay2.

    int i,j,k;
    int tag = 0;
    //Auxiliary variables

    FILE* fp;
    FILE* gp;

    int currBase = 0;
    int sendBase = currBase;
    int currWindow = (currBase + WINSIZE)%SEQNO;
    packet p[SEQNO];
    bool acked[SEQNO];
    packet ack;

    for(int i = 0;i<SEQNO;i++){
        acked[i] = false;
        p[i].seq_no = 0;
        p[i].size = 0;
        p[i].isAck = false;
        p[i].isLast = false;
    }


    if ((s=socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1)
    {
        printf("Error: Creating socket 1.\n");
    }

    memset((char*)&si_other1,0,sizeof(si_other1));
    si_other1.sin_family = AF_INET;
    si_other1.sin_port = htons(PORTO);
    si_other1.sin_addr.s_addr = inet_addr("127.0.0.1");
    //Use this to send the odd packets.
    //address of Relay1

    memset((char*)&si_other2,0,sizeof(si_other2));
    si_other2.sin_family = AF_INET;
    si_other2.sin_port = htons(PORTE);
    si_other2.sin_addr.s_addr = inet_addr("127.0.0.1");
    //Use this to send the even packets.
    //address of Relay2
    

    fp = fopen("input.txt","r");
    gp = fopen("input.txt","r");
    if(fp==NULL || gp==NULL){
        printf("Error: Cannot open input file.\n");
    }

    fseek(gp,0,SEEK_END);


    printf("%-7s %-5s %-17s %-5s %-3s %-7s %-7s\n","Node","Event","Timestamp","PType","Seq","Source","Dest");

    maxSd = s;

    while(true){
        i = sendBase;
        k = currWindow;
        while(i!=k){
            //Sending packets in the window that havent been sent yet.
            tag = 1;
            p[i].size = fread(p[i].msg,1,PACKET_SIZE,fp);
            p[i].seq_no = i;
            acked[i] = false;
            if(fp==gp || p[i].size<PACKET_SIZE){
                p[i].isLast = 1;
                end = 1;
                currWindow = (i+1)%SEQNO;
            }

            if(i%2==1){
                //Sending odd packets to RELAY1
                if (sendto(s,(char*)&p[i], sizeof(p[i]), 0, (struct sockaddr*) &si_other1, slen) == -1)
                {
                    printf("Error: Sending data to relay odd.\n");
                }
                printf("%-7s %-5s %-17s %-5s %-3d %-7s %-7s\n","CLIENT","S",timeStamp(),"DATA",p[i].seq_no,"CLIENT","RELAY1");
            }
            else{
                //Sending even packets to RELAY1
                if (sendto(s,(char*)&p[i], sizeof(p[i]), 0, (struct sockaddr*) &si_other2, slen) == -1)
                {
                    printf("Error: Sending data to relay even.\n");
                }
                printf("%-7s %-5s %-17s %-5s %-3d %-7s %-7s\n","CLIENT","S",timeStamp(),"DATA",p[i].seq_no,"CLIENT","RELAY2");
            }
            i = (i+1)%SEQNO;
            if(end==1)break;
        }

        if(tag==1){
            sendBase = i;
            tag = 0;
            gettimeofday(&t1,NULL);
            t1.tv_sec+=RETRANS;
        }

        FD_ZERO(&rdset);

        FD_SET(s,&rdset);

        gettimeofday(&t2,NULL);
        timersub(&t1,&t2,&t3);
        //Setting the timer for timeout

        selRes = select(maxSd+1,&rdset,NULL,NULL,&t3);

        if((selRes<0) && (errno!=EINTR)){
            printf("Select error.\n");
            return 1;
        }

        if(selRes==0){
            i = currBase;
            k = currWindow;
            while(i!=k){
                //Timeout, all un-acked packets in the window are re-transmitted.
                if(acked[i]==false){
                    printf("%-7s %-5s %-17s %-5s %-3d %-7s %-7s\n","CLIENT","TO",timeStamp(),"DATA",p[i].seq_no,"---","---");
                    if(i%2==1){
                        if (sendto(s,(char*)&p[i], sizeof(p[i]), 0, (struct sockaddr*) &si_other1, slen) == -1)
                        {
                            printf("Error: Sending data to relay odd.\n");
                        }
                        printf("%-7s %-5s %-17s %-5s %-3d %-7s %-7s\n","CLIENT","RE",timeStamp(),"DATA",p[i].seq_no,"CLIENT","RELAY1");
                    }
                    else{
                        if (sendto(s,(char*)&p[i], sizeof(p[i]), 0, (struct sockaddr*) &si_other2, slen) == -1)
                        {
                            printf("Error: Sending data to relay even.\n");
                        }
                        printf("%-7s %-5s %-17s %-5s %-3d %-7s %-7s\n","CLIENT","RE",timeStamp(),"DATA",p[i].seq_no,"CLIENT","RELAY2");
                    }
                }
                i = (i+1)%SEQNO;
            }
            gettimeofday(&t1,NULL);
            t1.tv_sec+=RETRANS;
            //Resetting the timer for timeout
        }

        if(FD_ISSET(s,&rdset)){
            //ACK recieved from either RELAY1 or RELAY2
            if ((recv_len = recvfrom(s, (char*)&ack, sizeof(ack), 0, (struct sockaddr *) &si_entry,&slen)) == -1)
            {
                printf("Recieve error.\n");
                return 1;
            }

            if(ack.seq_no%2==1){
                printf("%-7s %-5s %-17s %-5s %-3d %-7s %-7s\n","CLIENT","R",timeStamp(),"ACK",ack.seq_no,"RELAY1","CLIENT");
            }
            else{
                printf("%-7s %-5s %-17s %-5s %-3d %-7s %-7s\n","CLIENT","R",timeStamp(),"ACK",ack.seq_no,"RELAY2","CLIENT");
            }

            if(acked[ack.seq_no])continue;
            acked[ack.seq_no] = true;
            i = currBase;
            k = currWindow;
            while(i!=k){
                //Sliding of window.
                tag = 1;
                if(!acked[i]){
                    break;
                }
                i = (i+1)%SEQNO;
            }
            currBase = i;
            if(end==1 && currBase==sendBase)break;  
            //Beacuse the process is over.
            if(tag==1){
                tag = 0;
                if(!end)currWindow = (currBase+WINSIZE)%SEQNO;
                //otherwise all the packets have already been sent.
            }
        }

        
    }
    close(s);
    fclose(gp);
    fclose(fp);

}
