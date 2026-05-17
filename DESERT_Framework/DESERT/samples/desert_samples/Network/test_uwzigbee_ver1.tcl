# ----------------------------------------------------------------------------------
# UNDERWATER ZIGBEE (IEEE 802.15.4 MAC Rules + Static Mesh Routing)
# ----------------------------------------------------------------------------------

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
load libuwmll.so
load libuwcsmaca.so
load libuwip.so
load libuwstaticrouting.so
load libuwudp.so
load libuwcbr.so

#############################
# NS-Miracle initialization #
#############################
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
set opt(pktsize)            125
set opt(app_period)         60
set opt(node_distance)      2000.0 
set opt(txpower)            140.0 
set opt(ack_mode)           "setAckMode"

if {$opt(bash_parameters)} {
    if {$argc == 6} {
        set opt(pktsize)       [lindex $argv 0]
        set opt(app_period)    [lindex $argv 1]
        set opt(rngstream)     [lindex $argv 2]
        set opt(nn)            [lindex $argv 3]
        set opt(topo)          [lindex $argv 4]
        set opt(distance)      [lindex $argv 5]
    }
}

global defaultRNG
for {set k 0} {$k < $opt(rngstream)} {incr k} {
    $defaultRNG next-substream
}

set opt(tracefilename) "./test_uwzigbee.tr"
set opt(tracefile) [open $opt(tracefilename) w]
set opt(cltracefilename) "./test_uwzigbee.cltr"
set opt(cltracefile) [open $opt(cltracefilename) w]

set channel [new Module/UnderwaterChannel]
set propagation [new MPropagation/Underwater]
set data_mask [new MSpectralMask/Rect]
$data_mask setFreq       $opt(freq)
$data_mask setBandwidth  $opt(bw)

#########################
# Module Configuration  #
#########################
# UW/CBR (Application)
Module/UW/CBR set packetSize_          $opt(pktsize)
Module/UW/CBR set period_              $opt(app_period)
Module/UW/CBR set PoissonTraffic_      1

# =================================================================
# THE ZIGBEE CORE: UW/CSMA_CA (UNDERWATER ADJUSTED MAC)
# =================================================================
Module/UW/CSMA_CA set debug_            0
Module/UW/CSMA_CA set listen_time_      7.0    ;# Wait 7s for acoustic ACKs
Module/UW/CSMA_CA set wait_costant_     0.5    ;# Larger backoffs for water
Module/UW/CSMA_CA set max_tx_tries_     5      ;# Zigbee retransmission limit
Module/UW/CSMA_CA set ack_mode_         "setAckMode" ;# Enforce hop-by-hop ACKs

# UW/PHYSICAL (BPSK Modem & Energy Tracking)              
Module/UW/PHYSICAL  set BitRate_                  $opt(bitrate)
Module/UW/PHYSICAL  set MaxTxSPL_dB_              $opt(txpower)
Module/UW/PHYSICAL  set TxPower_                  10.0 
Module/UW/PHYSICAL  set RxPower_                  1.0  
Module/UW/PHYSICAL  set IdlePower_                0.01 
Module/UW/PHYSICAL  set ConsumedEnergy_           0.0  
Module/UW/PHYSICAL  set debug_                    0

################################
# Procedure(s) to create nodes #
################################
proc createNode { id } {
    global channel propagation data_mask ns app position node udp portnum ipr ipif
    global phy posdb opt mll mac 
    
    set node($id) [$ns create-M_Node $opt(tracefile) $opt(cltracefile)] 

    set app($id)  [new Module/UW/CBR] 
    set udp($id)  [new Module/UW/UDP]
    set ipr($id)  [new Module/UW/StaticRouting]
    set ipif($id) [new Module/UW/IP]
    set mll($id)  [new Module/UW/MLL] 
    set mac($id)  [new Module/UW/CSMA_CA] 
    set phy($id)  [new Module/UW/PHYSICAL]

    # STACK ORDER (APP -> UDP -> IPR -> IPF -> MLL -> MAC -> PHY)
    $node($id) addModule 7 $app($id)   1  "APP"
    $node($id) addModule 6 $udp($id)   1  "UDP"
    $node($id) addModule 5 $ipr($id)   1  "IPR"
    $node($id) addModule 4 $ipif($id)  1  "IPF"   
    $node($id) addModule 3 $mll($id)   1  "MLL"
    $node($id) addModule 2 $mac($id)   1  "MAC"
    $node($id) addModule 1 $phy($id)   1  "PHY"

    $node($id) setConnection $app($id)   $udp($id)   1
    $node($id) setConnection $udp($id)   $ipr($id)   1
    $node($id) setConnection $ipr($id)   $ipif($id)  1
    $node($id) setConnection $ipif($id)  $mll($id)   1
    $node($id) setConnection $mll($id)   $mac($id)   1
    $node($id) setConnection $mac($id)   $phy($id)   1
    $node($id) addToChannel  $channel    $phy($id)   1

    set portnum($id) [$udp($id) assignPort $app($id) ]
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
    if { [catch {$mac($id) addr $id} err] } {
        if { [catch {$mac($id) setMacAddr $id} err] } {
            catch {$mac($id) set macAddr_ $id}
        }
    }
    $mac($id) initialize
}

