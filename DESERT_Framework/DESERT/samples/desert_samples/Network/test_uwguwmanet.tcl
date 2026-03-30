#
# Copyright (c) 2015 Regents of the SIGNET lab, University of Padova.
# All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions
# are met:
# 1. Redistributions of source code must retain the above copyright
#    notice, this list of conditions and the following disclaimer.
# 2. Redistributions in binary form must reproduce the above copyright
#    notice, this list of conditions and the following disclaimer in the
#    documentation and/or other materials provided with the distribution.
# 3. Neither the name of the University of Padova (SIGNET lab) nor the 
#    names of its contributors may be used to endorse or promote products 
#    derived from this software without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS 
# "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED 
# TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR 
# PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR 
# CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, 
# EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, 
# PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; 
# OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, 
# WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR 
# OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF 
# ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
#
# Author: Giovanni Toso <tosogiov@dei.unipd.it>
# Version: 1.0.0
# NOTE: tcl sample tested on Ubuntu 12.04, 64 bits OS
#
# ----------------------------------------------------------------------------------
# This script depicts a very simple but complete stack in which two nodes send data
# to a common sink. The routes are dinamic and decided by UW/ICRP protocol.
# The application used to generate data is UW/CBR.
# ----------------------------------------------------------------------------------
# Stack
#             Node 1                         Node 2                        Sink
#   +--------------------------+   +--------------------------+   +-------------+------------+
#   |  7. UW/CBR               |   |  7. UW/CBR               |   |  7. UW/CBR  | UW/CBR     |
#   +--------------------------+   +--------------------------+   +-------------+------------+
#   |  6. UW/UDP               |   |  6. UW/UDP               |   |  6. UW/UDP               |
#   +--------------------------+   +--------------------------+   +--------------------------+
#   |  5. UW/GUWMANETNode      |   |  5. UW/GUWMANETNode      |   |  5. UW/GUWMANETSink      |
#   +--------------------------+   +--------------------------+   +--------------------------+
#   |  4. UW/IP                |   |  4. UW/IP                |   |  4. UW/IP                |
#   +--------------------------+   +--------------------------+   +--------------------------+
#   |  3. UW/MLL               |   |  3. UW/MLL               |   |  3. UW/MLL               |
#   +--------------------------+   +--------------------------+   +--------------------------+
#   |  2. UW/CSMA_ALOHA        |   |  2. UW/CSMA_ALOHA        |   |  2. UW/CSMA_ALOHA        |
#   +--------------------------+   +--------------------------+   +--------------------------+
#   |  1. Module/MPhy/BPSK     |   |  1. Module/MPhy/BPSK     |   |  1. Module/MPhy/BPSK     |
#   +--------------------------+   +--------------------------+   +--------------------------+
#            |         |                    |         |                   |         |       
#   +----------------------------------------------------------------------------------------+
#   |                                     UnderwaterChannel                                  |
#   +----------------------------------------------------------------------------------------+

######################################
# Flags to enable or disable options #
######################################
set opt(verbose)            1
set opt(bash_parameters)    1
set opt(trace_files)        1

#####################
# Library Loading   #
#####################
load libMiracle.so
load libMiracleBasicMovement.so
load libmphy.so
load libmmac.so
load libUwmStd.so
load libuwinterference.so
load libuwphy_clmsgs.so
load libuwstats_utilities.so
load libuwphysical.so
load libuwcsmaaloha.so
load libuwip.so
load libuwmll.so
load libuwudp.so
load libuwcbr.so
load libguwmanet.so

#############################
# NS-Miracle initialization #
#############################
# You always need the following two lines to use the NS-Miracle simulator
set ns [new Simulator]
$ns use-Miracle

##################
# Tcl variables  #
##################
set opt(nn)                 3.0 ;# Number of Nodes
set opt(topo)               "line"
set opt(starttime)          1
set opt(stoptime)           100000
set opt(txduration)         [expr $opt(stoptime) - $opt(starttime)]
set opt(rngstream)          1

set opt(maxinterval_)       20.0
set opt(freq)               25000.0
set opt(bw)                 5000.0
set opt(bitrate)            4800.0
set opt(ack_mode)           "setNoAckMode"
set opt(pktsize)       125
set opt(cbr_period)    60
# Parameters used to configure the BPSK module of WOSS
set opt(txpower)	    130.0 

