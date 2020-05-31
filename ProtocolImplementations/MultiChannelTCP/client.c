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
#include <pthread.h>
#include "packet.h"


packet p1;
packet p2;
packet ack1;
packet ack2;
int s1 = -1;
//Socket for connection with Channel 1
int s2 = -1;
//Socket for connection with Channel 2
pthread_t thread1;
pthread_t thread2;

void* func1(void * arg){
    // Thread for timed retransmissions(timeout) in Channel 1
    while(1){
        sleep(RETRANS);
        write(s1,(char*)&p1,sizeof(p1));
        printf("SENT PKT: Seq. No %4d of size %4d Bytes from channel %d\n",p1.seq_no,p1.size,p1.Channel);
    }
    return NULL;
}

void* func2(void* arg){
    // Thread for timed retransmissions(timeout) in Channel 2
    while(1){
        sleep(RETRANS);
        write(s2,(char*)&p2,sizeof(p2));
        printf("SENT PKT: Seq. No %4d of size %4d Bytes from channel %d\n",p2.seq_no,p2.size,p2.Channel);
    }
    return NULL;
}


int main(){
    struct sockaddr_in si_server1;
    int slen = sizeof(si_server1);
    int opt = 1;

    int maxSd, selRes;
    fd_set rdset;

    if((s1 = socket(AF_INET,SOCK_STREAM,0))<0){
        printf("Error: Could not create socket for channel 1.\n");
        return 1;
    }

    if((s2 = socket(AF_INET,SOCK_STREAM,0))<0){
        printf("Error: Could not create socket for channel 2.\n");
        return 1;
    }

    memset((char *) &si_server1, 0, sizeof(si_server1));
    si_server1.sin_family = AF_INET;
    si_server1.sin_port = htons(PORT1);
    si_server1.sin_addr.s_addr = inet_addr("127.0.0.1");
    //PORT to connect to for TCP connections to server.


    if(connect(s1,(struct sockaddr *)&si_server1, sizeof(si_server1))<0)
    {
        printf("Error : Connection for first channel failed.\n");
        return 1;
    }


    if(connect(s2,(struct sockaddr *)&si_server1, sizeof(si_server1))<0)
    {
        printf("Error : Connection for second channel failed.\n");
        return 1;
    }



    FD_ZERO(&rdset);

    FD_SET(s1,&rdset);
    if(s1>maxSd)maxSd = s1;

    FD_SET(s2,&rdset);
    if(s2>maxSd)maxSd = s2;



    FILE* fp = NULL;
    fp = fopen("input.txt","r");
    FILE* gp = NULL;
    gp = fopen("input.txt","r");
    if(fp==NULL|| gp==NULL){
        printf("Error: File cannot be opened.\n");
        return -1;
    }
    fseek(gp,0,SEEK_END);


    bool end = 0;
    bool tag1 = 1;
    bool tag2 = 1;


    p1.seq_no = ftell(fp);
    p1.size = fread(p1.msg,1,PACKET_SIZE,fp);
    p1.Channel = 0;
    if(p1.size<PACKET_SIZE || fp==gp){
        p1.isLast = 1;
        end = 1;
    }
    write(s1,(char*)&p1,sizeof(p1));
    pthread_create(&thread1,NULL,func1,NULL);
    //Starting the timer for timeout
    printf("SENT PKT: Seq. No %4d of size %4d Bytes from channel %d\n",p1.seq_no,p1.size,p1.Channel);


    if(!end){
        //If the final packet has not been encountered yet.
        p2.seq_no = ftell(fp);
        p2.size = fread(p2.msg,1,PACKET_SIZE,fp);
        p2.Channel = 1;
        if(p2.size<PACKET_SIZE || fp == gp){
            p2.isLast = 1;
            end = 1;
        }
        if(p2.size!=0){
            write(s2,(char*)&p2,sizeof(p2));
            pthread_create(&thread2,NULL,func2,NULL);
            printf("SENT PKT: Seq. No %4d of size %4d Bytes from channel %d\n",p2.seq_no,p2.size,p2.Channel);
                        
        }
        else{
            tag2 = 0;
        }
    }
    else{
        tag2 = 0;
    }

    while(tag1||tag2){
        FD_ZERO(&rdset);

        FD_SET(s1,&rdset);

        FD_SET(s2,&rdset);

        selRes = select(maxSd+1,&rdset,NULL,NULL,NULL);

        if((selRes<0) && (errno!=EINTR)){
            printf("Select error.\n");
            return 1;
        }

        if(FD_ISSET(s1,&rdset)){
            //ACK recieved for the packet transmiited in Channel 1
            read(s1,(char*)&ack1,sizeof(ack1));
            printf("RCVD ACK: for PKT with Seq. No. %4d from channel %d\n",ack1.seq_no,ack1.Channel);

            if(ack1.seq_no<p1.seq_no){
                //Ignoring the ACK.
                ;
            }
            else{
                pthread_cancel(thread1);
                //Stopping the timer for timeout of the packet transmitted in Channel 1.
                
                if(end==0){
                    p1.seq_no = ftell(fp);
                    p1.size = fread(p1.msg,1,PACKET_SIZE,fp);
                    p1.Channel = 0;
                    if(p1.size<PACKET_SIZE || fp == gp){
                        p1.isLast = 1;
                        end = 1;
                    }
                    if(p1.size!=0){
                        //Next packet to be transmitted in Channel 1
                        write(s1,(char*)&p1,sizeof(p1));
                        pthread_create(&thread1,NULL,func1,NULL);
                        printf("SENT PKT: Seq. No %4d of size %4d Bytes from channel %d\n",p1.seq_no,p1.size,p1.Channel);
                        
                    }
                    else{
                        tag1 = 0;
                    }
                    
                }
                else{
                    tag1 = 0;
                }    
            }
        }

        if(FD_ISSET(s2,&rdset)){
            //ACK recieved for the packet transmiited in Channel 2
            read(s2,(char*)&ack2,sizeof(ack2));
            printf("RCVD ACK: for PKT with Seq. No. %4d from channel %d\n",ack2.seq_no,ack2.Channel);

            if(ack2.seq_no<p2.seq_no){
                //Ignoring the ACK.
                ;
            }
            else{
                pthread_cancel(thread2);
                //Stopping the timer for timeout of the packet transmiited in Channel 2.

                if(end==0){
                    p2.seq_no = ftell(fp);
                    p2.size = fread(p2.msg,1,PACKET_SIZE,fp);
                    p2.Channel = 1;
                    if(p2.size<PACKET_SIZE || fp==gp){
                        p2.isLast = 1;
                        end = 1;
                    }
                    if(p2.size!=0){
                        //Next packet to be transmitted in Channel 2
                        write(s2,(char*)&p2,sizeof(p2));
                        pthread_create(&thread2,NULL,func2,NULL);
                        printf("SENT PKT: Seq. No %4d of size %4d Bytes from channel %d\n",p2.seq_no,p2.size,p2.Channel);
                        
                    }
                    else{
                        tag2 = 0;
                    }
                    
                }
                else{
                    tag2 = 0;
                }
            }
        }

    }

    fclose(fp);
    return 1;
}

