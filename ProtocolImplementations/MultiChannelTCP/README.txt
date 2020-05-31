//Jithin Kallukalam Sojan
//2017A7PS0163P

#Q1.

--------------------------------------------------------------------------

#To run the process, do the following in order:

#First Terminal(SERVER):
~$ gcc server.c -o server
~$ ./server

#Second Terminal(CLIENT):
~$ gcc -pthread client.c -o client
~$ ./client

---------------------------------------------------------------------------

#Requirements:


1. A file named input.txt should be present.

--------------------------------------------------------------------------


#Explanation:


#To handle the multiple timers required in the client program for  the timeouts, threads are used. That is why the gcc command requires -pthread option. The thread corresponding to a packet is shutdown as soon as its ack arrives at the client.

#To transmit over two simultaneous connections, two TCP connections were made to the server port. Also, as soon as a channel is free, meaning an ACK was recieved from it, a new packet is created and sent through it. The retransmission of a packet happens through the same channel as it was initially transmitted.

#To handle both the connections in a non-blocking manner, select() function was used with the set containing the  listener socket and the two sockets that get created eventually when the client contacts the listener socket.

#The destination port is defined as 8881(PORT1). It can be changed by changing the PORT1 macro in the packet.h file.

#BUFFER_SIZE is given as 1000(PACKET_SIZE is 100). It can be changed to any multiple of the packet size in the packet.h file(BUFFER_SIZE macro).

#PACKET_SIZE and RETRANS(value of the timeout) can be changed by changing the corresponding macros in the packet.h file.
-----------------------------------------------------------------------------