# Module/UW/GUWMANETNode set metrics_ 1

if {$opt(bash_parameters)} {
    if {$argc != 6} {
        puts "The script requires 6 inputs:"
        puts "- the first one is the cbr packet size (byte);"
        puts "- the second one is the cbr poisson period (seconds);"
        puts "- the third one is the rngstream;"
        puts "- 4: number of nodes"
        puts "- 5: topology (line or random)"
        puts "- 6: channel quality / Tx Power dB (e.g. 130.0)"
        puts "example: ns uwgmposition.tcl 125 60 13 line 130.0"
        puts "Please try again."
        return
    } else {
        set opt(pktsize)       [lindex $argv 0]
        set opt(cbr_period)    [lindex $argv 1]
        set opt(rngstream)	   [lindex $argv 2]
        set opt(nn)            [lindex $argv 3]
        set opt(topo)          [lindex $argv 4]
        set opt(txpower)       [lindex $argv 5]
    }
}
set opt(buffer_period) [expr $opt(cbr_period) / 3]
if {$opt(buffer_period) > 20} {
    set opt(buffer_period) 20
}

global defaultRNG
for {set k 0} {$k < $opt(rngstream)} {incr k} {
	$defaultRNG next-substream
}

if {$opt(trace_files)} {
    set opt(tracefilename) "./test_uwguwmanet.tr"
    set opt(tracefile) [open $opt(tracefilename) w]
    set opt(cltracefilename) "./test_uwguwmanet.cltr"
    set opt(cltracefile) [open $opt(tracefilename) w]
} else {
    set opt(tracefilename) "/dev/null"
    set opt(tracefile) [open $opt(tracefilename) w]
    set opt(cltracefilename) "/dev/null"
    set opt(cltracefile) [open $opt(cltracefilename) w]
}

set channel [new Module/UnderwaterChannel]
set propagation [new MPropagation/Underwater]
set data_mask [new MSpectralMask/Rect]
$data_mask setFreq       $opt(freq)
$data_mask setBandwidth  $opt(bw)

#########################
# Module Configuration  #
#########################
# UW/CBR
Module/UW/CBR set packetSize_          $opt(pktsize)
Module/UW/CBR set period_              $opt(cbr_period)
Module/UW/CBR set PoissonTraffic_      1

# UW/GUWMANET
Module/UW/GUWMANETNode set period_data_                  $opt(buffer_period);
# Module/UW/GUWMANETNode set timer_route_validity_	        [expr $opt(stoptime)*100]
# Module/UW/GUWMANETNode set timer_sink_probe_validity_    [expr $opt(stoptime)*100]
# Module/UW/GUWMANETNode set max_ack_error_		        3
Module/UW/GUWMANETNode set buffer_max_size_              5
# Module/UW/GUWMANETNode set timer_buffer_                 $opt(buffer_period);

Module/UW/GUWMANETSink set periodPoissonTraffic_	        0.1
Module/UW/GUWMANETSink set t_probe			            600

# BPSK              
Module/UW/PHYSICAL  set BitRate_                      $opt(bitrate)
Module/UW/PHYSICAL  set MaxTxSPL_dB_                  $opt(txpower)

