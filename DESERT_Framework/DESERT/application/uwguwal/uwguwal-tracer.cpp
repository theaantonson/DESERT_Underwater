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


#include "uwguwal.h"
#include <mac.h>

/**
 * Class that defines a tracer for <i>hdr_guwal</i> packets.
 */
class UwGuwalTracer : public Tracer
{
public:
    UwGuwalTracer();

protected:
    void format(Packet *p, SAP *sap);
};

UwGuwalTracer::UwGuwalTracer()
    : Tracer(4) // application = 4
{
}

void
UwGuwalTracer::format(Packet *p, SAP *sap)
{
    hdr_cmn *ch = hdr_cmn::access(p);

    if (ch->ptype() != PT_UWGUWAL)
        return;

    hdr_guwal *hguwal = hdr_guwal::access(p);

    writeTrace(sap, " SRC=%d DST=%d TYPE=%d ACK=%d PRIO=%d", 
               hguwal->src_addr,
               hguwal->dst_addr,
               hguwal->parcel_type,
               hguwal->e2e_ack,
               hguwal->priority);
}

// ------------------------------------------------------------------
// Initialization functions to register the tracer in the NS2/TCL core
// ------------------------------------------------------------------

extern "C" int
Uwguwaltracer_Init()
{
    SAP::addTracer(new UwGuwalTracer);
    return 0;
}

extern "C" int
Cyguwguwaltracer_Init()
{
    Uwguwaltracer_Init();
    return 0;
}