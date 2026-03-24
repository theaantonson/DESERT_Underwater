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
 * @file   guwmanet-hdr-ack.h
 * @author Giovanni Toso
 * @version 1.1.1
 *
 * \brief Provides the Ack Messages header description.
 *
 * Provides the Ack Messages header description.
 */

#ifndef HDR_GUWMANET_ACK_H
#define HDR_GUWMANET_ACK_H

#include <packet.h>

#define HDR_GUWMANET_ACK(p) (hdr_guwmanet_ack::access(p))

extern packet_t PT_GUWMANET_ACK;

/**
 * <i>hdr_guwmanet_ack</i> describes acks packets used by <i>UWGUWMANET</i>.
 */
typedef struct hdr_guwmanet_ack {

	int uid_; /**< Uid of the packet acked. */
	static int offset_; /**< Required by the PacketHeaderManager. */
	int source_id_;
	int last_hop_id_;
	int destination_id_;
	int data_sequence_number_;

	/**
	 * Reference to the uid_ variable.
	 */
	inline int &
	uid()
	{
		return uid_;
	}

	/**
	 * Reference to the offset_ variable.
	 */
	inline static int &
	offset()
	{
		return offset_;
	}

	inline static struct hdr_guwmanet_ack *
	access(const Packet *p)
	{
		return (struct hdr_guwmanet_ack *) p->access(offset_);
	}
} hdr_guwmanet_ack;

#endif // HDR_GUWMANET_ACK_H
