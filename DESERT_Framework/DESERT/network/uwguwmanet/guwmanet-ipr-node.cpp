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
 * @file   guwmanet-ipr-node.cpp
 * @author Giovanni Toso
 * @version 1.2.1
 *
 * \brief Implements a GuwmanetIPRoutingNode.
 *
 */

#include "guwmanet-ipr-node.h"

extern packet_t PT_GUWMANET_PATH_EST;
extern packet_t PT_GUWMANET_PROBE;

long GuwmanetIPRoutingNode::number_of_datapkt_ = 0;
long GuwmanetIPRoutingNode::number_of_ackpkt_ = 0;
long GuwmanetIPRoutingNode::number_of_drops_maxretx_ = 0;
long GuwmanetIPRoutingNode::number_of_drops_buffer_full_ = 0;
long GuwmanetIPRoutingNode::number_of_pkt_forwarded_ = 0;

/**
 * Adds the module for GuwmanetIPRoutingNode in ns2.
 */
static class GuwmanetNodeModuleClass : public TclClass
{
public:
	GuwmanetNodeModuleClass()
		: TclClass("Module/UW/GUWMANETNode")
	{
	}

	TclObject *
	create(int, const char *const *)
	{
		return (new GuwmanetIPRoutingNode());
	}
} class_module_guwmanet_node;

// Constructor for GuwmanetIPRoutingNode class
GuwmanetIPRoutingNode::GuwmanetIPRoutingNode()
	: ipAddr_(0)
    , PoissonTraffic_(0)
    , period_status_(0.1)
    , period_data_(0.1)
    , printDebug_(0)
    , probe_min_snr_(10) // Minimum SNR required to consider a packet "good"
    , buffer_max_size_(100)
    , pkt_stored_(0)
    , pkt_tx_(0)
    , trace_(false)
    , max_retx_(1)
    , listen_timer_(this)
{
	if (STACK_TRACE)
		std::cout << "> GuwmanetIPRoutingNode()" << std::endl;
	bind("PoissonTraffic_", &PoissonTraffic_);
    bind("period_status_", &period_status_);
    bind("period_data_", &period_data_);
    bind("printDebug_", &printDebug_);
    bind("probe_min_snr_", &probe_min_snr_);
    bind("buffer_max_size_", &buffer_max_size_);
    bind("max_retx_", &max_retx_);

	trace_separator_ = '\t';
	//    cout.precision(5);
	//    cout.setf(ios::floatfield, ios::fixed);
} /* GuwmanetIPRoutingNode::GuwmanetIPRoutingNode */

// Destructor for GuwmanetIPRoutingNode class
GuwmanetIPRoutingNode::~GuwmanetIPRoutingNode()
{
	for (size_t i = 0; i < packet_buffer_.size(); i++) {
        Packet::free(packet_buffer_[i].p);
    }
    packet_buffer_.clear();
} /* GuwmanetIPRoutingNode::~GuwmanetIPRoutingNode */

int
GuwmanetIPRoutingNode::recvSyncClMsg(ClMessage *m)
{
	if (STACK_TRACE)
		std::cout << "> recvSyncClMsg()" << std::endl;
	return Module::recvSyncClMsg(m);
} /* GuwmanetIPRoutingNode::recvSyncClMsg */

int
GuwmanetIPRoutingNode::recvAsyncClMsg(ClMessage *m)
{
	if (STACK_TRACE)
		std::cout << "> recvAsyncClMsg()" << std::endl;
	if (m->type() == UWIP_CLMSG_SEND_ADDR) {
		UWIPClMsgSendAddr *m_ = (UWIPClMsgSendAddr *) m;
		ipAddr_ = m_->getAddr();
	}
	return Module::recvAsyncClMsg(m);
} /* GuwmanetIPRoutingNode::recvAsyncClMsg */


