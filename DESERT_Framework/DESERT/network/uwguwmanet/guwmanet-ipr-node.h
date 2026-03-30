//
// Copyright (c) 2017 Regents of the SIGNET lab, University of Padova.
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
 * @file   guwmanet-ipr-node.h
 * @author Giovanni Toso
 * @version 1.1.1
 *
 * \brief Dinamic source routing protocol, this file contains Nodes
 * specifications.
 *
 * Dinamic source routing protocol, this file contains Nodes specifications.
 */

#ifndef GUWMANET_NODE_H
#define GUWMANET_NODE_H

#include "guwmanet-hdr-ack.h"
#include "guwmanet-hdr-data.h"
#include "guwmanet-ipr-common-structures.h"

#include <uwcbr-module.h>
#include <uwip-clmsg.h>
#include <uwip-module.h>

#include "mphy.h"
#include "packet.h"
#include <module.h>
#include <tclcl.h>

#include <cmath>
#include <ctime>
#include <fstream>
#include <iostream>
#include <limits>
#include <map>
#include <rng.h>
#include <sstream>
#include <string>
#include <vector>

class GuwmanetIPRoutingNode;

// XXX
struct BufferedPacket {
    Packet* p;
    double recv_time;
    double snr;
    int sender_id;
};

class ListenTimer : public TimerHandler {
public:
    ListenTimer(GuwmanetIPRoutingNode *m) : TimerHandler(), module(m) {}
protected:
    virtual void expire(Event *e);
    GuwmanetIPRoutingNode *module;
};

/**
 * GuwmanetIPRoutingNode class is used to represent the routing layer of a node.
 */
class GuwmanetIPRoutingNode : public Module
{
public:
	/**
	 * Constructor of GuwmanetIPRoutingNode class.
	 */
	GuwmanetIPRoutingNode();

	/**
	 * Destructor of GuwmanetIPRoutingNode class.
	 */
	virtual ~GuwmanetIPRoutingNode();

	static const int LISTLENGTH = 30;
	
	// XXX
	void evaluateBufferAndForward();

protected:
	/*****************************
	 |     Internal Functions    |
	 *****************************/
	/**
	 * TCL command interpreter. It implements the following OTcl methods:
	 *
	 * @param argc Number of arguments in <i>argv</i>.
	 * @param argv Array of strings which are the command parameters (Note that
	 * <i>argv[0]</i> is the name of the object).
	 * @return TCL_OK or TCL_ERROR whether the command has been dispatched
	 * successfully or not.
	 *
	 */
	virtual int command(int, const char *const *);

	/**
	 * Performs the reception of packets from upper and lower layers.
	 *
	 * @param Packet* Pointer to the packet will be received.
	 */
	virtual void recv(Packet *);

	/**
	 * Cross-Layer messages synchronous interpreter.
	 *
	 * @param ClMessage* an instance of ClMessage that represent the message
	 * received
	 * @return <i>0</i> if successful.
	 */
	virtual int recvSyncClMsg(ClMessage *);

	/**
	 * Cross-Layer messages asynchronous interpreter. Used to retrive the IP
	 * od the current node from the IP module.
	 *
	 * @param ClMessage* an instance of ClMessage that represent the message
	 * received and used for the answer.
	 * @return <i>0</i> if successful.
	 */
	virtual int recvAsyncClMsg(ClMessage *);

	/**
	 * Initializes a GuwmanetIPRoutingNode node.
	 * It sends to the lower layers a Sync message asking for the IP of the
	 * node.
	 *
	 * @see UWIPClMsgReqAddr(int src)
	 * @see sendSyncClMsgDown(ClMessage* m)
	 */
	virtual void initialize();

	/**
	 * Returns a string with an IP in the classic form "x.x.x.x" converting an
	 * ns2 nsaddr_t address.
	 *
	 * @param nsaddr_t& ns2 address
	 * @return String that contains a printable IP in the classic form "x.x.x.x"
	 */
	static string printIP(const nsaddr_t &);

	/**
	 * Returns a string with an IP in the classic form "x.x.x.x" converting an
	 * ns2 ns_addr_t address.
	 *
	 * @param ns_addr_t& ns2 address
	 * @return String that contains a printable IP in the classic form "x.x.x.x"
	 */
	static string printIP(const ns_addr_t &);

	/**
	 * Returns a nsaddr_t address from an IP written as a string in the form
	 * "x.x.x.x".
	 *
	 * @param char* IP in string form
	 * @return nsaddr_t that contains the IP converter from the input string
	 */
	static nsaddr_t str2addr(const char *);

