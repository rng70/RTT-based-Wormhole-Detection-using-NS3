#include "ns3/aodv-module.h"
#include "ns3/netanim-module.h"
#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/applications-module.h"
#include "ns3/mobility-module.h"
#include "ns3/wifi-module.h"
#include "ns3/flow-monitor-module.h"

NS_LOG_COMPONENT_DEFINE("Wormhole");

using namespace ns3;

void create_nodes(NodeContainer& c, NodeContainer& nm, NodeContainer& m){
    NS_LOG_INFO("Create Nodes");

    c.Create(10);
    
    nm.Add(c.Get(0));
    nm.Add(c.Get(3));
    nm.Add(c.Get(4));
    nm.Add(c.Get(5));
    nm.Add(c.Get(6));
    nm.Add(c.Get(7));
    nm.Add(c.Get(8));
    nm.Add(c.Get(9));
    
    m.Add(c.Get(1));
    m.Add(c.Get(2));
}

void wormhole(int param_count, char* param_list[]){

    bool enableFlowMonitor = false;
    std::string phyMode ("DsssRate1Mbps");

    // Create Nodes
    NodeContainer all_nodes;
    NodeContainer malicious;
    NodeContainer not_malicious;

    create_nodes(all_nodes, not_malicious, malicious);

    // Setup wifi
    WifiHelper wifi;
    YansWifiPhyHelper wifiPhy = YansWifiPhyHelper::
    Default();
    wifiPhy.SetPcapDataLinkType (YansWifiPhyHelper::DLT_IEEE802_11);

    YansWifiChannelHelper wifiChannel;
    wifiChannel.SetPropafationDelay ("ns3::ConstantSpeedPropagationDelayModel");
    

}

int main(int argc, char* argv[]){
    wormhole(argc, argv);
}