################################
# Procedure(s) to create nodes #
################################
proc createNode { id } {
    global channel propagation data_mask ns cbr position node udp portnum ipr ipif
    global phy posdb opt mll mac 
    
    set node($id) [$ns create-M_Node $opt(tracefile) $opt(cltracefile)] 

    set cbr($id)  [new Module/UW/CBR] 
    set udp($id)  [new Module/UW/UDP]
    set ipr($id)  [new Module/UW/GUWMANETNode]
    set ipif($id) [new Module/UW/IP]
    set mll($id)  [new Module/UW/MLL] 
    set mac($id)  [new Module/UW/CSMA_ALOHA] 
    set phy($id)  [new Module/UW/PHYSICAL]

    $node($id) addModule 7 $cbr($id)   1  "CBR"
    $node($id) addModule 6 $udp($id)   1  "UDP"
    $node($id) addModule 5 $ipr($id)   1  "IPR"
    $node($id) addModule 4 $ipif($id)  1  "IPF"   
    $node($id) addModule 3 $mll($id)   1  "MLL"
    $node($id) addModule 2 $mac($id)   1  "MAC"
    $node($id) addModule 1 $phy($id)   1  "PHY"

    $node($id) setConnection $cbr($id)   $udp($id)   1
    $node($id) setConnection $udp($id)   $ipr($id)   1
    $node($id) setConnection $ipr($id)   $ipif($id)  1
    $node($id) setConnection $ipif($id)  $mll($id)   1
    $node($id) setConnection $mll($id)   $mac($id)   1
    $node($id) setConnection $mac($id)   $phy($id)   1
    $node($id) addToChannel  $channel    $phy($id)   1

    set portnum($id) [$udp($id) assignPort $cbr($id) ]
    
    set tmp_ [expr ($id) + 1]
    $ipif($id) addr $tmp_
    
    set position($id) [new "Position/BM"]
    $node($id) addPosition $position($id)
    set posdb($id) [new "PlugIn/PositionDB"]
    $node($id) addPlugin $posdb($id) 20 "PDB"
    $posdb($id) addpos [$ipif($id) addr] $position($id)
    
    set interf_data($id) [new "Module/UW/INTERFERENCE"]
    $interf_data($id) set maxinterval_ $opt(maxinterval_)
    $interf_data($id) set debug_       0

    $phy($id) setPropagation $propagation
    $phy($id) setSpectralMask $data_mask
    $phy($id) setInterference $interf_data($id)
    $mac($id) $opt(ack_mode)
    $mac($id) initialize
}

proc createSink { } {

    global channel propagation data_mask ns cbr_sink position_sink node_sink udp_sink portnum_sink 
    global phy_data_sink posdb_sink opt mll_sink mac_sink ipr_sink ipif_sink interf_data_sink

    set node_sink [$ns create-M_Node $opt(tracefile) $opt(cltracefile)]

    for {set cnt 0} {$cnt < $opt(nn)} {incr cnt} {
        set cbr_sink($cnt)  [new Module/UW/CBR] 
    }
    set udp_sink       [new Module/UW/UDP]
    set ipr_sink       [new Module/UW/GUWMANETSink]
    set ipif_sink      [new Module/UW/IP]
    set mll_sink       [new Module/UW/MLL] 
    set mac_sink       [new Module/UW/CSMA_ALOHA]
    set phy_data_sink  [new Module/UW/PHYSICAL] 

    for { set cnt 0} {$cnt < $opt(nn)} {incr cnt} {
        $node_sink addModule 7 $cbr_sink($cnt) 0 "CBR"
    }
    $node_sink addModule 6 $udp_sink       0 "UDP"
    $node_sink addModule 5 $ipr_sink       0 "IPR"
    $node_sink addModule 4 $ipif_sink      0 "IPF"   
    $node_sink addModule 3 $mll_sink       0 "MLL"
    $node_sink addModule 2 $mac_sink       0 "MAC"
    $node_sink addModule 1 $phy_data_sink  0 "PHY"

    for { set cnt 0} {$cnt < $opt(nn)} {incr cnt} {
        $node_sink setConnection $cbr_sink($cnt)  $udp_sink 1   
    }
    $node_sink setConnection $udp_sink  $ipr_sink           1
    $node_sink setConnection $ipr_sink  $ipif_sink          1
    $node_sink setConnection $ipif_sink $mll_sink           1 
    $node_sink setConnection $mll_sink  $mac_sink           1
    $node_sink setConnection $mac_sink  $phy_data_sink      1
    $node_sink addToChannel  $channel   $phy_data_sink      1

    for { set cnt 0} {$cnt < $opt(nn)} {incr cnt} {
        set portnum_sink($cnt) [$udp_sink assignPort $cbr_sink($cnt)]
    }
    
    $ipif_sink addr 254

    set position_sink [new "Position/BM"]
    $node_sink addPosition $position_sink
    set posdb_sink [new "PlugIn/PositionDB"]
    $node_sink addPlugin $posdb_sink 20 "PDB"
    $posdb_sink addpos [$ipif_sink addr] $position_sink

    set interf_data_sink [new "Module/UW/INTERFERENCE"]
    $interf_data_sink set maxinterval_ $opt(maxinterval_)
    $interf_data_sink set debug_       0

    $phy_data_sink setSpectralMask $data_mask
    $phy_data_sink setInterference $interf_data_sink
    $phy_data_sink setPropagation $propagation

    $mac_sink $opt(ack_mode)
    $mac_sink initialize
}