	/**
	 * Returns a delay value to use in transmission. The delay can be
	 * <i>0</i> or poissonian accordingly with the flag PoissonTraffic_.
	 *
	 * @return Transmission delay.
	 * @see PoissonTraffic_
	 * @see periodPoissonTraffic_
	 */
	inline const double
	getDelay(const double &period_) const
	{
		if (PoissonTraffic_)
			return (-log(RNG::defaultrng()->uniform_double()) /
					(1.0 / period_));
		else
			return 0;
	}

	/**
	 * Evaluates is the number passed as input is equal to zero. When C++ works
	 * with
	 * double and float number you can't compare them with 0. If the absolute
	 * value of the number is smaller than eplison that means that the number is
	 * equal to zero.
	 *
	 * @param double& Number to evaluate.
	 * @return <i>true</i> if the number passed in input is equal to zero,
	 * <i>false</i> otherwise.
	 * @see std::numeric_limits<double>::epsilon()
	 */
	inline const bool
	isZero(const double &value) const
	{
		return std::fabs(value) < std::numeric_limits<double>::epsilon();
	}


	/*****************************
	 |           Data            |
	 *****************************/
	/**
	 * Initializes a data packet passed as argument with the default values.
	 *
	 * @param Packet* Pointer to a packet already allocated to fill with the
	 * right values.
	 */
	virtual void initPktDataPacket(Packet *);

	/**
	 * Forwards a data packet to the next hop. All the information to route
	 * the packet are contained in the packet. If the current node is in the
	 * coverage
	 * are of the sink it will forward the packet directly to the sink,
	 * otherwise
	 * the packet will be forwarded to the next hop.
	 *
	 * @param Packet* Pointer to a Data packet to forward.
	 */
	virtual void forwardDataPacket(Packet *);

	/*****************************
	 |           Acks            |
	 *****************************/
	/**
	 * Creates an ack packet and sends it to the previous hop using the
	 * information
	 * contained in the header of the data packet passed as input parameter. It
	 * is
	 * an ack to the previous hop, and not to the source of the packet.
	 *
	 * @param Packet* Pointer to a Data packet to acknowledge.
	 * @see GuwmanetIPRoutingNode::initPktAck()
	 * @see GuwmanetIPRoutingNode::clearHops()
	 * @see GuwmanetIPRoutingNode::setNumberOfHopToSink(const int&)
	 */
	virtual void sendBackAck(const Packet *);

	/**
	 * Initializes an ack packet passed as argument with the default values.
	 *
	 * @param Packet* Pointer to a packet already allocated to fill with the
	 * right values.
	 * @see GuwmanetIPRoutingNode::initPktAck(Packet*)
	 */
	virtual void initPktAck(Packet *);

	/*****************************
	 |        Statistics         |
	 *****************************/
	/**
	 * Returns the size in byte of a <i>hdr_guwmanet_ack</i> packet header.
	 *
	 * @return The size of a <i>hdr_guwmanet_ack</i> packet header.
	 */
	static inline const int
	getAckHeaderSize()
	{
		return sizeof(hdr_guwmanet_ack);
	}

	/**
	 * Returns the size in byte of a <i>hdr_guwmanet_data</i> packet header.
	 *
	 * @return The size of a <i>hdr_guwmanet_data</i> packet header.
	 */
	static inline const int
	getDataPktHeaderSize()
	{
		return sizeof(hdr_guwmanet_data);
	}

	/**
	 * Returns the number of Ack packets processed by the entire network.
	 *
	 * @return Number of Ack packets processed by the entire network.
	 */
	inline const long &
	getAckCount() const
	{
		return number_of_ackpkt_;
	}

	/**
	 * Returns the number of Data packets processed by the entire network.
	 *
	 * @return Number of Data packets processed by the entire network.
	 */
	inline const long &
	getDataCount() const
	{
		return number_of_datapkt_;
	}

	/**
	 * Returns the number of Data packets forwarded by the entire network.
	 *
	 * @return Number of Data packets forwarded by the entire network.
	 */
	inline const long &
	getForwardedCount() const
	{
		return number_of_pkt_forwarded_;
	}

	/**
	 * Returns the number of packets dropped by the entire network for the
	 * reason: buffer is full.
	 *
	 * @return Number of packets dropped by the entire network.
	 */
	inline const long &
	getDataDropsCountBuffer() const
	{
		return number_of_drops_buffer_full_;
	}

