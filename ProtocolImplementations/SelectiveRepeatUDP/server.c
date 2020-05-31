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


int main(){
    struct sockaddr_in si_other1;
    struct sockaddr_in si_othero;
    struct sockaddr_in si_othere;
    struct sockaddr_in si_me;

    bool tago = false;
    bool tage = false;
    //Auxiliary variables

    int slen = sizeof(si_other1);

    fd_set rdset;
    int maxSd,selRes;
    int end = 0;
    int recv_len;

    int opt1 = 1;
    int s;

    packet p,ack;
    ack.isAck = 1;

    bool acked[SEQNO];

    int i,k;
    char c;
    //Auxiliary variables

    char node[7] = "SERVER";
    char relay[7];

    for(int i = 0;i<SEQNO;i++){
        acked[i] = false;
        //Setting all seq numbers as un-acked.
    }

    char* buffer = (char*)malloc(sizeof(char)*PACKET_SIZE*SEQNO);
    int size = 0;
    //Current size of elements in the buffer.

    int currBase = 0;
    int currWindow = (currBase+WINSIZE)%SEQNO;

    int last = -3;
    int lastSize = 0;
    bool hasLast = false;

    FILE* fp;

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
    si_me.sin_port = htons(PORTD);
    si_me.sin_addr.s_addr = htonl(INADDR_ANY);
    //Address of server to bind to

    memset((char *) &si_othero, 0, sizeof(si_othero));
    si_othero.sin_family = AF_INET;
    //Address of RELAY1

    memset((char *) &si_othere, 0, sizeof(si_othere));
    si_othere.sin_family = AF_INET;
    //Address of RELAY2
    

    if( bind(s , (struct sockaddr*)&si_me, sizeof(si_me) ) == -1)
    {
        printf("Error: Binding of socket 1.\n");
    }

    fp = fopen("output.txt","w");
    if(fp==NULL){
        printf("Error: Could not open file.\n");
    }

    printf("%-7s %-5s %-17s %-5s %-3s %-7s %-7s\n","Node","Event","Timestamp","PType","Seq","Source","Dest");

    while(!end){
        //Accepting packets from the Relays
        if ((recv_len = recvfrom(s, (char*)&p, sizeof(p), 0, (struct sockaddr *) &si_other1,&slen)) == -1)
        {
            printf("Recieve error.\n");
            return 1;
        }

        if(p.size==-1 && !tago){
            //Signal from RELAY1 containing its address.
            tago = true;
            si_othero.sin_port = si_other1.sin_port;
            si_othero.sin_addr.s_addr = si_other1.sin_addr.s_addr;
            continue;
        }
        else if(p.size==-1 && !tage){
            //Signal from RELAY2 containing its address.
            tage = true;
            si_othere.sin_port = si_other1.sin_port;
            si_othere.sin_addr.s_addr = si_other1.sin_addr.s_addr;
            continue;
        }

        if(p.seq_no%2==1){
            strcpy(relay,"RELAY1");
        }
        else{
            strcpy(relay,"RELAY2");
        }
        printf("%-7s %-5s %-17s %-5s %-3d %-7s %-7s\n",node,"R",timeStamp(),"DATA",p.seq_no,relay,node);

        ack.seq_no = p.seq_no;
        //Sending ACKs back to the same relays.
        if (sendto(s,(char*)&ack, sizeof(ack), 0, (struct sockaddr*) &si_other1, slen) == -1)
        {
            printf("Error: Sending data to client.\n");
        }
        printf("%-7s %-5s %-17s %-5s %-3d %-7s %-7s\n",node,"S",timeStamp(),"ACK",ack.seq_no,node,relay);
        

        //Figuring out whether current packet should be ignored. Not in window.
        if(currBase<currWindow){
            if(!(currBase<=p.seq_no && p.seq_no<currWindow)){
                continue;
            }
        }
        else{
            if(!(currBase<=p.seq_no || p.seq_no<currWindow)){
                continue;
            }
        }
        if(acked[p.seq_no])continue;


        acked[p.seq_no] = true;
        if(p.isLast){
            hasLast = true;
            last = p.seq_no;
            lastSize = p.size;
        }

        if(p.seq_no!=SEQNO){
            //Ensuring that next character does not get overwritten.
            c = *(buffer + ((p.seq_no+1)*PACKET_SIZE));
        }
        sprintf(buffer+(p.seq_no*PACKET_SIZE),"%s",p.msg);
        if(p.seq_no!=SEQNO){
            //Ensuring that next character does not get overwritten.
            *(buffer + ((p.seq_no+1)*PACKET_SIZE)) = c;
        }
        

        i = currBase;
        k = currWindow;
        while(i!=k){
            //Chacking how much to slide the window.
            if(!acked[i]){
                break;
            }
            else{
                acked[i] = false;
                //Setting ack back.
            }
            i = (i+1)%SEQNO;
        }

        if(i!=currBase){
            //If window has slid, write from buffer into the file.
            //It means that there are packets in order.
            if(i<currBase){
                //This means that the Window has crossed the max Sequence number, its cycling through.
                if(hasLast && ((last+1)%SEQNO==0)){
                    size = (SEQNO-1-currBase)*PACKET_SIZE  + lastSize;
                }
                else{
                    size = (SEQNO-currBase)*PACKET_SIZE;
                }
                fwrite(buffer+(currBase*PACKET_SIZE),1,size,fp);

                if(hasLast && ((last+1)%SEQNO==i)){
                    size = (i-1)*PACKET_SIZE + lastSize;
                }
                else{
                    size = i*PACKET_SIZE;
                }
                fwrite(buffer,1,size,fp);

            }
            else{
                if(hasLast && ((last+1)%SEQNO==i)){
                    size = (i-1-currBase)*PACKET_SIZE + lastSize;
                }
                else{
                    size = (i-currBase)*PACKET_SIZE;
                }
                fwrite(buffer+(currBase*PACKET_SIZE),1,size,fp);
            }
        }

        if((hasLast)&&((last+1)%SEQNO==i)){
            end = 1;
            break;
        }

        currBase = i;
        currWindow = (i+WINSIZE)%SEQNO;
    }

    ack.isAck = 1;
    ack.size = -1;
    //Sending signal to Relay 1 to shut down.
    if (sendto(s,(char*)&ack, sizeof(ack), 0, (struct sockaddr*) &si_othero, slen) == -1)
    {
        printf("Error: Sending signal to relay1.\n");
    }

    ack.isAck = 1;
    ack.size = -1;
    //Sending signal to Relay 2 to shut down.
    if (sendto(s,(char*)&ack, sizeof(ack), 0, (struct sockaddr*) &si_othere, slen) == -1)
    {
        printf("Error: Sending signal to relay2.\n");
    }

    close(s);
    fclose(fp);
    return 1;
}