/* This function is used to initialize the node. It sends to the lower
 * layers a Sync message asking for the IP.
 * In the tcl file it is important to add a line to call the command that will
 * invoke
 * this function.
 * Example:
 * ...
 * set ipr($id)  [new Module/UW/GUWMANETNode]
 * set ipif($id) [new Module/UW/IP]
 * ...
 * $node($id) addModule 5 $ipr($id)   0  "IPR"
 * $node($id) addModule 4 $ipif($id)  0  "IPF"
 * ...
 * $ipif($id) addr "1.0.0.${tmp_}"
 * ...
 * $ipr($id) initialize
 */
void
GuwmanetIPRoutingNode::initialize()
{
	if (STACK_TRACE)
		std::cout << "> initialize()" << std::endl;
	// Asking for the IP of the current Node.
	if (ipAddr_ == 0) {
		UWIPClMsgReqAddr *m = new UWIPClMsgReqAddr(getId());
		m->setDest(CLBROADCASTADDR);
		sendSyncClMsgDown(m);
	}
} /* GuwmanetIPRoutingNode::initialize */

/* This function convert an IP from a nsaddr_t to a string in the
 * classical form: x.x.x.x. It returns a string, it doesn't print
 * the value in the stdout.
 */
string
GuwmanetIPRoutingNode::printIP(const nsaddr_t &ip_)
{
	if (STACK_TRACE)
		std::cout << "> printIP()" << std::endl;
	stringstream out;
	out << ((ip_ & 0xff000000) >> 24);
	out << ".";
	out << ((ip_ & 0x00ff0000) >> 16);
	out << ".";
	out << ((ip_ & 0x0000ff00) >> 8);
	out << ".";
	out << ((ip_ & 0x000000ff));
	return out.str();
} /* GuwmanetIPRoutingNode::printIP */

/* This function convert an IP from a ns_addr_t to a string in the
 * classical form: x.x.x.x
 */
string
GuwmanetIPRoutingNode::printIP(const ns_addr_t &ipt_)
{
	return GuwmanetIPRoutingNode::printIP(ipt_.addr_);
} /* GuwmanetIPRoutingNode::printIP */

nsaddr_t
GuwmanetIPRoutingNode::str2addr(const char *str)
{
	if (STACK_TRACE)
		std::cout << "> str2addr()" << std::endl;
	int level[4] = {0, 0, 0, 0};
	char tmp[20];
	strncpy(tmp, str, 19);
	tmp[19] = '\0';
	char *p = strtok(tmp, ".");
	for (int i = 0; p && i < 4; p = strtok(NULL, "."), i++) {
		level[i] = atoi(p);
		if (level[i] > 255)
			level[i] = 255;
		else if (level[i] < 0)
			level[i] = 0;
	}
	nsaddr_t addr = 0;
	for (int i = 0; i < 4; i++) {
		addr += (level[i] << 8 * (3 - i));
	}
	return addr;
} /* GuwmanetIPRoutingNode::str2addr */

// Command implementation from nsmiracle. */
int
GuwmanetIPRoutingNode::command(int argc, const char *const *argv)
{
	Tcl &tcl = Tcl::instance();

	if (argc == 2) {
		if (strcasecmp(argv[1], "initialize") == 0) {   
            this->initialize();                          
            return TCL_OK;                               
        } else if (strcasecmp(argv[1], "getackcount") == 0) {
            tcl.resultf("%lu", this->getAckCount());
            return TCL_OK;
        } else if (strcasecmp(argv[1], "getdatacount") == 0) {
            tcl.resultf("%lu", this->getDataCount());
            return TCL_OK;
        } else if (strcasecmp(argv[1], "getforwardedcount") == 0) {
            tcl.resultf("%lu", this->getForwardedCount());
            return TCL_OK;
        } else if (strcasecmp(argv[1], "getdatadropscountbuffer") == 0) {
            tcl.resultf("%lu", this->getDataDropsCountBuffer());
            return TCL_OK;
        } else if (strcasecmp(argv[1], "getdatadropscountmaxretx") == 0) {
            tcl.resultf("%lu", this->getDataDropsCountMaxRetx());
            return TCL_OK;
        }
    } else if (argc == 3) {
        if (strcasecmp(argv[1], "addr") == 0) {
            ipAddr_ = static_cast<nsaddr_t>(strtol(argv[2], NULL, 10));
            if (ipAddr_ == 0) {
                fprintf(stderr, "0 is not a valid IP address");
                return TCL_ERROR;
            }
            return TCL_OK;
        } else if (strcasecmp(argv[1], "trace") == 0) {
            string tmp_ = ((char *) argv[2]);
            trace_file_name_ = new char[tmp_.length() + 1];
            strcpy(trace_file_name_, tmp_.c_str());
            if (trace_file_name_ == NULL) {
                fprintf(stderr, "Empty string for the trace file name");
                return TCL_ERROR;
            }
            trace_ = true;
            return TCL_OK;
        }
    }
    return Module::command(argc, argv);
} /* GuwmanetIPRoutingNode::command */