proc createSink { } {
    global channel propagation data_mask ns app_sink position_sink node_sink udp_sink portnum_sink 
    global phy_data_sink posdb_sink opt mll_sink mac_sink ipr_sink ipif_sink interf_data_sink

    set node_sink [$ns create-M_Node $opt(tracefile) $opt(cltracefile)]

    for {set cnt 0} {$cnt < $opt(nn)} {incr cnt} {
        set app_sink($cnt)  [new Module/UW/CBR] 
    }
    set udp_sink       [new Module/UW/UDP]
    set ipr_sink       [new Module/UW/StaticRouting]
    set ipif_sink      [new Module/UW/IP]
    set mll_sink       [new Module/UW/MLL] 
    set mac_sink       [new Module/UW/CSMA_CA]
    set phy_data_sink  [new Module/UW/PHYSICAL] 

    for { set cnt 0} {$cnt < $opt(nn)} {incr cnt} {
        $node_sink addModule 7 $app_sink($cnt) 0 "APP"
    }
    $node_sink addModule 6 $udp_sink       0 "UDP"
    $node_sink addModule 5 $ipr_sink       0 "IPR"
    $node_sink addModule 4 $ipif_sink      0 "IPF"   
    $node_sink addModule 3 $mll_sink       0 "MLL"
    $node_sink addModule 2 $mac_sink       0 "MAC"
    $node_sink addModule 1 $phy_data_sink  0 "PHY"

    for { set cnt 0} {$cnt < $opt(nn)} {incr cnt} {
        $node_sink setConnection $app_sink($cnt)  $udp_sink 1   
    }
    $node_sink setConnection $udp_sink  $ipr_sink           1
    $node_sink setConnection $ipr_sink  $ipif_sink          1
    $node_sink setConnection $ipif_sink $mll_sink           1 
    $node_sink setConnection $mll_sink  $mac_sink           1
    $node_sink setConnection $mac_sink  $phy_data_sink      1
    $node_sink addToChannel  $channel   $phy_data_sink      1

    for { set cnt 0} {$cnt < $opt(nn)} {incr cnt} {
        set portnum_sink($cnt) [$udp_sink assignPort $app_sink($cnt)]
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
    
    # SAFELY FORCE SINK MAC TO 254
    if { [catch {$mac_sink addr 254} err] } {
        if { [catch {$mac_sink setMacAddr 254} err] } {
            catch {$mac_sink set macAddr_ 254}
        }
    }
    $mac_sink initialize
}

# Initialize Nodes and Sink
for {set id 0} {$id < $opt(nn)} {incr id}  {
    createNode $id
}
createSink

# -------------------------------------------------------------
# Setup Application Flows
# -------------------------------------------------------------
for {set id1 0} {$id1 < $opt(nn)} {incr id1}  {
    $app($id1) set destAddr_ [$ipif_sink addr]
    $app($id1) set destPort_ $portnum_sink($id1)
    $app_sink($id1) set destAddr_ [$ipif($id1) addr]
    $app_sink($id1) set destPort_ $portnum($id1)
}

# -------------------------------------------------------------
# Setup Mesh (Static Routing acting as fixed AODV)
# Node 2 -> Node 1 -> Node 0 -> Sink
# -------------------------------------------------------------
for {set i 0} {$i < $opt(nn)} {incr i}  {
    if {$i == 0} {
        $ipr($i) addRoute [$ipif_sink addr] [$ipif_sink addr]
    } else {
        set nexthop [expr $i - 1]
        $ipr($i) addRoute [$ipif_sink addr] [$ipif($nexthop) addr]
    }
    $ipr_sink addRoute [$ipif($i) addr] [$ipif($i) addr]
}

# -------------------------------------------------------------
# Setup ARP Tables (MAC 254 matching)
# -------------------------------------------------------------
for {set id1 0} {$id1 < $opt(nn)} {incr id1}  {
    for {set id2 0} {$id2 < $opt(nn)} {incr id2}  {
      $mll($id1) addentry [$ipif($id2) addr] $id2
    }   
    $mll($id1) addentry [$ipif_sink addr] 254
    $mll_sink addentry [$ipif($id1) addr] $id1
    
    $mll($id1) addentry 255 255
}
$mll_sink addentry 255 255

# Set Positions
set sinkX 0.0
for {set id 0} {$id < $opt(nn)} {incr id}  {
    $position($id) setX_ [expr $id * $opt(distance)]
    $position($id) setY_ 0.0
    $position($id) setZ_ -1000.0
    puts "NODE_POS: $id [$position($id) getX_] [$position($id) getY_] [$position($id) getZ_]"
    if {$id == 0 } {
        set sinkX [expr [$position($id) getX_] - $opt(distance)]
    }
}
$position_sink setX_ $sinkX
$position_sink setY_ 0.0
$position_sink setZ_ 0.0
puts "NODE_POS: 254 [$position_sink getX_] [$position_sink getY_] [$position_sink getZ_]"

# Start Apps
for {set id1 0} {$id1 < $opt(nn)} {incr id1}  {
    $ns at 10.0    "$app($id1) start"
    $ns at $opt(stoptime)     "$app($id1) stop"
}

# Final Procedure
proc finish {} {
    global ns opt app_sink app ipr ipr_sink phy

    puts "---------------------------------------------------------------------"
    puts "Simulation summary"
    puts "number of nodes  : $opt(nn)"
    puts "packet size      : $opt(pktsize) byte"
    puts "app period       : $opt(app_period) s"
    puts "simulation length: $opt(txduration) s"
    puts "---------------------------------------------------------------------"

    set sum_app_throughput     0
    set sum_app_sent_pkts      0.0
    set sum_app_rcv_pkts       0.0
    set sum_ftt                0.0
    set sum_energy             0.0

    for {set i 0} {$i < $opt(nn)} {incr i}  {
        set app_throughput       [$app_sink($i) getthr]
        set app_sent_pkts        [$app($i) getsentpkts]
        set app_rcv_pkts         [$app_sink($i) getrecvpkts]
        set app_ftt              [$app_sink($i) getftt]
        
        if { [catch {set consumed_energy [$phy($i) set ConsumedEnergy_]} err] } {
            if { [catch {set consumed_energy [$phy($i) getConsumedEnergy]} err2] } {
                set consumed_energy 0.0
            }
        }
        
        set sum_app_throughput   [expr $sum_app_throughput + $app_throughput]
        set sum_app_sent_pkts    [expr $sum_app_sent_pkts + $app_sent_pkts]
        set sum_app_rcv_pkts     [expr $sum_app_rcv_pkts + $app_rcv_pkts]
        set sum_ftt              [expr $sum_ftt + $app_ftt]
        set sum_energy           [expr $sum_energy + $consumed_energy]

        set battery_capacity 10000.0
        set lifetime 0.0
        if {$consumed_energy > 0} {
            set power_draw [expr $consumed_energy / $opt(txduration)]
            set lifetime [expr $battery_capacity / $power_draw]
        }
        
        if ($opt(verbose)) {
            puts "node($i) throughput: $app_throughput"
            puts "node($i) sent:       -> $app_sent_pkts"
            puts "node($i) received:   <- $app_rcv_pkts"
            puts "node($i) E2E Delay (FTT) : $app_ftt s"
            puts "node($i) Energy Consumed : $consumed_energy J"
            puts "node($i) Est. Lifetime   : $lifetime s"
            puts "---------------------------------------"
        }
    }
    
    # -------------------------------------------------------------
    # CALCULATE GLOBAL METRICS
    # -------------------------------------------------------------
    set usefuldata      [expr $sum_app_rcv_pkts * $opt(pktsize)]
    set mean_throughput [expr (double($sum_app_throughput) / $opt(nn))]
    set mean_ftt        [expr $sum_ftt / $opt(nn)]
    set mean_energy     [expr $sum_energy / $opt(nn)]

    puts "Metrics"
    puts "Mean Throughput          : $mean_throughput"
    puts "Sent Packets             : $sum_app_sent_pkts"
    puts "Received Packets         : $sum_app_rcv_pkts"
    
    if {$sum_app_sent_pkts > 0} {
        set pdr [expr ((double($sum_app_rcv_pkts)/ $sum_app_sent_pkts) * 100)]
        set per [expr 100.0 - $pdr]
        puts "Packet Error Rate        : $per %"
        puts "Packet Delivery Ratio    : $pdr %"
    } else {
        puts "Packet Error Rate        : 100.0 %"
        puts "Packet Delivery Ratio    : 0.0 %"
    }
    
    puts "Mean End-to-End Delay    : $mean_ftt s"
    puts "Mean Energy per Node     : $mean_energy J"
    puts "Total Energy used        : $sum_energy J"
    puts "Useful bytes received    : $usefuldata"
    
    $ns flush-trace
    close $opt(tracefile)
}

$ns at [expr $opt(stoptime) + 250.0]  "finish; $ns halt" 
$ns run