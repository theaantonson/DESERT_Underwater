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
 * @file   guwmanet-ipr-sink.cpp
 * @author Giovanni Toso
 * @version 1.1.1
 *
 * \brief Implements a GuwmanetIPRoutingSink.
 *
 */

#include "guwmanet-ipr-sink.h"
#include "mphy_pktheader.h"
#include "guwmanet-ipr-common-structures.h"

extern packet_t PT_GUWMANET_ACK;
extern packet_t PT_GUWMANET_DATA;
extern packet_t PT_GUWMANET_PROBE;
extern packet_t PT_GUWMANET_PATH_EST;

long GuwmanetIPRoutingSink::number_of_ackpkt_ = 0;

/**
 * Adds the module for GuwmanetIPRoutingSink in ns2.
 */
static class GuwmanetSinkModuleClass : public TclClass
{
public:
	GuwmanetSinkModuleClass()
		: TclClass("Module/UW/GUWMANETSink")
	{
	}

	TclObject *
	create(int, const char *const *)
	{
		return (new GuwmanetIPRoutingSink());
	}
} class_module_guwmanet_sink;


GuwmanetIPRoutingSink::GuwmanetIPRoutingSink()
    : ipAddr_(0)
    , PoissonTraffic_(0)
    , periodPoissonTraffic_(0.1)
    , period_status_(0.1)
    , printDebug_(0)
    , trace_(false)
{
    bind("PoissonTraffic_", &PoissonTraffic_);
    bind("periodPoissonTraffic_", &periodPoissonTraffic_);
    bind("period_status_", &period_status_);
    bind("printDebug_", &printDebug_);
    trace_separator_ = '\t';
}

GuwmanetIPRoutingSink::~GuwmanetIPRoutingSink()
{
	if (STACK_TRACE)
		std::cout << "> ~GuwmanetIPRoutingSink()" << std::endl;
	if (printDebug_ > 10)
		std::cout << "S: " << this->printIP(ipAddr_) << " deleted."
				  << std::endl;
}

int
GuwmanetIPRoutingSink::recvSyncClMsg(ClMessage *m)
{
	if (STACK_TRACE)
		std::cout << "> recvSyncClMsg()" << std::endl;
	return Module::recvSyncClMsg(m);
}

int
GuwmanetIPRoutingSink::recvAsyncClMsg(ClMessage *m)
{
	if (STACK_TRACE)
		std::cout << "> recvAsyncClMsg()" << std::endl;
	if (m->type() == UWIP_CLMSG_SEND_ADDR) {
		UWIPClMsgSendAddr *m_ = (UWIPClMsgSendAddr *) m;
		ipAddr_ = m_->getAddr();
	}
	return Module::recvAsyncClMsg(m);
}

int
GuwmanetIPRoutingSink::command(int argc, const char *const *argv)
{
	if (STACK_TRACE)
		std::cout << "> command()" << std::endl;
	Tcl &tcl = Tcl::instance();
	if (argc == 2) {
		if (strcasecmp(argv[1], "initialize") == 0) {      
            this->initialize();                            
            return TCL_OK;                                 
        } else if (strcasecmp(argv[1], "getackcount") == 0) {
            tcl.resultf("%lu", this->getAckCount());
            return TCL_OK;
        }
    } else if (argc == 3) {
        if (strcasecmp(argv[1], "addr") == 0) {
            ipAddr_ = static_cast<nsaddr_t>(strtol(argv[2], NULL, 10));
            return TCL_OK;
        }
    }
	return Module::command(argc, argv);
}

void
GuwmanetIPRoutingSink::recv(Packet *p)
{
	if (STACK_TRACE)
		std::cout << "> recv()" << std::endl;
	
    hdr_cmn *ch = HDR_CMN(p);
    int type = ch->ptype();

	if (ch->error()) {
        Packet::free(p);
        return;
    }

    if (ch->direction() == hdr_cmn::UP && type != PT_GUWMANET_ACK) {
        if (printDebug_ > 0) {
            std::cout << "[" << NOW << "]::SINK[" << printIP(ipAddr_) 
                      << "] RECEIVED DATA. SENDING ACK." << std::endl;
        }
		Packet *p_copy = p->copy();
        this->sendBackAck(p_copy);

		ch->size() -= sizeof(hdr_guwmanet_data); 
		// ch->ptype() = PT_CBR;

        sendUp(p); // Pass data to Application layer
        return;
    }
    Packet::free(p);
}