#################
# Node Creation #
#################
# Initialize Nodes and Sink
for {set id 0} {$id < $opt(nn)} {incr id}  {
    createNode $id
}
createSink

################################
# Inter-node module connection #
################################
proc connectNodes {id1} {
    global ipif ipr portnum cbr cbr_sink ipif_sink portnum_sink ipr_sink

    $cbr($id1) set destAddr_ [$ipif_sink addr]
    $cbr($id1) set destPort_ $portnum_sink($id1)
    $cbr_sink($id1) set destAddr_ [$ipif($id1) addr]
    $cbr_sink($id1) set destPort_ $portnum($id1)
}

# Setup flows
for {set id1 0} {$id1 < $opt(nn)} {incr id1}  {
    connectNodes $id1
}

# Fill ARP tables
for {set id1 0} {$id1 < $opt(nn)} {incr id1}  {
    for {set id2 0} {$id2 < $opt(nn)} {incr id2}  {
      $mll($id1) addentry [$ipif($id2) addr] [$mac($id2) addr]
    }   
    $mll($id1) addentry [$ipif_sink addr] [ $mac_sink addr]
    $mll_sink addentry [$ipif($id1) addr] [ $mac($id1) addr]
}

# Setup positions
set json_file [open "positions.json" w]
puts $json_file "\{"

for {set id 0} {$id < $opt(nn)} {incr id}  {
    if {$opt(topo) == "line"} {
        $position($id) setX_ [expr $id * 500.0]
        $position($id) setY_ 0.0
        $position($id) setZ_ -1000.0
    } elseif {$opt(topo) == "random"} {
        $position($id) setX_ [expr rand() * 2000.0]
        $position($id) setY_ [expr rand() * 2000.0]
        $position($id) setZ_ [expr -200.0 - (rand() * 800.0)]
    }
    
    # Save to positions.json
    puts $json_file "  \"$id\": \[[$position($id) getX_], [$position($id) getY_], [$position($id) getZ_]\],"
}

# XXX TODO: where to place sink?
$position_sink setX_ 1000.0
$position_sink setY_ 1000.0
$position_sink setZ_ 0.0

puts $json_file "  \"254\": \[[$position_sink getX_], [$position_sink getY_], [$position_sink getZ_]\]"
puts $json_file "\}"
close $json_file

for {set id 0} {$id < $opt(nn)} {incr id}  {
    $ipr($id) initialize
}
$ipr_sink initialize


# -------------------------------------------------------------
# Debug: check node initialization
# -------------------------------------------------------------
puts "=== Node Early Initialization Check ==="
for {set i 0} {$i < $opt(nn)} {incr i} {
    puts "Node $i:"
    # set ipAddr [$ipr($i) set ipAddr_]
    # puts "  ipAddr_ = $ipAddr"

    # set period_data [$ipr($i) set period_data_]
    # puts "  period_data_ = $period_data"

    # set debug [$ipr($i) set printDebug_]
    # set metrics [$ipr($i) set metrics_]
    # puts "  printDebug_ = $debug"
    # puts "  metrics_ = $metrics"

    foreach var {PoissonTraffic_ safe_timer_buffer_ disable_path_error_} {
        if { [catch {set val [$ipr($i) set $var]} err] } {
            puts "  $var = <not initialized>"
        } else {
            puts "  $var = $val"
        }
    }
    puts "-----------------------------------"
}
puts "=== End of Early Node Initialization Check ==="

#####################
# Start/Stop Timers #
#####################
# Set here the timers to start and/or stop modules (optional)
# e.g., 
for {set id1 0} {$id1 < $opt(nn)} {incr id1}  {
    $ns at $opt(starttime)    "$cbr($id1) start"
    $ns at $opt(stoptime)     "$cbr($id1) stop"
}

