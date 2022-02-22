#include "ns3/aodv-module.h"
#include "ns3/netanim-module.h"
#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/applications-module.h"
#include "ns3/mobility-module.h"
#include "ns3/wifi-module.h"
#include "ns3/flow-monitor-module.h"
#include "myapp.h"

NS_LOG_COMPONENT_DEFINE("Wormhole");

using namespace ns3;

void ReceivePacket(Ptr<const Packet> p, const Address &addr)
{
    std::cout << Simulator::Now().GetSeconds() << "\t" << p->GetSize() << "\n";
}

void wormhole(int param_count, char *param_list[])
{
#pragma GCC diagnostic ignored "-Wunused-variable" // "-Wstringop-overflow"

    bool enableFlowMonitor = false;
    int nWifis = 10;
    uint32_t port;
    uint32_t bytesTotal;
    uint32_t packetsReceived;

    std::string m_CSVfileName;
    int m_nSinks;
    std::string m_protocolName;
    double m_txp;
    bool m_traceMobility;
    uint32_t m_protocol;

    double TotalTime = 200.0;
    std::string rate("2048bps");
    std::string phyMode("DsssRate11Mbps");
    std::string tr_name("manet-routing-compare");
    int nodeSpeed = 20; // in m/s
    int nodePause = 0;  // in s
    m_protocolName = "protocol";

    CommandLine cmd;
    cmd.AddValue("EnableMonitor", "Enable Flow Monitor", enableFlowMonitor);
    cmd.AddValue("phyMode", "Wifi Phy mode", phyMode);
    cmd.Parse(param_count, param_list);

    Config::SetDefault("ns3::OnOffApplication::PacketSize", StringValue("64"));
    Config::SetDefault("ns3::OnOffApplication::DataRate", StringValue(rate));

    // Set Non-unicastMode rate to unicast mode
    Config::SetDefault("ns3::WifiRemoteStationManager::NonUnicastMode", StringValue(phyMode));

    // Create Nodes
    NodeContainer cdevices;
    NodeContainer malicious;
    NodeContainer not_malicious;

    cdevices.Create(nWifis);

    not_malicious.Add(cdevices.Get(0));
    not_malicious.Add(cdevices.Get(3));
    not_malicious.Add(cdevices.Get(4));
    not_malicious.Add(cdevices.Get(5));
    not_malicious.Add(cdevices.Get(6));
    not_malicious.Add(cdevices.Get(7));
    not_malicious.Add(cdevices.Get(8));
    not_malicious.Add(cdevices.Get(9));

    malicious.Add(cdevices.Get(1));
    malicious.Add(cdevices.Get(2));

    // Setup wifi
    WifiHelper wifi;
    wifi.SetStandard(WIFI_STANDARD_80211b);

    YansWifiPhyHelper wifiPhy;
    wifiPhy.SetErrorRateModel("ns3::NistErrorRateModel");
    wifiPhy.SetPcapDataLinkType(YansWifiPhyHelper::DLT_IEEE802_11);

    YansWifiChannelHelper wifiChannel;
    wifiChannel.SetPropagationDelay("ns3::ConstantSpeedPropagationDelayModel");

    wifiChannel.AddPropagationLoss("ns3::TwoRayGroundPropagationLossModel", "SystemLoss", DoubleValue(1), "HeightAboveZ", DoubleValue(1.5));

    // For range near 250m
    wifiPhy.Set("TxPowerStart", DoubleValue(33));
    wifiPhy.Set("TxPowerEnd", DoubleValue(33));
    // wifiPhy.Set("TxPowerLevels", UintegerValue(1));
    // wifiPhy.Set("TxGain", DoubleValue(0));
    // wifiPhy.Set("RxGain", DoubleValue(0));
    // wifiPhy.SetEdThreshold(DoubleValue(-61.8));
    // wifiPhy.Set("CcaMode1Threshold", DoubleValue(-64.8));

    wifiPhy.SetChannel(wifiChannel.Create());

    // Add a non-QoS upper mac
    WifiMacHelper wifiMac;
    wifiMac.SetType("ns3::AdhocWifiMac");

    wifi.SetRemoteStationManager("ns3::ConstantRateWifiManager",
                                 "DataMode", StringValue(phyMode),
                                 "ControlMode", StringValue(phyMode));

    NetDeviceContainer devices, mal_devices;
    devices = wifi.Install(wifiPhy, wifiMac, cdevices);
    mal_devices = wifi.Install(wifiPhy, wifiMac, malicious);

    /** added by me **/

    MobilityHelper mobilityAdhoc;
    int64_t streamIndex = 0; // used to get consistent mobility across scenarios

    ObjectFactory pos;
    pos.SetTypeId("ns3::RandomRectanglePositionAllocator");
    pos.Set("X", StringValue("ns3::UniformRandomVariable[Min=0.0|Max=300.0]"));
    pos.Set("Y", StringValue("ns3::UniformRandomVariable[Min=0.0|Max=1500.0]"));

    Ptr<PositionAllocator> taPositionAlloc = pos.Create()->GetObject<PositionAllocator>();
    streamIndex += taPositionAlloc->AssignStreams(streamIndex);

    std::stringstream ssSpeed;
    ssSpeed << "ns3::UniformRandomVariable[Min=0.0|Max=" << nodeSpeed << "]";
    std::stringstream ssPause;
    ssPause << "ns3::ConstantRandomVariable[Constant=" << nodePause << "]";
    mobilityAdhoc.SetMobilityModel("ns3::RandomWaypointMobilityModel",
                                   "Speed", StringValue(ssSpeed.str()),
                                   "Pause", StringValue(ssPause.str()),
                                   "PositionAllocator", PointerValue(taPositionAlloc));
    mobilityAdhoc.SetPositionAllocator(taPositionAlloc);
    mobilityAdhoc.Install(cdevices);
    mobilityAdhoc.Install(malicious);
    mobilityAdhoc.Install(not_malicious);
    streamIndex += mobilityAdhoc.AssignStreams(cdevices, streamIndex);
    streamIndex += mobilityAdhoc.AssignStreams(malicious, streamIndex);
    streamIndex += mobilityAdhoc.AssignStreams(not_malicious, streamIndex);
    NS_UNUSED(streamIndex); // From this point, streamIndex is unused

    //  Enable AODV
    AodvHelper aodv;
    AodvHelper malicious_aodv;

    // Set up internet stack
    InternetStackHelper internet;
    internet.SetRoutingHelper(aodv);
    internet.Install(not_malicious);

    malicious_aodv.Set("EnableWrmAttack", BooleanValue(true)); // putting *false* instead of *true* would disable the malicious behavior of the node

    malicious_aodv.Set("FirstEndWifiWormTunnel", Ipv4AddressValue("10.0.1.1"));
    malicious_aodv.Set("FirstEndWifiWormTunnel", Ipv4AddressValue("10.0.1.2"));

    internet.SetRoutingHelper(malicious_aodv);
    internet.Install(malicious);

    // Set up Addresses
    Ipv4AddressHelper ipv4;
    NS_LOG_INFO("Assign IP Addresses.");
    ipv4.SetBase("10.0.1.0", "255.255.255.0");
    Ipv4InterfaceContainer ifcont = ipv4.Assign(devices);

    ipv4.SetBase("10.1.2.0", "255.255.255.0");
    Ipv4InterfaceContainer mal_ifcont = ipv4.Assign(mal_devices);

    NS_LOG_INFO("Create Applications.");

    // UDP connection from N0 to N3

    uint16_t sinkPort = 6;
    Address sinkAddress(InetSocketAddress(ifcont.GetAddress(3), sinkPort)); // interface of n3
    PacketSinkHelper packetSinkHelper("ns3::TcpSocketFactory", InetSocketAddress(Ipv4Address::GetAny(), sinkPort));
    ApplicationContainer sinkApps = packetSinkHelper.Install(cdevices.Get(3)); // n3 as sink

    sinkApps.Start(Seconds(0.));
    sinkApps.Stop(Seconds(100.));

    Ptr<Socket> ns3UdpSocket = Socket::CreateSocket(cdevices.Get(0), TcpSocketFactory::GetTypeId()); // source at n0

    // Create UDP application at n0
    Ptr<MyApp> app = CreateObject<MyApp>();
    app->Setup(ns3UdpSocket, sinkAddress, 1040, 5, DataRate("250Kbps"));
    cdevices.Get(0)->AddApplication(app);
    app->SetStartTime(Seconds(40.));
    app->SetStopTime(Seconds(100.));

    // Set Mobility for all nodes

    // MobilityHelper mobility;
    // Ptr<ListPositionAllocator> positionAlloc = CreateObject<ListPositionAllocator>();
    // positionAlloc->Add(Vector(100, 0, 0));  // node0
    // positionAlloc->Add(Vector(200, 0, 0));  // node1
    // positionAlloc->Add(Vector(450, 0, 0));  // node2
    // positionAlloc->Add(Vector(550, 0, 0));  // node3
    // positionAlloc->Add(Vector(200, 10, 0)); // node4
    // positionAlloc->Add(Vector(450, 10, 0)); // node5

    // mobility.SetPositionAllocator(positionAlloc);
    // mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
    // mobility.Install(c);

    // AnimationInterface anim("wormhole.xml"); // Mandatory
    // AnimationInterface::SetConstantPosition(cdevices.Get(0), 0, 500);
    // AnimationInterface::SetConstantPosition(cdevices.Get(1), 200, 500);
    // AnimationInterface::SetConstantPosition(cdevices.Get(2), 400, 500);
    // AnimationInterface::SetConstantPosition(cdevices.Get(3), 600, 500);
    // AnimationInterface::SetConstantPosition(cdevices.Get(4), 200, 600);
    // AnimationInterface::SetConstantPosition(cdevices.Get(5), 400, 600);

    // anim.EnablePacketMetadata(true);

    Ptr<OutputStreamWrapper> routingStream = Create<OutputStreamWrapper>("wormhole.routes", std::ios::out);
    aodv.PrintRoutingTableAllAt(Seconds(45), routingStream);

    // Trace Received Packets
    Config::ConnectWithoutContext("/NodeList/*/ApplicationList/*/$ns3::PacketSink/Rx", MakeCallback(&ReceivePacket));

    //
    // Calculate Throughput using Flowmonitor
    //
    FlowMonitorHelper flowmon;
    Ptr<FlowMonitor> monitor = flowmon.InstallAll();

    //
    // Now, do the actual simulation.
    //
    NS_LOG_INFO("Run Simulation.");
    Simulator::Stop(Seconds(100.0));
    Simulator::Run();

    monitor->CheckForLostPackets();

    Ptr<Ipv4FlowClassifier> classifier = DynamicCast<Ipv4FlowClassifier>(flowmon.GetClassifier());
    std::map<FlowId, FlowMonitor::FlowStats> stats = monitor->GetFlowStats();
    for (std::map<FlowId, FlowMonitor::FlowStats>::const_iterator i = stats.begin(); i != stats.end(); ++i)
    {
        Ipv4FlowClassifier::FiveTuple t = classifier->FindFlow(i->first);
        if ((t.sourceAddress == "10.0.1.1" && t.destinationAddress == "10.0.1.4"))
        {
            std::cout << "Source: " << t.sourceAddress << " --> Destination " << t.destinationAddress << std::endl;
            std::cout << "\t  Flow " << i->first << " (" << t.sourceAddress << " -> " << t.destinationAddress << ")\n";
            std::cout << "\t  Tx Bytes:   " << i->second.txBytes << "\n";
            std::cout << "\t  Rx Bytes:   " << i->second.rxBytes << "\n";
            std::cout << "\t  Throughput: " << i->second.rxBytes * 8.0 / (i->second.timeLastRxPacket.GetSeconds() - i->second.timeFirstTxPacket.GetSeconds()) / 1024 / 1024 << " Mbps\n";
            std::cout << "\t  Delay:      " << i->second.delaySum << std::endl;
        }
        else
        {
            std::cout << "Not wormhole: Source: " << t.sourceAddress << " --> Destination " << t.destinationAddress << std::endl;
            std::cout << "\t  Flow " << i->first << " (" << t.sourceAddress << " -> " << t.destinationAddress << ")\n";
            std::cout << "\t  Tx Bytes:   " << i->second.txBytes << "\n";
            std::cout << "\t  Rx Bytes:   " << i->second.rxBytes << "\n";
            std::cout << "\t  Throughput: " << i->second.rxBytes * 8.0 / (i->second.timeLastRxPacket.GetSeconds() - i->second.timeFirstTxPacket.GetSeconds()) / 1024 / 1024 << " Mbps\n";
            std::cout << "\t  Delay:      " << i->second.delaySum << std::endl;
        }
    }

    monitor->SerializeToXmlFile("lab-4.flowmon", true, true);
#pragma GCC diagnostic pop

    Simulator::Destroy();
}

int main(int argc, char *argv[])
{
    wormhole(argc, argv);

    return 0;
}