void
GuwmanetIPRoutingSink::initialize()
{
	if (STACK_TRACE)
		std::cout << "> initialize()" << std::endl;
	// Asking for the IP of the current Node.
	if (ipAddr_ == 0) {
		UWIPClMsgReqAddr *m = new UWIPClMsgReqAddr(getId());
		m->setDest(CLBROADCASTADDR);
		sendSyncClMsgDown(m);
	}
}


string
GuwmanetIPRoutingSink::printIP(const nsaddr_t &ip_)
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
}

/* This function convert an IP from a ns_addr_t to a string in the
 * classical form: x.x.x.x
 */
string
GuwmanetIPRoutingSink::printIP(const ns_addr_t &ipt_)
{
	return GuwmanetIPRoutingSink::printIP(ipt_.addr_);
} /* GuwmanetIPRoutingSink::printIP */

nsaddr_t
GuwmanetIPRoutingSink::str2addr(const char *str)
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
} /* GuwmanetIPRoutingSink::str2addr */


void
GuwmanetIPRoutingSink::sendBackAck(const Packet *p)
{
	if (STACK_TRACE)
		std::cout << "> sendBackAck()" << std::endl;
	hdr_cmn *ch = HDR_CMN(p);
	hdr_uwcbr *uwcbrh = HDR_UWCBR(p);
	hdr_guwmanet_data *hdata = HDR_GUWMANET_DATA(p);

	Packet *p_ack = Packet::alloc();
	this->initPktAck(p_ack);

	hdr_cmn *ch_ack = HDR_CMN(p_ack);
	hdr_uwip *iph_ack = HDR_UWIP(p_ack);
	hdr_guwmanet_ack *hack = HDR_GUWMANET_ACK(p_ack);

	ch_ack->next_hop() = ch->prev_hop_;
	iph_ack->daddr() = ch->prev_hop_;
	
	hack->source_id_ = ipAddr_;
    hack->destination_id_ = hdata->source_id_;
    hack->data_sequence_number_ = hdata->data_sequence_number_;
    hack->last_hop_id_ = 0;

	if (printDebug_ > 5) {
		std::cout << "[" << NOW << "]::Node[IP:" << this->printIP(ipAddr_)
				  << "]::UID:" << ch->uid()
				  << "::ACK_TO:" << printIP(ch->prev_hop_) << std::endl;
	}
	number_of_ackpkt_++;
	//    cout << printIP(ipAddr_) << ":ack:" << printIP(ch->prev_hop_) << endl;
	if (trace_)
		this->tracePacket(p_ack, "SEND_ACK");
	sendDown(p_ack, this->getDelay(periodPoissonTraffic_));
} /* GuwmanetIPRoutingSink::sendBackAck */

void
GuwmanetIPRoutingSink::initPktAck(Packet *p)
{
	if (STACK_TRACE)
		std::cout << "> initPktPathEstSearch()" << std::endl;

	// Common header.
	hdr_cmn *ch = HDR_CMN(p);
	ch->uid() = guwmanetuid_++;
	ch->ptype() = PT_GUWMANET_ACK;
	// ch->size()     = 0; // Look down.
	ch->direction() = hdr_cmn::DOWN;
	// ch->next_hop()  = 0;
	ch->prev_hop_ = ipAddr_;

	// IP header.
	// iph->daddr() = 0;
	// iph->dport() = 0;
	// iph->saddr() = 0;
	// iph->sport() = 0;

	ch->size() += sizeof(hdr_guwmanet_ack);
	ch->timestamp() = Scheduler::instance().clock();
} /* GuwmanetIPRoutingSink::initPktAck */

