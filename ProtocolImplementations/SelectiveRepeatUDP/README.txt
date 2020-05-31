//Jithin Kallukalam Sojan
//2017A7PS0163P

#Q2.

--------------------------------------------------------------------------

#To run the process, do the following in order:

#First Terminal(SERVER)
~$ gcc server.c -o server
~$ ./server

#Second Terminal(RELAY 1)
~$ gcc relay.c -o relay
~$ ./relay 8887

#Third Terminal(RELAY 2)
~$ ./relay 8888

#Fourth Terminal(CLIENT)
~$ gcc -pthread client.c -o client
~$ ./client 8887 8888

---------------------------------------------------------------------------

#Requirements:


1. A file named input.txt should be present.

--------------------------------------------------------------------------


#Explanation:


#To handle the timeouts in the client module, as it would be complicated to set individual timers for each packet in the window, timers are set everytime the window is shifted, or when un-acked packets are retransmiited. This results in only one timer and is set in the timeout field of the select() function.

#To handle the timeout in the client, select function is used along with the socket that communicates with relay1 and relay2. For the server, since all the packets come to the same port, blocking read used in an infinite loop works. For the relay module, the two sockets corresponding to the one recieving packets from the client and the one sending packets to the server are placed in the set that is observed by the select function.

#ACKs are sent along the path that the packet arrived (Odd sequence numbers through relay1 and even sequence numbers through relay2).

#The port number 8889 is reserved for the server and can be changed in the PORTD macro in the packet.h file.

#The port numbers for relay1 and relay2 have to mentioned as Command Line Arguements while executing the two instances of the relay module(In the above case: relay1 has Port 8887 and relay2 has Port 8888).

#The ports corresponding to relay1 and relay2 have to be given as Command Line Arguements to the executable of the client module. 

#Ensure that the number of sequence numbers is atleast twice the window size(Theoretical Limit). Both are defined as macros in packet.h (WINSIZE = 5, SEQNO = 10).

#Buffer size is taken as PACKET_SIZE * SEQNO.

--------------------------------------------------------------------------
