#include "../include/simulator.h"
#include <iostream>
#include <vector>
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
float Timer;
int A_seqnum, A_acknum;
int N;
int B_expected_seqnum;
int next_seqnum;
int base;   //send_base
vector<msg> msg_buffer;     // store message
// References from Chapter 3 lecture ppt.
int get_checksum(struct pkt packet)
{
    int checksum = 0;
    checksum += packet.seqnum;
    checksum += packet.acknum;
    for (int i = 0; i < sizeof(packet.payload); ++i) {
        checksum += packet.payload[i];
    }
    return checksum;
}

bool valid_checksum(struct pkt packet){
    return (packet.checksum == get_checksum(packet));
}

struct pkt make_pkt(int nextseq, struct msg message){
    struct pkt new_pkt;    //initialize packet
    strncpy(new_pkt.payload, message.data, sizeof(new_pkt.payload));
    new_pkt.acknum = A_acknum;
    new_pkt.seqnum = nextseq;
    new_pkt.checksum = get_checksum(new_pkt);
    return new_pkt;
}

/* called from layer 5, passed the data to be sent to other side */
void A_output(struct msg message)
{
	// cout << "Call A_output"<< endl;
    msg_buffer.push_back(message);
    if (next_seqnum < base + N){
        struct pkt packet = make_pkt(next_seqnum, msg_buffer[next_seqnum]);
        tolayer3(0, packet);
        if(base == next_seqnum){
            //cout<<"Timer for seq # "<<next_seq_num<<endl;
            starttimer(0, Timer);
        }
        next_seqnum++;
    }
}


/* called from layer 3, when a packet arrives for layer 4 */
void A_input(struct pkt packet)
{
	// cout << "Call A_input"<< endl;
    if(valid_checksum(packet)){
        base = packet.acknum +1;
        if(base == next_seqnum)
            stoptimer(0);
        else{
			stoptimer(0);
            starttimer(0, Timer);
        }
    }
}

/* called when A's timer goes off */
void A_timerinterrupt()
{
	// cout << "Call A_timerinterrupt"<< endl;
    next_seqnum = base;
    while(next_seqnum < base + N && next_seqnum < msg_buffer.size()){
        msg message = msg_buffer[next_seqnum];
        // Create packet
        tolayer3(0, make_pkt(next_seqnum, message));
		// start timer for packet
        if(base == next_seqnum){
            starttimer(0, Timer);
        }
        next_seqnum++;

    }
}

/* the following routine will be called once (only) before any other */
/* entity A routines are called. You can use it to do any initialization */
void A_init()
{
	// cout << "Call initialization"<< endl;
	N = getwinsize();
	if(N < 50){
		Timer = 25.0;
	} else if (N >= 50 && N < 200){
		Timer = 33.0;
	} else if (N >= 200) {
		Timer = 40.0;
	}
	N = getwinsize();
    A_seqnum = 0;
    A_acknum = 0;
    next_seqnum = 0;
    base = 0;   //send_base
}

/* Note that with simplex transfer from a-to-B, there is no B_output() */

/* called from layer 3, when a packet arrives for layer 4 at B*/
void B_input(struct pkt packet)
{
	// cout << "Call B_input"<< endl;
    if(B_expected_seqnum == packet.seqnum && valid_checksum(packet)){
		// send data to layer 5
        tolayer5(1, packet.payload);
		// create packet
        struct pkt ack_pkt;
        ack_pkt.acknum = B_expected_seqnum;
        ack_pkt.checksum = get_checksum(ack_pkt);
		// send packet
        tolayer3(1, ack_pkt);
        B_expected_seqnum++;
    }
}

/* the following rouytine will be called once (only) before any other */
/* entity B routines are called. You can use it to do any initialization */
void B_init()
{
    B_expected_seqnum = 0;
}