void
GuwmanetIPRoutingSink::tracePacket(const Packet *const p, const string &position)
{
	if (STACK_TRACE)
		std::cout << "> tracePacket()" << std::endl;
	hdr_MPhy *ph = HDR_MPHY(p);
	hdr_uwip *iph = HDR_UWIP(p);
	hdr_cmn *ch = HDR_CMN(p);
	hdr_guwmanet_ack *hack = HDR_GUWMANET_ACK(p);
	hdr_uwcbr *uwcbrh = HDR_UWCBR(p);
	if (trace_) {
		double snr_ = 0;
		if (ph->Pn == 0) { // TODO: trick for CMRE logs
			snr_ = -999;
		} else {
			snr_ = 10 * log10(ph->Pr / ph->Pn);
		}
		if (ch->ptype() == PT_GUWMANET_ACK) {
			this->writeInTrace(this->createTraceString(position,
					Scheduler::instance().clock(),
					ipAddr_,
					ch->uid(),
					hack->uid(),
					ch->prev_hop_,
					ch->next_hop(),
					iph->saddr(),
					iph->daddr(),
					snr_,
					ch->direction(),
					ch->ptype()));
		} else {
			this->writeInTrace(this->createTraceString(position,
					Scheduler::instance().clock(),
					ipAddr_,
					ch->uid(),
					uwcbrh->sn(),
					ch->prev_hop_,
					ch->next_hop(),
					iph->saddr(),
					iph->daddr(),
					snr_,
					ch->direction(),
					ch->ptype()));
		}
	}
} /* GuwmanetIPRoutingSink::tracePacket */

string
GuwmanetIPRoutingSink::createTraceString(const string &info_string,
		const double &simulation_time_, const int &node_id_, const int &pkt_id_,
		const int &pkt_sn_, const int &pkt_from_, const int &pkt_next_hop,
		const int &pkt_source_, const int &pkt_destination_, const double &snr_,
		const int &direction_, const int &pkt_type)
{
	if (STACK_TRACE)
		std::cout << "> createTraceString()" << std::endl;
	osstream_.clear();
	osstream_.str("");
	osstream_ << info_string << this->trace_separator_ << simulation_time_
			  << this->trace_separator_ << (node_id_ & 0x000000ff)
			  << this->trace_separator_ << (pkt_id_ & 0x000000ff)
			  << this->trace_separator_ << (pkt_sn_ & 0x000000ff)
			  << this->trace_separator_ << (pkt_from_ & 0x000000ff)
			  << this->trace_separator_ << (pkt_next_hop & 0x000000ff)
			  << this->trace_separator_ << (pkt_source_ & 0x000000ff)
			  << this->trace_separator_ << (pkt_destination_ & 0x000000ff)
			  << this->trace_separator_ << snr_ << this->trace_separator_
			  << direction_ << this->trace_separator_ << pkt_type;
	return osstream_.str();
} /* GuwmanetIPRoutingSink::createTraceString */

void
GuwmanetIPRoutingSink::writeInTrace(const string &string_to_write_)
{
	if (STACK_TRACE)
		std::cout << "> writeInTrace()" << std::endl;

	trace_file_.open(trace_file_name_, fstream::app);
	trace_file_ << string_to_write_ << endl;
	trace_file_.close();
} /* GuwmanetIPRoutingNode::writeInTrace */

void
GuwmanetIPRoutingSink::writePathInTrace(const Packet *p)
{
	if (STACK_TRACE)
		std::cout << "> writePathInTrace()" << std::endl;

	hdr_uwip *iph = HDR_UWIP(p);
	hdr_cmn *ch = HDR_CMN(p);
	hdr_guwmanet_data *hdata = HDR_GUWMANET_DATA(p);

	trace_file_path_.open(trace_file_path_name_, fstream::app);
	osstream_.clear();
	osstream_.str("");
	osstream_ << Scheduler::instance().clock() << '\t' << ch->uid() << '\t'
			  << this->printIP(iph->saddr());

	osstream_ << '\t' << this->printIP(iph->daddr());
	trace_file_path_ << osstream_.str() << endl;
	trace_file_path_.close();
} /*  GuwmanetIPRoutingSink::writePathInTrace */