void
GuwmanetIPRoutingNode::recv(Packet *p)
{
	// XXX hela functionen
	if (STACK_TRACE)
        std::cout << "> recv()" << std::endl;

    hdr_cmn *ch = HDR_CMN(p);
    hdr_uwip *iph = HDR_UWIP(p);
    int type = ch->ptype();

	if (ch->error()) {
        Packet::free(p);
        return;
    }

    // -----------------------------------------------------------
    // PACKET GOING DOWN (From Application to the Ocean)
    // -----------------------------------------------------------
    if (ch->direction() == hdr_cmn::DOWN) {
		if (type != PT_GUWMANET_ACK) { 
            std::cout << "[VIZ] " << NOW << " " << ipAddr_ << " SENT " << ch->uid() << std::endl;
            this->initPktDataPacket(p);
            number_of_datapkt_++;

            // enable forwarding 
            ch->next_hop() = UWIP_BROADCAST; 
            ch->prev_hop_ = ipAddr_;

            seen_uids_.push_back(ch->uid());

            sendDown(p, this->getDelay(period_data_));
            return;
        }

		/*
        
		ch->ptype() = PT_GUWMANET_DATA; 
		this->initPktDataPacket(p);
		
		number_of_datapkt_++;
		
		if (printDebug_ > 0) {
			std::cout << "[" << NOW << "]::Node[" << printIP(ipAddr_) 
						<< "] generated new DATA packet UID: " << ch->uid() << std::endl;
		}

		sendDown(p, this->getDelay(period_data_));
		return;
		if (type != PT_GUWMANET_ACK && type != PT_GUWMANET_DATA) {
		}
		*/
    }

    // -----------------------------------------------------------
    // PACKET GOING UP (From the Ocean to the Application)
    // -----------------------------------------------------------
    if (ch->direction() == hdr_cmn::UP) {
        for (size_t i = 0; i < seen_uids_.size(); i++) {
            if (seen_uids_[i] == ch->uid()) {
                Packet::free(p); 
                return;
            }
        }
        seen_uids_.push_back(ch->uid());

        std::cout << "[VIZ] " << NOW << " " << ipAddr_ << " HEARD " << ch->uid() << std::endl;
        // === HANDLING ACKS ===
        if (type == PT_GUWMANET_ACK) {
            hdr_guwmanet_ack *hack = HDR_GUWMANET_ACK(p);
            
            if (hack->destination_id_ == ipAddr_) {
                // The ACK was meant for ME! I reached the destination successfully.
                std::cout << "[VIZ] " << NOW << " " << ipAddr_ << " IGNORED " << ch->uid() << std::endl;
                if (printDebug_ > 0) {
                    std::cout << "[" << NOW << "]::Node[" << printIP(ipAddr_) 
                              << "] RECEIVED ACK FOR SN: " << hack->data_sequence_number_ << std::endl;
                }
                sendUp(p); // Send to application
            } else {
                // I am an intermediate node on the breadcrumb trail. 
                // Check if I know where to forward this ACK.
                std::map<int, int>::iterator it = temp_routing_table_.find(hack->destination_id_);
                if (it != temp_routing_table_.end()) {
                    int next_hop_for_ack = it->second;
                    
                    // The Paper: "All nodes being involved in the back routing process 
                    // convert the temporary routing entries into permanent ones"
                    perm_routing_table_[hack->destination_id_] = next_hop_for_ack;
                    
                    ch->next_hop() = next_hop_for_ack;
                    ch->prev_hop_ = ipAddr_;
                    ch->direction() = hdr_cmn::DOWN;
                    sendDown(p, 0.01);
                } else {
                    Packet::free(p); // Dead end
                }
            }
            return;
        }

        // === HANDLING DATA (GOSSIPING) ===
        if (type != PT_GUWMANET_ACK) {
            hdr_guwmanet_data *hdata = HDR_GUWMANET_DATA(p);

            // 1. Am I the final destination?
            if (iph->daddr() == ipAddr_) {
                if (printDebug_ > 0) {
                    std::cout << "[" << NOW << "]::Node[" << printIP(ipAddr_) 
                              << "] RECEIVED DATA! Sending ACK." << std::endl;
                }
				Packet *p_copy = p->copy();
                this->sendBackAck(p_copy); // Automatically generates and sends the ACK
                sendUp(p); // Pass data to application
                return;
            }

            // 2. The Overhear Check (Building the breadcrumb trail)
            if (hdata->last_hop_id_ == ipAddr_) {
                // The paper: "If a node overhears that it was elected as last hop... 
                // it generates a temporary routing entry"
                temp_routing_table_[hdata->source_id_] = ch->prev_hop_;
                if (printDebug_ > 5) {
                    std::cout << "[" << NOW << "]::Node[" << printIP(ipAddr_) 
                              << "] OVERHEARD MYSELF AS LAST HOP. Built temp route back to " 
                              << hdata->source_id_ << std::endl;
                }
            }

            // 3. Put the packet in the Memory Bank (Buffer)
            if (packet_buffer_.size() < buffer_max_size_) {
                BufferedPacket bp;
                bp.p = p->copy();
                bp.recv_time = Scheduler::instance().clock();
                
                // Get SNR from MAC layer (this depends on your specific MAC, usually stored in packet info)
                // For now, we will assume a dummy value if MAC doesn't provide it
                bp.snr = 15.0; 
                
                bp.sender_id = ch->prev_hop_;
                packet_buffer_.push_back(bp);

                // Start the Listen Timer if it isn't running
                if (listen_timer_.status() == TIMER_IDLE) {
                    listen_timer_.resched(0.5); // TL + TBackoff (0.5 seconds for now)
                }
            }
            
            Packet::free(p); // Free the original, we are storing the copy
            return;
        }

        // Drop any unrecognized packets
        Packet::free(p);
    }
} /* GuwmanetIPRoutingNode::recv */

void
GuwmanetIPRoutingNode::tracePacket(const Packet *const p, const string &position)
{
	if (STACK_TRACE)
		std::cout << "> tracePacket()" << std::endl;
	hdr_cmn *ch = HDR_CMN(p);
	hdr_uwip *iph = HDR_UWIP(p);
	
	osstream_.clear();
	osstream_.str("");
	osstream_ << Scheduler::instance().clock() << trace_separator_
				<< printIP(ipAddr_) << trace_separator_
				<< position << trace_separator_
				<< ch->uid() << trace_separator_
				<< ch->ptype() << trace_separator_
				<< printIP(iph->saddr()) << trace_separator_
				<< printIP(iph->daddr());
	
	writeInTrace(osstream_.str());
} /* GuwmanetIPRoutingNode::tracePacket */

void
GuwmanetIPRoutingNode::writeInTrace(const string &string_to_write_)
{
	if (STACK_TRACE)
		std::cout << "> writeInTrace()" << std::endl;
	trace_file_.open(trace_file_name_, ios_base::app);
    trace_file_ << string_to_write_ << endl;
    trace_file_.close();
} /* GuwmanetIPRoutingNode::writeInTrace */
