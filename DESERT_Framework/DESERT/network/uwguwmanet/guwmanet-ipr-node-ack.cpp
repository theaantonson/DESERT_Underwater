//
// Copyright (c) 2021 Regents of the SIGNET lab, University of Padova.
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
 * @file   guwmanet-ipr-node-ack.cpp
 * @author Giovanni Toso
 * @version 1.1.1
 *
 * \brief Provides the implementation of all the methods regarding Ack Packets.
 *
 * Provides the implementation of all the methods regarding Ack Packets.
 */

#include "guwmanet-ipr-common-structures.h"
#include "guwmanet-ipr-node.h"

/* This function uses the value contained in the header of a uwcbr packet.
 * TODO: try to modifies this in order to use the ch->uid() field instead.
 */
void
GuwmanetIPRoutingNode::sendBackAck(const Packet *p)
{
	// XXX
	if (STACK_TRACE)
        std::cout << "> sendBackAck()" << std::endl;

    hdr_cmn *ch_data = HDR_CMN(p);
    hdr_guwmanet_data *hdata = HDR_GUWMANET_DATA(p);

    // Create the new ACK packet
    Packet *p_ack = Packet::alloc();
    this->initPktAck(p_ack);

    hdr_cmn *ch_ack = HDR_CMN(p_ack);
    hdr_uwip *iph_ack = HDR_UWIP(p_ack);
    hdr_guwmanet_ack *hack = HDR_GUWMANET_ACK(p_ack);

    // 1. Set the MAC next-hop to the neighbor who just handed us the data
    ch_ack->next_hop() = ch_data->prev_hop_;
    iph_ack->daddr() = ch_data->prev_hop_;

    // 2. Fill the GUWMANET ACK Header
    // We send it back to the original source of the flood
    hack->destination_id_ = hdata->source_id_; 
    hack->source_id_ = ipAddr_;
    
    // The paper's "16-bit checksum" so intermediate nodes know which route to use
    hack->data_sequence_number_ = hdata->data_sequence_number_; 

    // The paper states: "During the complete back forwarding the last hop field is set to zero"
    hack->last_hop_id_ = 0; 

    if (printDebug_ > 5) {
        std::cout << "[" << NOW << "]::Node[IP:" << this->printIP(ipAddr_)
                  << "]::ACK_GENERATED_FOR_SN:" << hack->data_sequence_number_ 
                  << " SENDING_TO:" << printIP(ch_ack->next_hop()) << std::endl;
    }
    
    number_of_ackpkt_++;
    
    if (trace_)
        this->tracePacket(p_ack, "SEND_ACK");
        
    // Send to the MAC layer
    sendDown(p_ack, this->getDelay(period_status_));
} /* GuwmanetIPRoutingNode::sendBackAck */

void
GuwmanetIPRoutingNode::initPktAck(Packet *p)
{
	// XXX
	if (STACK_TRACE)
		std::cout << "> initPktAck()" << std::endl;

	// Common header
	hdr_cmn *ch = HDR_CMN(p);
	ch->uid() = guwmanetuid_++;
	ch->ptype() = PT_GUWMANET_ACK; 
	ch->direction() = hdr_cmn::DOWN;
	ch->prev_hop_ = ipAddr_;

	ch->size() += sizeof(hdr_guwmanet_ack);
	ch->timestamp() = Scheduler::instance().clock();
} /* GuwmanetIPRoutingNode::initPktAck */


