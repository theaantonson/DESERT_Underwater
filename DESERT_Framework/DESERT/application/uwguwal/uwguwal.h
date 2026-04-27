#ifndef UWGUWAL_H
#define UWGUWAL_H

#include <climits>
#include <module.h>
#include <uwip-module.h>
#include <uwudp-module.h>

extern packet_t PT_UWGUWAL;

static int global_guwal_uid = 1;

// defining the guwal package design
// https://ieeexplore.ieee.org/document/6387908 
struct hdr_guwal {
    uint16_t parcel_type : 2;
    uint16_t e2e_ack : 1;    
    uint16_t priority : 1;   
    uint16_t src_addr : 6;   
    uint16_t dst_addr : 6;   
   
    uint8_t payload[12]; 

    uint16_t checksum;           

    static int offset_;
    inline static int& offset() { return offset_; }

    inline static hdr_guwal* access(const Packet* p) {
        return (hdr_guwal*)(p->access(offset_));
    }
};

class UwGuwal;

class UwGuwalSendTimer : public TimerHandler {
public:
    UwGuwalSendTimer(UwGuwal* m) : TimerHandler() { module = m; }
protected:
    virtual void expire(Event* e);
    UwGuwal* module;
    int destAddr_; 
    int destPort_;
};

class UwGuwal : public Module
{
public:
    UwGuwal();
    virtual ~UwGuwal();
    virtual int command(int argc, const char*const* argv);
    virtual void recv(Packet* p);
    virtual void start();
    virtual void stop();
    virtual void transmit();

protected:
    virtual void sendPkt();      
    virtual void initPkt(Packet* p); 

    UwGuwalSendTimer sendTmr_;

    uint16_t dstPort_;
    nsaddr_t dstAddr_;
    int op_src_addr_; 
    int op_dst_addr_;
    int parcel_type_;
    int priority_; 

    // counters for statistics 
    int pkts_sent_;
    int pkts_recv_;
    double sum_ftt_;

    int packetSize_;
    double period_;
    int PoissonTraffic_;
};

#endif