	/**
	 * Returns the number of packets dropped by the entire network for the
	 * reason: maximum number of retransmission reached.
	 *
	 * @return Number of packets dropped by the entire network.
	 */
	inline const long &
	getDataDropsCountMaxRetx() const
	{
		return number_of_drops_maxretx_;
	}

	/*****************************
	 |         Trace file         |
	 *****************************/
	/**
	 * Traces a packet.
	 *
	 * @param Packet to be traced.
	 * @param String optional for the packet.
	 */
	virtual void tracePacket(
			const Packet *const, const string &position = "UNDEF___");

	/**
	 * Opens the trace file, writes the string passed as input and closes the
	 * file.
	 *
	 * @param String to write in the trace file.
	 */
	virtual void writeInTrace(const string &);

	// Variables
	nsaddr_t ipAddr_; /**< IP of the current node. */
	int PoissonTraffic_; /**< Enable (<i>1</i>) or disable (<i>0</i>) the
							Poisson traffic for GUWMANET packets. */
	double period_status_; /**< Period of the Poisson traffic for status and
							  ack packets. */
	double period_data_; /**< Period of the Poisson traffic for data packets in
							the buffer. */
	int printDebug_; /**< Flag to enable or disable dirrefent levels of debug.
					  */
	double probe_min_snr_; /**< Value below which if a node receives a probe it
							  discards it. */

	// Load
	double list_packets[LISTLENGTH]; /**< List of the last LISTLENGTH temporal
										instants in which the node received data
										packets. */
	double list_acks[LISTLENGTH]; /**< List of the last LISTLENGTH temporal
									 instants in which the node received acks.
									 */
	int pointer_packets_; /**< Pointer of the first avaiable space in
							 list_packets list. */
	int pointer_acks_; /**< Pointer of the first avaiable space in list_acks
						  list. */
	long list_packets_max_time_; /**< Clock of the last packet received by the
									node. */
	long list_acks_max_time_; /**< Clock of the last ack received by the node.
							   */
	bool packets_array_full; /**< <i>true</i> if list of packets is full,
								<i>false</i> otherwise. */
	bool acks_array_full; /**< <i>true</i> if list of acks is full,
							 <i>false</i> otherwise. */
	double alpha_; /**< Parameters used by Load metric. It is a correlation
					  factor. */

	// Buffer
	uint32_t buffer_max_size_; /**< Maximum length of the data buffer. */
	long pkt_stored_; /**< Keep track of the total number of packet
						 transmitted. */
	long pkt_tx_; /**< Keep track of the total number of packet retransmitted.
				   */

	// Trace file
	bool trace_; /**< Flag used to enable or disable the trace file for nodes,
				  */
	char *trace_file_name_; /**< Name of the trace file writter for the
							   current node. */
	ostringstream osstream_; /**< Used to convert to string. */
	ofstream trace_file_; /**< Ofstream used to write the trace file in the
							 disk. */
	char trace_separator_; /**< Used as separator among elements in an entr of
							  the tracefile. */

	// Statistics
	static long number_of_datapkt_; /**< Comulative number of Data packets
									   processed by GuwmanetIPRoutingNode objects. */
	static long number_of_ackpkt_; /**< Comulative number of Ack packets
									  processed by GuwmanetIPRoutingNode objects. */
	static long number_of_drops_buffer_full_; /**< Comulative number of packets
												 dropped by GuwmanetIPRoutingNode
												 objects, reason: the buffer is
												 full. */
	static long number_of_drops_maxretx_; /**< Comulative number of packets
											 dropped by GuwmanetIPRoutingNode
											 objects, reason: max number of
											 retransmission reached. */
	static long number_of_pkt_forwarded_; /**< Comulative number of Data packets
											 forwarded by the network. */

	uint max_retx_; /**< Maximum Number of transmissions performed: real
					   retransmissions counter the counter is increased only
					   when the packet is sent downlayer */
			   
	/*****************************
	 |    Guwmanet specifics      |
	 *****************************/
	// XXX
	std::vector<BufferedPacket> packet_buffer_; 
    std::map<int, int> temp_routing_table_; 
    std::map<int, int> perm_routing_table_;
	std::vector<int> seen_uids_;
	ListenTimer listen_timer_;
private:
	/**
	 * Copy constructor declared as private. It is not possible to create a new
	 * GuwmanetIPRoutingNode object passing to its constructor another
	 * GuwmanetIPRoutingNode object.
	 *
	 * @param GuwmanetIPRoutingNode& GuwmanetIPRoutingNode object.
	 */
	GuwmanetIPRoutingNode(const GuwmanetIPRoutingNode &);
};

#endif // GUWMANET_NODE_H
