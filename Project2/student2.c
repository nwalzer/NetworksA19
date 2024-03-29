#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "project2.h"
 
/* ***************************************************************************
 ALTERNATING BIT AND GO-BACK-N NETWORK EMULATOR: VERSION 1.1  J.F.Kurose

   This code should be used for unidirectional or bidirectional
   data transfer protocols from A to B and B to A.
   Network properties:
   - one way network delay averages five time units (longer if there
     are other messages in the channel for GBN), but can be larger
   - packets can be corrupted (either the header or the data portion)
     or lost, according to user-defined probabilities
   - packets may be delivered out of order.

   Compile as gcc -g project2.c student2.c -o p2
**********************************************************************/

//size of buffer on A side to hold packets
#define QSIZE 128

//time interval
#define RTT 2000

//size of a standard message
#define MSIZE 20

//max size of the sending window
#define WINDOW 5

//set to 1 to allow print statements, set to 0 to not print anything
int PRINT = 0;

struct Sndr {
	struct pkt pktBuff[QSIZE]; //buffer for packets given from Layer 5
	int end; //total number of packets sent. This % QSIZE gets end of unsent packets
	int start; //This % QSIZE gets start of unsent packets
	int nextSeq; //next sequence that's expected
	int currIdx; //used for iteration through the Sndr buffer
} Sndr;

struct Rcvr {
	int currSeq; //current sequence we are awaiting
	struct pkt lastPkt; //last packet B received (simplifies ACK/NAK)
} Rcvr;

struct Sndr A;
struct Rcvr B;

//For the given packet returns the checksum
//There is no real algorithm to it, just kinda does a lot of stuff and hopes for the best :)
int generateChecksum(struct pkt* packet){
	int toRet = 0xFFFF;
	int sum = 0;
	int x = 0;
	int j = 0;
	int i = 0;
	for(i = 0; i < sizeof(int); i++){
		toRet += packet->seqnum >> (i);
		toRet += packet->acknum >> (i);
	}
	for(i = 0; i < MSIZE; i++){
		j = packet->payload[i];
		sum +=((i+1) * j);
		x = ((toRet >> 8) ^ j) & 0xFF;
		x ^= x >> 4;
		toRet = ((toRet << 8) ^ (x << 12) ^ (x << 5) ^ x) & 0xFFFF;
	}
	if(PRINT){
		printf("CHECKSUM(): %d\n", toRet);
	}
	return toRet;
}

//send any waiting packets
void send(){
	while(A.currIdx < A.end && A.currIdx < A.start + WINDOW){ //while we haven't hit the end AND haven't exceeded the send iwndow
		struct pkt *toSend = &A.pktBuff[A.currIdx % QSIZE]; //build packet
		tolayer3(0, *toSend); //send the packet to layer 3
		if(PRINT){
			printf("SEND(): sent packet %d with %s\n", toSend->seqnum, toSend->payload);
		}
		if(A.currIdx == A.start){ //if we sent the packet we are now going to wait on, start timer
			startTimer(0, RTT);
		}
		A.currIdx++; //increment that next packet sent
	}
}

/********* STUDENTS WRITE THE NEXT SEVEN ROUTINES *********/
/* 
 * The routines you will write are detailed below. As noted above, 
 * such procedures in real-life would be part of the operating system, 
 * and would be called by other procedures in the operating system.  
 * All these routines are in layer 4.
 */

/* 
 * A_output(message), where message is a structure of type msg, containing 
 * data to be sent to the B-side. This routine will be called whenever the 
 * upper layer at the sending side (A) has a message to send. It is the job 
 * of your protocol to insure that the data in such a message is delivered 
 * in-order, and correctly, to the receiving side upper layer.
 */
void A_output(struct msg message) {
	if(A.end - A.start >= QSIZE){ //if full packet buffer, drop packets
		if(PRINT){
			printf("Sender packet buffer is full. Packet dropped: %s\n", message.data);
		}
		return;
	}
	struct pkt* sndPkt = &A.pktBuff[A.end % QSIZE];
	sndPkt->seqnum = A.end; 
	sndPkt->acknum = A.nextSeq; 
	memmove(sndPkt->payload, message.data, 20); //copy message data into packet
	sndPkt->checksum = generateChecksum(sndPkt); //generate packet checksum
	A.end++; //push the end of the buffer down one more index
	send(); //send packets
}

/*
 * Just like A_output, but residing on the B side.  USED only when the 
 * implementation is bi-directional.
 * BI-DIRECTIONAL HAS NOT BEEN IMPLEMENTED
 */
void B_output(struct msg message)  {
	if(PRINT){
		printf("This program does not support bi-directional messaging. This function call has been ignored\n");
	}
}

/* 
 * A_input(packet), where packet is a structure of type pkt. This routine 
 * will be called whenever a packet sent from the B-side (i.e., as a result 
 * of a tolayer3() being done by a B-side procedure) arrives at the A-side. 
 * packet is the (possibly corrupted) packet sent from the B-side.
 */
