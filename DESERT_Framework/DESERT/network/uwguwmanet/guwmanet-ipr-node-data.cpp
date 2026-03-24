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
 * @file   guwmanet-ipr-node-data.cc
 * @author Giovanni Toso
 * @version 1.1.1
 *
 * \brief Provides the implementation of all the methods regarding Data Packets.
 *
 * Provides the implementation of all the methods regarding Data Packets.
 */

#include "guwmanet-ipr-node.h"

extern packet_t PT_GUWMANET_DATA;
extern packet_t PT_GUWMANET_PATH_EST;

int guwmanet_sequence_number_counter = 0;

/* This function initialize a Data Packet passed as argument with the values
 * contained in the routing table.
 */
void
GuwmanetIPRoutingNode::initPktDataPacket(Packet *p)
{
	if (STACK_TRACE)
		std::cout << "> initPktDataPacket()" << std::endl;

	// Common header.
	hdr_cmn *ch = HDR_CMN(p);
	// ch->uid()       = 0; // Set by the Application above.
	// ch->ptype()     = 0; // Set by the Application above.
	// ch->size()      = 0; // Look Down;
	ch->direction() = hdr_cmn::DOWN;
	// ch->next_hop()  = 0; // This value must be set by the invoker.
	// ch->prev_hop_     = ipAddr_;

	// XXX
	// Since GUWMANET uses flooding initially, the MAC next_hop is Broadcast (0)
    ch->next_hop() = 0; 

    // IP header
    hdr_uwip *iph = HDR_UWIP(p);
    iph->saddr() = ipAddr_;
    // The daddr() is usually already set by the application layer

    // GUWMANET Custom Header
    hdr_guwmanet_data *hdata = HDR_GUWMANET_DATA(p);
    hdata->source_id_ = ipAddr_;
    hdata->destination_id_ = iph->daddr(); 
    hdata->last_hop_id_ = ipAddr_; // You are the first hop
    
    // Assign a unique sequence number for this new data packet
    hdata->data_sequence_number_ = guwmanet_sequence_number_counter++;

	// end XXX

	ch->size() += sizeof(hdr_guwmanet_data);
	ch->timestamp() = Scheduler::instance().clock();
} /* GuwmanetIPRoutingNode::initPktDataPacket */

/* Use this function only if the invoker is a node with hop_to_sink > 1,
 * otherwise: drop.
 * This function is used by a node to forward a data packet to the next hop.
 * All the information to route the packet are contained in the packet.
 */
void
GuwmanetIPRoutingNode::forwardDataPacket(Packet *p)
{
	if (STACK_TRACE)
		std::cout << "> forwardDataPacket()" << std::endl;
	hdr_cmn *ch = HDR_CMN(p);
	hdr_uwip *iph = HDR_UWIP(p);
	hdr_guwmanet_data *hdata = HDR_GUWMANET_DATA(p);

	// Look up the next hop in our permanent routing table
    int final_destination = iph->daddr();
    std::map<int, int>::iterator it = perm_routing_table_.find(final_destination);

	// XXX
    if (it != perm_routing_table_.end()) {
		// We found a permanent route!
		int next_hop_node = it->second;
		
		// The Paper says: "After a route is established, all following data 
		// transmissions are sent with the last hop field set to zero."
		hdata->last_hop_id_ = 0;
		
		ch->next_hop() = next_hop_node;
		ch->prev_hop_ = ipAddr_;
		
		if (printDebug_ > 0) {
			std::cout << "[" << NOW << "]::Node[" << printIP(ipAddr_) 
						<< "] USING PERMANENT ROUTE to " << final_destination 
						<< ". Forwarding to Next Hop: " << next_hop_node << std::endl;
		}
		
		sendDown(p, this->getDelay(period_data_));
	} else {
		Packet::free(p);
	}
	
} /* GuwmanetIPRoutingNode::forwardDataPacket */
