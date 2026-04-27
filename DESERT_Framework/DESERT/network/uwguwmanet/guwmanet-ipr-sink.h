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
 * @file   guwmanet-ipr-sink.h
 * @author Giovanni Toso
 * @version 1.1.1
 *
 * \brief Dinamic source routing protocol, this file contains Sinks
 * specifications.
 *
 * Dinamic source routing protocol, this file contains Sinks specifications.
 */

#ifndef GUWMANET_SINK_H
#define GUWMANET_SINK_H

#include "guwmanet-hdr-ack.h"
#include "guwmanet-hdr-data.h"
#include "guwmanet-ipr-common-structures.h"

#include <uwcbr-module.h>
#include <uwip-clmsg.h>
#include <uwip-module.h>

#include "packet.h"
#include <module.h>
#include <tclcl.h>

#include <fstream>
#include <iostream>
#include <rng.h>
#include <sstream>
#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <vector>

/**
 * GuwmanetIPRoutingSink class is used to represent the routing layer of a sink.
 */
class GuwmanetIPRoutingSink : public Module
{
public:

	/**
	 * Constructor of GuwmanetIPRoutingNode class.
	 */
	GuwmanetIPRoutingSink();

	/**
	 * Constructor of GuwmanetIPRoutingNode class.
	 */
	virtual ~GuwmanetIPRoutingSink();

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
	 * Return a string with an IP in the classic form "x.x.x.x" converting an
	 * ns2 nsaddr_t address.
	 *
	 * @param nsaddr_t& ns2 address
	 * @return String that contains a printable IP in the classic form "x.x.x.x"
	 */
	static string printIP(const nsaddr_t &);

	/**
	 * Return a string with an IP in the classic form "x.x.x.x" converting an
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
	 * @see GuwmanetIPRoutingSink::initPktAck()
	 */
	virtual void sendBackAck(const Packet *);

	/**
	 * Initializes an ack packet passed as argument with the default values.
	 *
	 * @param Packet* Pointer to a packet already allocated to fill with the
	 * right values.
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
	 * Returns the number of Ack packets processed by the entire network.
	 *
	 * @return Number of Ack packets processed by the entire network.
	 */
	inline const long &
	getAckCount() const
	{
		return number_of_ackpkt_;
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
	 * Function that accept a list of string and create an entry for the trace
	 * file.
	 */
	virtual string createTraceString(const string &, const double &,
			const int &, const int &, const int &, const int &, const int &,
			const int &, const int &, const double &, const int &, const int &);

	/**
	 * Opens the trace file, writes the string passed as input and closes the
	 * file.
	 *
	 * @param String to write in the trace file.
	 */
	virtual void writeInTrace(const string &);

	/**
	 * Writes in the Path Trace file the path contained in the Packet
	 *
	 * @param Packet to analyze.
	 */
	virtual void writePathInTrace(const Packet *);

	// Internal variables
	nsaddr_t ipAddr_; /**< IP of the current node. */
	double t_probe; /**< Period of the probing. */
	int PoissonTraffic_; /**< Enable (<i>1</i>) or disable (<i>0</i>) the
							Poisson traffic for GUWMANET packets. */
	double periodPoissonTraffic_; /**< Period of the Poisson traffic. */
	double period_status_;
	int printDebug_; /**< Flag to enable or disable dirrefent levels of debug.
					  */
	std::vector<int> seen_uids_; /**< memory bank of received messages */

	// Statistics
	static long number_of_ackpkt_; /**< Comulative number of Ack packets
									  processed by GuwmanetIPRoutingNode objects. */
	int numberofnodes_; /**< Number of nodes in the network, used for
						   statistic purposes. */
	unsigned int **arrayofstats_; /**< Structure that contains the number of
								   * data packets received by the the sink,
								   * for different nodes and for different
								   * values of hop count. It is used for
								   * statistics purposes.
								   */


	// Trace file
	bool trace_; /**< Flag used to enable or disable the trace file for nodes,
				  */
	bool trace_path_; /**< Flag used to enable or disable the path trace file
						 for nodes, */
	char *trace_file_name_; /**< Name of the trace file writter for the
							   current node. */
	char *trace_file_path_name_; /**< Name of the trace file that contains
									the list of paths of the data packets
									received. */
	ostringstream osstream_; /**< Used to convert to string. */
	ofstream trace_file_; /**< Ofstream used to write the trace file in the
							 disk. */
	ofstream trace_file_path_; /**< Ofstream used to write the path trace file
								  in the disk. */
	char trace_separator_; /**< Used as separator among elements in an entr of
							  the tracefile. */

private:
	/**
	 * Copy constructor declared as private. It is not possible to create a new
	 * GuwmanetIPRoutingSink object passing to its constructor another
	 * GuwmanetIPRoutingSink object.
	 *
	 * @param GuwmanetIPRoutingSink& GuwmanetIPRoutingSink object.
	 */
	GuwmanetIPRoutingSink(const GuwmanetIPRoutingSink &);
};

#endif // GUWMANET_SINK_H