###################
# Final Procedure #
###################
# Define here the procedure to call at the end of the simulation
proc finish {} {
    global ns opt
    global cbr_sink cbr ipr ipr_sink phy

    puts "---------------------------------------------------------------------"
    puts "Simulation summary"
    puts "number of nodes  : $opt(nn)"
    puts "packet size      : $opt(pktsize) byte"
    puts "cbr period       : $opt(cbr_period) s"
    puts "simulation length: $opt(txduration) s"
    puts "---------------------------------------------------------------------"

    set sum_cbr_throughput     0
    set sum_cbr_sent_pkts      0.0
    set sum_cbr_rcv_pkts       0.0

    # XXX
    set sum_ftt                0.0
    set sum_energy             0.0

    for {set i 0} {$i < $opt(nn)} {incr i}  {
        set cbr_throughput       [$cbr_sink($i) getthr]
        set cbr_sent_pkts        [$cbr($i) getsentpkts]
        set cbr_rcv_pkts         [$cbr_sink($i) getrecvpkts]
        # XXX
        set cbr_ftt              [$cbr_sink($i) getftt]
        set consumed_energy      [$phy($i) set ConsumedEnergy_]

        set sum_cbr_throughput   [expr $sum_cbr_throughput + $cbr_throughput]
        set sum_cbr_sent_pkts    [expr $sum_cbr_sent_pkts + $cbr_sent_pkts]
        set sum_cbr_rcv_pkts     [expr $sum_cbr_rcv_pkts + $cbr_rcv_pkts]
        # XXX
        set sum_ftt              [expr $sum_ftt + $cbr_ftt]
        set sum_energy           [expr $sum_energy + $consumed_energy]

        # XXX 
        set battery_capacity 10000.0
        set lifetime 0.0
        if {$consumed_energy > 0} {
            set power_draw [expr $consumed_energy / $opt(txduration)]
            set lifetime [expr $battery_capacity / $power_draw]
        }
        
        if ($opt(verbose)) {
            puts "node($i) throughput: $cbr_throughput"
            puts "node($i) sent:       -> $cbr_sent_pkts"
            puts "node($i) received:   <- $cbr_rcv_pkts"
            puts "node($i) E2E Delay (FTT) : $cbr_ftt s"
            puts "node($i) Energy Consumed : $consumed_energy J"
            puts "node($i) Est. Lifetime   : $lifetime s"
            puts "---------------------------------------"
        }
    }
    
    puts "Node Stats"
    set ackcountnode        [$ipr(0) getackcount]
    set ackcountsink        [$ipr_sink getackcount]
    set datacount           [$ipr(0) getdatacount]
    set forwardcount        [$ipr(0) getforwardedcount]
    set dropcountretx       [$ipr(0) getdatadropscountmaxretx]
    set dropcountbuffer     [$ipr(0) getdatadropscountbuffer]
    set usefuldata          [expr $sum_cbr_rcv_pkts * $opt(pktsize)]

    puts "Metrics"
    puts "Mean Throughput          : [expr (double($sum_cbr_throughput)/($opt(nn)))]"
    puts "Sent Packets             : $sum_cbr_sent_pkts"
    puts "Received Packets         : $sum_cbr_rcv_pkts"
    
    if {$sum_cbr_sent_pkts > 0} {
        # XXX
        set pdr [expr ((double($sum_cbr_rcv_pkts)/ $sum_cbr_sent_pkts) * 100)]
        puts "Packet Error Rate        : [expr ((1 - double($sum_cbr_rcv_pkts)/ $sum_cbr_sent_pkts) * 100)] %"
    } else {
        puts "Packet Error Rate        : 0 %"
    }
    # XXX
    set mean_ftt [expr $sum_ftt / $opt(nn)]
    set mean_energy [expr $sum_energy / $opt(nn)]
    puts "Mean End-to-End Delay    : $mean_ftt s"
    puts "Mean Energy per Node     : $mean_energy J"

    puts "Total GUWMANET ACKs      : [expr $ackcountnode + $ackcountsink]"
    puts "Total GUWMANET Data Gen  : $datacount"
    puts "Total Packets Forwarded  : $forwardcount"
    puts "Buffer Drops             : $dropcountbuffer"
    puts "Max Retx Drops           : $dropcountretx"
    puts "Useful bytes received    : $usefuldata"
    
    $ns flush-trace
    close $opt(tracefile)
}


###################
# start simulation
###################
$ns at [expr $opt(stoptime) + 250.0]  "finish; $ns halt" 
$ns run
