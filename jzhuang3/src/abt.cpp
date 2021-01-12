#include "../include/simulator.h"
#include <iostream>
#include <queue>
#include <string.h>

using namespace std;

/* ******************************************************************
 ALTERNATING BIT AND GO-BACK-N NETWORK EMULATOR: VERSION 1.1  J.F.Kurose

   This code should be used for PA2, unidirectional data transfer
   protocols (from A to B). Network properties:
   - one way network delay averages five time units (longer if there
     are other messages in the channel for GBN), but can be larger
   - packets can be corrupted (either the header or the data portion)
     or lost, according to user-defined probabilities
   - packets will be delivered in the order in which they were sent
     (although some can be lost).
**********************************************************************/

/********* STUDENTS WRITE THE NEXT SEVEN ROUTINES *********/

int A_seqnum, A_ackflag;
int B_seqnum;
float Timer =  10.0;
struct pkt trans_packet;
queue<msg> msg_buffer;
// References from Chapter 3 lecture ppt.
int get_checksum(struct pkt packet)
{	// calculate checksum
    int checksum = 0;
    checksum += packet.seqnum;
    checksum += packet.acknum;
    for (int i = 0; i < sizeof(packet.payload); ++i) {
        checksum += packet.payload[i];
    }
    return checksum;
}

// return true if checksum is valid
bool valid_checksum(struct pkt packet){
    return (packet.checksum == get_checksum(packet));
}

struct pkt make_pkt(int seq, char data[]){
    struct pkt new_pkt;    //initialize packet
    strncpy(new_pkt.payload, data, 20);
    new_pkt.acknum = 0;
    new_pkt.seqnum = seq;
    new_pkt.checksum = get_checksum(new_pkt);
    return new_pkt;
}

/* called from layer 3, when a packet arrives for layer 4 */
void A_output(struct msg message)
{
	//cout << "Here is A_output" << '\n';
    if(A_ackflag){
        A_ackflag = 0;	// packet waiting
        trans_packet = make_pkt(A_seqnum, message.data);
        tolayer3(0, trans_packet);
        starttimer(0, Timer);
    }
    else{
        msg_buffer.push(message);
    }
}

/* called from layer 3, when a packet arrives for layer 4 */
void A_input(struct pkt packet)
{
	//cout << "Here is A_input" << '\n';
    if(valid_checksum(packet) && packet.acknum == A_seqnum ){
        stoptimer(0);
        A_seqnum = 1 - A_seqnum;	//reverse seq number
        if(!msg_buffer.empty()){
			//cout << "message buffer size: " << msg_buffer.size() << endl;
            msg buffered_msg = msg_buffer.front();
            msg_buffer.pop();
            A_ackflag = 1;
            if (A_ackflag){
				A_ackflag = 0;	// packet waiting
				trans_packet = make_pkt(A_seqnum, buffered_msg.data);
				tolayer3(0, trans_packet);
				starttimer(0, Timer);
			}
        } else {
			A_ackflag = 1;
		}
    }
}

/* called when A's timer goes off */
void A_timerinterrupt()
{
	//cout << "Here is A_timerinterrupt" << '\n';
    tolayer3(0, trans_packet);
    starttimer(0, Timer);
	A_ackflag = 0;
}

/* the following routine will be called once (only) before any other */
/* entity A routines are called. You can use it to do any initialization */
void A_init()
{
	//cout << "Here is A_init" << '\n';
    A_seqnum = 0;
    A_ackflag = 1;
}

/* Note that with simplex transfer from a-to-B, there is no B_output() */

/* called from layer 3, when a packet arrives for layer 4 at B*/
void B_input(struct pkt packet)
{
	//cout << "Here is B_input" << '\n';
	struct pkt ack_pkt;
	//cout << "packet seq #: " << packet.seqnum << "\tB_seqnum: " << endl;
	if(valid_checksum(packet)){
		if(packet.seqnum == B_seqnum){
			tolayer5(1, packet.payload);
			ack_pkt.acknum = B_seqnum;
			ack_pkt.checksum = get_checksum(ack_pkt);
			B_seqnum = 1 - B_seqnum; 		//reverse seq number
		} else{
			ack_pkt.acknum = 1 - B_seqnum;	//reverse seq number
			ack_pkt.checksum = get_checksum(ack_pkt);
		}
		tolayer3(1, ack_pkt);
	}
}

/* the following routine will be called once (only) before any other */
/* entity B routines are called. You can use it to do any initialization */
void B_init()
{
	//cout << "Here is B_init" << '\n';
    B_seqnum = 0;
}