void A_input(struct pkt packet) {
	if(packet.checksum != generateChecksum(&packet)){ //if corrupted packet
		if(PRINT){
			printf("A_INPUT(): Corrupted packet\n");
		}
		return;
	} else if(packet.acknum < A.start){ //Any acknum for a previously successful packet is a NAK
		if(PRINT){
			printf("A_INPUT(): Received NAK using num %d\n", packet.acknum);
		}
		return;
	} else { //successful ACK receipt
		A.start = ++packet.acknum; //move starting position up
		if(PRINT){
			printf("A_INPUT(): Received ACK num %d\n", packet.acknum);
		}
		if(A.start == A.currIdx){ //if no longer waiting on packets
			stopTimer(0); //stop timer
			if(PRINT){
				printf("A_INPUT: Timer stopped\n");
			}
			send(); //send more
		} else { //if there are more packets being waited on
			if(PRINT){
				printf("A_INPUT(): Timer started with %d\n", RTT);
			}
			startTimer(0, RTT);
		}
	}
}

/*
 * A_timerinterrupt()  This routine will be called when A's timer expires 
 * (thus generating a timer interrupt). You'll probably want to use this 
 * routine to control the retransmission of packets. See starttimer() 
 * and stoptimer() in the writeup for how the timer is started and stopped.
 */
void A_timerinterrupt() {
	if(A.start >= A.currIdx) { //if currIdx == start then we aren't waiting on any packets, so no timer needs to be started
		if(PRINT){
			printf("A_TIMERINTERRUPT(): Not waiting on any packets, continuing: %d - %d\n", A.start, A.currIdx);
		}
		return;
	}
	int i = 0;
	for(i = A.start; i < A.currIdx; i++){ //for each packet we are waiting on
		struct pkt *toResend = &A.pktBuff[i % QSIZE];
		tolayer3(0, *toResend); //resend it
		if(PRINT){
			printf("A_TIMERINTERRUPT(): Resent packet %d with %s\n", toResend->seqnum, toResend->payload);
		}
	}
	stopTimer(0); //just in case
	startTimer(0, RTT); //restart timer
	if(PRINT){
		printf("A_TIMERINTERRUPT(): Timer started with %d based on %d - %d\n", RTT, A.start, A.currIdx);
	}
}  

/* The following routine will be called once (only) before any other    */
/* entity A routines are called. You can use it to do any initialization */
//We are starting with sequence 1
void A_init() {
	A.start = 1;
	A.end = 1;
	A.nextSeq = 1;
	A.currIdx = 1;
}


/* 
 * Note that with simplex transfer from A-to-B, there is no routine  B_output() 
 */

/*
 * B_input(packet),where packet is a structure of type pkt. This routine 
 * will be called whenever a packet sent from the A-side (i.e., as a result 
 * of a tolayer3() being done by a A-side procedure) arrives at the B-side. 
 * packet is the (possibly corrupted) packet sent from the A-side.
 */
void B_input(struct pkt packet) {
	if(packet.checksum != generateChecksum(&packet)){ //if we got a corrupted packet
		if(PRINT){
			printf("B_INPUT(): Corrupted packet. Sending NAK %d\n", B.lastPkt.acknum);
		}
		tolayer3(1, B.lastPkt);
	} else if (packet.seqnum != B.currSeq){ //if we got the wrong sequence
		if(PRINT){
			printf("B_INPUT(): Unexpected sequence. Sending NAK %d\n", B.lastPkt.acknum);
		}
		tolayer3(1, B.lastPkt);
	} else { //if successful file transmission
		if(PRINT){
			printf("B_INPUT(): Received valid packet %d with %s\n", packet.seqnum, packet.payload);
		}
		struct msg message;
		memmove(&message.data, packet.payload, 20);
		tolayer5(1, message); //send packet data to layer5
		if(PRINT){
			printf("B_INPUT(): Sending ACK %d\n", packet.seqnum);
		}
		B.lastPkt.acknum = packet.seqnum;
		B.lastPkt.checksum = generateChecksum(&B.lastPkt);
		tolayer3(1, B.lastPkt); //generate ACK and checksum then resend
		B.currSeq++;
	}
}

/*
 * B_timerinterrupt()  This routine will be called when B's timer expires 
 * (thus generating a timer interrupt). You'll probably want to use this 
 * routine to control the retransmission of packets. See starttimer() 
 * and stoptimer() in the writeup for how the timer is started and stopped.
 */
void  B_timerinterrupt() {
	printf("We have not implemented bidirectional messaging. Call ignored\n");
}

/* 
 * The following routine will be called once (only) before any other   
 * entity B routines are called. You can use it to do any initialization 
 */
void B_init() {
	B.currSeq = 1;
	B.lastPkt.seqnum = 0;
	B.lastPkt.acknum = 0;
	memset(B.lastPkt.payload, 0, 20);
	B.lastPkt.checksum = generateChecksum(&B.lastPkt);
}

