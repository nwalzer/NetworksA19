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

#define QSIZE 128
#define RTT 2000
#define MSIZE 20
#define WINDOW 5

struct Sndr {
	struct pkt pktBuff[QSIZE]; //buffer for packets given from Layer 5
	int end; //total number of packets sent. This % QSIZE gets end of unsent packets
	int start; //This % QSIZE gets start of unsent packets
	int nextSeq; //alternates between 1 and 0 for FSM
	int currIdx; //used for iteration through the Sndr buffer
} Sndr;

struct Rcvr {
	int currSeq; //alternates between 1 and 0 for FSM
	struct pkt lastPkt; //last packet B received (simplifies ACK/NAK)
} Rcvr;

struct Sndr A;
struct Rcvr B;

//For the given packet returns the checksum
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
	return toRet;
}

//send any waiting packets
void send(){
	//int start = A.currIdx;
	while(A.currIdx < A.end && A.currIdx < A.start + WINDOW){
		struct pkt *toSend = &A.pktBuff[A.currIdx % QSIZE];
		tolayer3(0, *toSend);
		printf("SEND(): sent packet %d with %s\n", toSend->seqnum, toSend->payload);
		if(A.currIdx == A.start){
			startTimer(0, RTT);
		}
		A.currIdx++;	
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
	if(A.end - A.start >= QSIZE){
		printf("Sender packet buffer is full. Packet dropped: %s\n", message.data);
		return;
	}
	struct pkt* sndPkt = &A.pktBuff[A.end % QSIZE];
	sndPkt->seqnum = A.end;
	sndPkt->acknum = A.nextSeq;
	memmove(sndPkt->payload, message.data, 20);
	sndPkt->checksum = generateChecksum(sndPkt);
	A.end++;
	send();
}

/*
 * Just like A_output, but residing on the B side.  USED only when the 
 * implementation is bi-directional.
 */
void B_output(struct msg message)  {
	printf("This program does not support bi-directional messaging. This function call has been ignored\n");
}

/* 
 * A_input(packet), where packet is a structure of type pkt. This routine 
 * will be called whenever a packet sent from the B-side (i.e., as a result 
 * of a tolayer3() being done by a B-side procedure) arrives at the A-side. 
 * packet is the (possibly corrupted) packet sent from the B-side.
 */
void A_input(struct pkt packet) {
	if(packet.checksum != generateChecksum(&packet)){
		printf("A_INPUT(): Corrupted packet\n");
		return;
	} else if(packet.acknum < A.start){
		printf("A_INPUT(): Received NAK using num %d\n", packet.acknum);
		return;
	} else {
		A.start = ++packet.acknum;
		printf("A_INPUT(): Received ACK num %d\n", packet.acknum);
		if(A.start == A.currIdx){
			stopTimer(0);
			printf("A_INPUT: Timer stopped\n");
			send();
		} else {
			printf("A_INPUT(): Timer started with %d\n", RTT);
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
		printf("A_TIMERINTERRUPT(): Not waiting on any packets, continuing: %d - %d\n", A.start, A.currIdx);
		return;
	}
	int i = 0;
	for(i = A.start; i < A.currIdx; i++){
		struct pkt *toResend = &A.pktBuff[i % QSIZE];
		tolayer3(0, *toResend);
		printf("A_TIMERINTERRUPT(): Resent packet %d with %s\n", toResend->seqnum, toResend->payload);
	}
	stopTimer(0);
	startTimer(0, RTT);
	printf("A_TIMERINTERRUPT(): Timer started with %d based on %d - %d\n", RTT, A.start, A.currIdx);
}  

/* The following routine will be called once (only) before any other    */
/* entity A routines are called. You can use it to do any initialization */
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
	if(packet.checksum != generateChecksum(&packet)){
		printf("B_INPUT(): Corrupted packet. Sending NAK %d\n", B.lastPkt.acknum);
		tolayer3(1, B.lastPkt);
	} else if (packet.seqnum != B.currSeq){
		printf("B_INPUT(): Unexpected sequence. Sending NAK %d\n", B.lastPkt.acknum);
		tolayer3(1, B.lastPkt);
	} else {
		printf("B_INPUT(): Received valid packet %d with %s\n", packet.seqnum, packet.payload);
		struct msg message;
		memmove(&message.data, packet.payload, 20);
		tolayer5(1, message);

		printf("B_INPUT(): Sending ACK %d\n", packet.seqnum);
		B.lastPkt.acknum = packet.seqnum;
		B.lastPkt.checksum = generateChecksum(&B.lastPkt);
		tolayer3(1, B.lastPkt);
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

