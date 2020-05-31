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
#include "packet.h"

int main(){
    int masterSocket1;
    //Listener socket
    int opt1 = 1;

    int maxSd, selRes;
    int sd;
    fd_set rdset;

    struct sockaddr_in address1;
    int new_socket = 0;
    int client_socket1 = 0;
    //Channel 1
    int client_socket2 = 0;
    //Channel 2

    packet p;
    packet ack;

    int count = 0;

    FILE* fp = NULL;
    char* buffer = (char*)malloc(BUFFER_SIZE*sizeof(buffer));
    int currSize = 0;
    //Current size of elements in the buffer.

    int end = 0;
    int tag = 0;
    int drop = 0;
    int dropped = 0;

    if((masterSocket1 = socket(AF_INET,SOCK_STREAM,0))<0){
        printf("Error: Could not create master socket.\n");
        return 1;
    }

    if(setsockopt(masterSocket1,SOL_SOCKET,SO_REUSEADDR,(char*)&opt1,sizeof(opt1))<0){
        printf("Error: Could not set masterSocket 1 Opt.\n");
        return 1;
    }

    memset(&address1,0,sizeof(address1));
    address1.sin_family = AF_INET;
    address1.sin_addr.s_addr = (INADDR_ANY);
    address1.sin_port = htons(PORT1);
    //Address of port to be bound to.

    if(bind(masterSocket1,(struct sockaddr*)&address1,sizeof(address1))<0){
        printf("Error: Binding of Master Socket 1 failed.\n");
        return 1;
    }

    if(listen(masterSocket1,5)<0){
        printf("Error: Maximum of 5 pending connections on listener1.\n");
        return 1;
    }

    fp = fopen("output.txt","w");
    if(fp==NULL){
        printf("Error: File cannot be opened.\n");
    }

    srand(5);
    //setting the seed for the random function.

    maxSd = masterSocket1;

    while(!end){
        FD_ZERO(&rdset);

        FD_SET(masterSocket1,&rdset);
        maxSd = masterSocket1;

        if(client_socket1!=0){
            FD_SET(client_socket1,&rdset);
            if(maxSd<client_socket1)maxSd = client_socket1;
        }

        if(client_socket2!=0){
            FD_SET(client_socket2,&rdset);
            if(maxSd<client_socket2)maxSd = client_socket2;
        }

        selRes = select(maxSd+1,&rdset,NULL,NULL,NULL);

        if((selRes<0) && (errno!=EINTR)){
            printf("Select error.\n");
            return 1;
        }

        if(FD_ISSET(masterSocket1,&rdset)){
            //accepting the tcp connections from the client
            if((new_socket = accept(masterSocket1,(struct sockaddr*)NULL,NULL))<0){
                printf("Error: Accept error.\n");
                return 1;
            }

            if(client_socket1==0){
                client_socket1=new_socket;       
            }
            else if(client_socket2==0){
                client_socket2 = new_socket;
            }
        }            
        
        if(FD_ISSET(client_socket1,&rdset)){
            sd = client_socket1;
            read(sd,(char*)&p,sizeof(p));
            drop = rand()%100;
            drop/=PDR;
            if(drop!=0){
                //Not dropped
                printf("RCVD PKT: Seq. No %4d of size %4d Bytes from channel %d\n",p.seq_no,p.size,p.Channel);
                //RCVD PKT only printed if the packet is not dropped.
                drop = 0;
                int offset = ftell(fp);

                if(p.seq_no==offset){
                    fwrite(p.msg,1,p.size,fp);
                    if(currSize != 0){
                        //Writing the remaining messages written in the buffer.
                        fwrite(buffer,1,currSize,fp);
                        currSize = 0;
                    }
                    
                    ack.isAck = 1;
                    ack.seq_no = p.seq_no;
                    ack.Channel = p.Channel;
                    write(sd,(char*)&ack,sizeof(ack));
                }
                else if(currSize<BUFFER_SIZE){
                    //Out of order, hence writing into buffer.
                    sprintf(buffer+currSize,"%s",p.msg);
                    currSize+=p.size;

                    ack.isAck = 1;
                    ack.seq_no = p.seq_no;
                    ack.Channel = p.Channel;
                    write(sd,(char*)&ack,sizeof(ack));
                }
                else if(currSize==BUFFER_SIZE){
                    //Buffer is full, hence packets dropped.
                    //These packets turn up in the RCVD PKT trace msg.
                    dropped = 1;
                }

                if(!dropped)
                printf("SENT ACK: for PKT with Seq. No %4d from channel %d\n",ack.seq_no,ack.Channel);

                if(!dropped && p.isLast==1 && currSize==0){
                    //Last packet encountered in order.
                    end = 1;
                }
                else if(!dropped && p.isLast==1 && currSize!=0){
                    tag = 1;
                }
                else if(tag==1 && currSize==0){
                    //Last packet already encountered and now all packets in order.
                    end = 1;
                }

                dropped = 0;
            }
            
        }

        if(FD_ISSET(client_socket2,&rdset)){
            //Same as for client_socket1; except this is for Channel 2.
            sd = client_socket2;
            read(sd,(char*)&p,sizeof(p));
            drop = rand()%100;
            drop/=PDR;
            if(drop!=0){
                printf("RCVD PKT: Seq. No %4d of size %4d Bytes from channel %d\n",p.seq_no,p.size,p.Channel);
                drop = 0;
                int offset = ftell(fp);
                if(p.seq_no==offset){
                    fwrite(p.msg,1,p.size,fp);
                    if(currSize != 0){
                        fwrite(buffer,1,currSize,fp);
                        currSize = 0;
                    }
                    
                    ack.isAck = 1;
                    ack.seq_no = p.seq_no;
                    ack.Channel = p.Channel;
                    write(sd,(char*)&ack,sizeof(ack));
                }
                else if(currSize<BUFFER_SIZE){
                    sprintf(buffer+currSize,"%s",p.msg);
                    currSize+=p.size;

                    ack.isAck = 1;
                    ack.seq_no = p.seq_no;
                    ack.Channel = p.Channel;
                    write(sd,(char*)&ack,sizeof(ack));
                }
                else if(currSize==BUFFER_SIZE){
                    //drop
                    dropped = 1;
                }

                if(!dropped && p.isLast==1 && currSize==0){
                    end = 1;
                }
                else if(!dropped && p.isLast==1 && currSize!=0){
                    tag = 1;
                }
                else if(tag==1 && currSize==0){
                    end = 1;
                }

                if(!dropped)
                printf("SENT ACK: for PKT with Seq. No %4d from channel %d\n",ack.seq_no,ack.Channel);

                dropped = 0;
            }
            
        }
    }
    
    close(masterSocket1);
    fclose(fp);
    return 1;
}
