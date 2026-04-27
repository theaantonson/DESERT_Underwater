#include "uwguwal.h"
#include <iostream>

int hdr_guwal::offset_ = 0;

static class UwGuwalClass : public TclClass {
public:
    UwGuwalClass() : TclClass("Module/UW/GUWAL") {}
    TclObject* create(int, const char*const*) {
        return (new UwGuwal());
    }
} class_uwguwal;

UwGuwal::UwGuwal() 
    : Module(),
    sendTmr_(this),
    dstPort_(0),
    dstAddr_(0),
    op_src_addr_(0),
    op_dst_addr_(0),
    parcel_type_(1),
    priority_(0),
    pkts_sent_(0),
    pkts_recv_(0),
    sum_ftt_(0.0)
{
    bind("packetSize_", &packetSize_);
    bind("period_", &period_);
    bind("destPort_", (int *) &dstPort_);
	bind("destAddr_", (int *) &dstAddr_);
    bind("PoissonTraffic_", &PoissonTraffic_);
}

UwGuwal::~UwGuwal() {}

int UwGuwal::command(int argc, const char*const* argv) {
    Tcl& tcl = Tcl::instance();
    
    if (argc == 2) {
        if (strcasecmp(argv[1], "start") == 0) {
            this->start();
            return TCL_OK;
        }
        if (strcasecmp(argv[1], "stop") == 0) {
            this->stop();
            return TCL_OK;
        }
        if (strcasecmp(argv[1], "getsentpkts") == 0) {
            tcl.resultf("%d", pkts_sent_);
            return TCL_OK;
        }
        if (strcasecmp(argv[1], "getrecvpkts") == 0) {
            tcl.resultf("%d", pkts_recv_);
            return TCL_OK;
        }
        if (strcasecmp(argv[1], "getftt") == 0) {
            // Return average FTT. Avoid dividing by zero!
            if (pkts_recv_ > 0) {
                tcl.resultf("%f", sum_ftt_ / pkts_recv_);
            } else {
                tcl.resultf("0.0");
            }
            return TCL_OK;
        }
        if (strcasecmp(argv[1], "getthr") == 0) {
            // Very basic throughput calculation
            double duration = NOW; // Assuming simulation started at 0
            if (duration > 0 && pkts_recv_ > 0) {
                // (Bytes received * 8 bits) / total seconds
                double thr = ((pkts_recv_ * packetSize_ * 8.0) / duration); 
                tcl.resultf("%f", thr);
            } else {
                tcl.resultf("0.0");
            }
            return TCL_OK;
        }
    }

    return Module::command(argc, argv);
}

void UwGuwal::initPkt(Packet* p) {
    hdr_guwal* gh = hdr_guwal::access(p);
    hdr_cmn* ch = HDR_CMN(p);

    gh->parcel_type = (uint16_t)parcel_type_ & 0x03;
    gh->e2e_ack = 0; 
    gh->priority = (uint16_t)priority_ & 0x01;
    gh->src_addr = (uint16_t)op_src_addr_ & 0x3F;
    gh->dst_addr = (uint16_t)op_dst_addr_ & 0x3F;

    hdr_uwip *uwiph = hdr_uwip::access(p);
	uwiph->daddr() = dstAddr_;

	hdr_uwudp *uwudp = hdr_uwudp::access(p);
	uwudp->dport() = dstPort_;
    
    gh->checksum = 0xFFFF;

    ch->size() = 16; 
    ch->ptype() = PT_UWGUWAL;
}

void UwGuwal::sendPkt() {
    Packet* p = Packet::alloc();
    initPkt(p);
    hdr_uwudp* udph = hdr_uwudp::access(p);
    udph->sport() = global_guwal_uid++;
    pkts_sent_++;
    
    if (debug_) std::cout << "GUWAL: Sending 16-byte packet from " 
                          << op_src_addr_ << " to " << op_dst_addr_ << std::endl;
    
    sendDown(p);
}


void UwGuwal::recv(Packet* p) {
    pkts_recv_++;
    
    hdr_cmn* ch = HDR_CMN(p);
    double ftt = NOW - ch->timestamp();
    sum_ftt_ += ftt;

    Packet::free(p); 
}

void UwGuwalSendTimer::expire(Event* e) {
    module->transmit();
}

void UwGuwal::start() {
    sendTmr_.resched(0.01); 
}

void UwGuwal::stop() {
    sendTmr_.cancel();
}

void UwGuwal::transmit() {
    sendPkt(); 

    double delay = period_; 
    
    if (PoissonTraffic_ == 1) {
        delay = -log((double)rand() / RAND_MAX) * period_;
    }
    
    sendTmr_.resched(delay);
}