#include "../include/simulator.h"
#include <iostream>
#include <vector>
#include <string.h>
#include <map>

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
int next_seqnum;
int send_base;   //send_base
int rcv_base;   // receiver side base
int B_expect;

struct sndpkt{
    char data[20];
    float start_time;
    bool is_acked;
};

vector<sndpkt> pkts_buffer;
vector<int> record_seq;     // record which seq number has a timer setup
map<int, pkt> rcv_buffer;

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

struct pkt make_pkt(int seqnum, char data[]){
    // cout << "making packet" << endl;
    struct pkt packet;
    strncpy(packet.payload, data, sizeof(packet.payload));
    packet.seqnum = seqnum;
    packet.acknum = A_acknum;
    packet.checksum = get_checksum(packet);
    return packet;
}

/* called from layer 5, passed the data to be sent to other side */
void A_output(struct msg message)
{
    //cout << "Here is A_output" << endl;
    // initialize the packet information
    sndpkt msg_meta;
    strncpy(msg_meta.data,message.data,sizeof(message.data));
    msg_meta.is_acked = false;
    msg_meta.start_time = -1;
    pkts_buffer.push_back(msg_meta);
    // if next available seq # in window, send pkt, set timer
    cout << "send_base: " << send_base << "next_seqnum: "<< next_seqnum << endl;
    if(next_seqnum < send_base + getwinsize()){
        // create a packet with the message data
        struct pkt packet = make_pkt(next_seqnum, message.data);
        tolayer3(0, packet);
        pkts_buffer[next_seqnum].start_time = get_sim_time();
        record_seq.push_back(next_seqnum);
        next_seqnum++;
        if(record_seq.size()==1){
            starttimer(0, Timer);
        }
    }
}

/* called from layer 3, when a packet arrives for layer 4 */
void A_input(struct pkt packet)
{
	// cout << "Here is A_input" << endl;
    if(valid_checksum(packet)){
        if(packet.seqnum < send_base + getwinsize()){
            pkts_buffer[packet.acknum].is_acked = true;
			stoptimer(0);
            if(send_base == packet.acknum){
                while(pkts_buffer.size()>send_base && pkts_buffer[send_base].is_acked){
                    send_base++;
                }
				// cout << "send_base: " << send_base << "next_seqnum: "<< next_seqnum << endl;
                while(next_seqnum < send_base + getwinsize() && next_seqnum < pkts_buffer.size()){
                    if(send_base <= next_seqnum){
                        struct pkt new_pkt = make_pkt(next_seqnum, pkts_buffer[next_seqnum].data);
                        tolayer3(0, new_pkt);
                        pkts_buffer[next_seqnum].start_time = get_sim_time();
                        // store the seq# which has a timer on
                        record_seq.push_back(next_seqnum);
                        next_seqnum++;
                        if(record_seq.size()==1){
                            starttimer(0, Timer);
                        }
                    }
                }
            }
            int seq = record_seq.front();
            if(seq == packet.acknum){
                record_seq.erase(record_seq.begin());
                // remove all the acked seq # in the record list
                while(!record_seq.empty() && record_seq.size() <= getwinsize() && pkts_buffer[record_seq.front()].is_acked){
                    // if pkt with the seq # is acked then remove the seq in the record list
                    record_seq.erase(record_seq.begin());
                }
                if(!record_seq.empty()){
                    int next_timer_seq = record_seq.front();
                    float next_remain_time = get_sim_time() - pkts_buffer[next_timer_seq].start_time;
                    float next_timeout = Timer - next_remain_time;
					stoptimer(0);
                    starttimer(0, next_timeout);
                }
            }
        }
    }
}

/* called when A's timer goes off */
void A_timerinterrupt()
{
    //cout << "Here is A_timerinterrupt" << endl;
    int seq = record_seq.front(); 	//get the unacked packet seq #
    record_seq.erase(record_seq.begin());
    while(!record_seq.empty() && record_seq.size() <= getwinsize() && pkts_buffer[record_seq.front()].is_acked){
        // if pkt with the seq # is acked then remove the seq in the record list
        record_seq.erase(record_seq.begin());
    }
    // resend the packet that has interrupted seq #
    if(seq < send_base + getwinsize() && send_base <= seq){
        struct pkt packet = make_pkt(seq, pkts_buffer[seq].data);
        tolayer3(0, packet);
        pkts_buffer[seq].start_time = get_sim_time();	// reset start time
        record_seq.push_back(seq);	// record the unacked seq number which has a timer
    }
    if(!record_seq.empty()){
        int next_timer_seq = record_seq.front();
        float next_remain_time = get_sim_time() - pkts_buffer[next_timer_seq].start_time;
        float next_timeout = Timer - next_remain_time;
        starttimer(0, next_timeout);
    } else{
		starttimer(0, Timer);
	}
}

/* the following routine will be called once (only) before any other */
/* entity A routines are called. You can use it to do any initialization */
void A_init() {
    //cout << "initial A!" << endl;
	if(getwinsize() <= 500){
		Timer = 32.0;
	} else if (getwinsize() >500){
		Timer = 40.0;
	}
    A_seqnum = 0;
    A_acknum = 0;
    next_seqnum = 0;
    send_base = 0;
}

/* Note that with simplex transfer from a-to-B, there is no B_output() */

/* called from layer 3, when a packet arrives for layer 4 at B*/
void B_input(struct pkt packet)
{
    //cout << "Here is B_input" << endl;
    map<int, pkt>::iterator trace;
    if(valid_checksum(packet)){
        struct pkt ack_pkt;
        ack_pkt.acknum = packet.seqnum;
        ack_pkt.checksum = get_checksum(ack_pkt);
        if(rcv_base == packet.seqnum){
            tolayer5(1, packet.payload);
            rcv_base++;
        } else if(packet.seqnum < rcv_base + getwinsize() && packet.seqnum >= rcv_base){
            rcv_buffer[packet.seqnum] = packet;
        }
        tolayer3(1, ack_pkt);
    }
    //cout << "rcv_buffer size: " << rcv_buffer.size() << endl;
    //cout << "rcv_base and window size: " << rcv_base << '\t' << getwinsize() << endl;
    // send the packet from rcv_base and so on to layer 5
    trace = rcv_buffer.find(rcv_base);
    while (trace != rcv_buffer.end()){
        //cout << rcv_buffer[rcv_base].acknum << '\t' << rcv_buffer[rcv_base].seqnum << endl;
        //cout << "rcv_base: " << trace->first << "   packet.payload: " << trace->second.payload << '\n';
        tolayer5(1, trace->second.payload);
        rcv_buffer.erase(trace);
        rcv_base++;
        trace = rcv_buffer.find(rcv_base);
    }
}

/* the following rouytine will be called once (only) before any other */
/* entity B routines are called. You can use it to do any initialization */
void B_init()
{
    //cout << "initial B!" << endl;
    rcv_base = 0;
}