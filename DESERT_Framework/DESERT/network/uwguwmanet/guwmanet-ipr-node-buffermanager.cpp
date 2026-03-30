//
// Copyright (c) 2021  Regents of the SIGNET lab, University of Padova.
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions
// are met:
// 1. Redistributions of source code must retain the above copyright
//    notice, this list of conditions and the following disclaimer.
// 2. Redistributions in binary form must reproduce the above copyright
//    notice, this list of conditions and the following disclaimer in the
//    documentation and/or other materials provided with the distribution.
// 3. Neither the name of the University of Padova (SIGNET lab) nor the
//    names of its contributors may be used to endorse or promote products
//    derived from this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
// TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
// PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR
// CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
// EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
// PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
// OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
// WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
// OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
// ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//

/**
 * @file   guwmanet-ipr-node-buffermanager.cpp
 * @author Giovanni Toso
 * @version 1.1.1
 *
 * \brief Provides the implementation of all the methods regarding Buffer
 * Management.
 *
 * Provides the implementation of all the methods regarding Buffer Management.
 */

#include "guwmanet-ipr-node.h"
#include "uwcbr-module.h"

extern packet_t PT_GUWMANET_ACK;
extern packet_t PT_GUWMANET_DATA;

/* Timer. It is invoked automatically when the timer on a sink probe expires.
 */
// XXX 
void
ListenTimer::expire(Event *e)
{
    module->evaluateBufferAndForward();
} /* ListenTimer::expire */


// XXX
/* This is the core of GUWMANET. It implements the math from the paper:
 * 1. Look at all buffered packets (Pd)
 * 2. Filter out those with SNR < SNRmin (Creating P*_d)
 * 3. Pick the first to arrive from P*_d, OR the highest SNR from Pd if P*_d is empty.
 * 4. Forward the packet and nominate the winner as the "last hop".
 */
// XXX
void
GuwmanetIPRoutingNode::evaluateBufferAndForward()
{
    if (STACK_TRACE)
        std::cout << "> evaluateBufferAndForward()" << std::endl;

    if (packet_buffer_.empty()) {
        return; // Nothing to evaluate
    }

    BufferedPacket best_packet;
    bool found_good_snr = false;
    double best_time = 99999999.0;
    double best_bad_snr = -999999.0;
    int winning_neighbor_id = 0;

    // --- PHASE 1 & 2: Filter and Elect the Winner ---
    for (size_t i = 0; i < packet_buffer_.size(); i++) {
        BufferedPacket bp = packet_buffer_[i];

        if (bp.snr >= probe_min_snr_) { 
            // The packet passes the quality check (It belongs to subset P*_d)
            if (!found_good_snr || bp.recv_time < best_time) {
                // We pick the one that arrived FIRST (lowest receive time)
                best_packet = bp;
                best_time = bp.recv_time;
                winning_neighbor_id = bp.sender_id;
                found_good_snr = true;
            }
        } else if (!found_good_snr) {
            // The packet is bad (belongs only to Pd). 
            // We only care about this if we haven't found ANY good packets yet.
            if (bp.snr > best_bad_snr) {
                // We settle for the highest SNR of the bad bunch
                best_packet = bp;
                best_bad_snr = bp.snr;
                winning_neighbor_id = bp.sender_id;
            }
        }
    }

    // --- PHASE 3: Prepare the packet for forwarding ---
    Packet *p_forward = best_packet.p->copy();
    hdr_cmn *ch = HDR_CMN(p_forward);
    hdr_guwmanet_data *hdata = HDR_GUWMANET_DATA(p_forward);

    // The Paper says: "The forwarding node puts its own nickname into the source 
    // address field... As last hop field the nickname of the transmitter is chosen"
    hdata->last_hop_id_ = winning_neighbor_id;
    hdata->source_id_ = ipAddr_; 

    // Configure for MAC layer Broadcast (Flooding)
    ch->prev_hop_ = ipAddr_;
    ch->next_hop() = UWIP_BROADCAST; // MAC layer broadcast address
    ch->direction() = hdr_cmn::DOWN;

    std::cout << "[VIZ] " << Scheduler::instance().clock() << " " << ipAddr_ << " FORWARDED " << ch->uid() << std::endl;
    number_of_pkt_forwarded_++;

    if (printDebug_ > 0) {
        std::cout << "[" << NOW << "]::Node[" << printIP(ipAddr_) 
                  << "] EVALUATED BUFFER. Winner is Node: " << winning_neighbor_id 
                  << ". FORWARDING DATA." << std::endl;
    }

    // Send it down to the MAC layer!
    sendDown(p_forward, 0.01); // 0.01 is a tiny delay to prevent instant collisions

    // --- PHASE 4: Clean up the memory bank ---
    for (size_t i = 0; i < packet_buffer_.size(); i++) {
        Packet::free(packet_buffer_[i].p);
    }
    packet_buffer_.clear();

} /* GuwmanetIPRoutingNode::evaluateBufferAndForward */
