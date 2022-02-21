#include "ns3/command-line.h"
#include "ns3/config.h"
#include "ns3/string.h"
#include "ns3/log.h"
#include "ns3/yans-wifi-helper.h"
#include "ns3/ssid.h"
#include "ns3/mobility-helper.h"
#include "ns3/on-off-helper.h"
#include "ns3/yans-wifi-channel.h"
#include "ns3/mobility-model.h"
#include "ns3/packet-sink.h"
#include "ns3/packet-sink-helper.h"
#include "ns3/tcp-westwood.h"
#include "ns3/internet-stack-helper.h"
#include "ns3/ipv4-address-helper.h"
#include "ns3/ipv4-global-routing-helper.h"
#include "ns3/core-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/network-module.h"
#include "ns3/applications-module.h"
#include "ns3/mobility-module.h"
#include "ns3/internet-module.h"
#include "ns3/yans-wifi-helper.h"
#include "ns3/ssid.h"
#include "ns3/flow-monitor-helper.h"
#include "ns3/flow-monitor-module.h"
#include "ns3/traffic-control-module.h"

// Default Network Topology
//
//   Wifi 10.1.3.0
//                 AP
//  *    *    *    *
//  |    |    |    |    10.1.1.0
// n5   n6   n7   n0 -------------- n1   n2   n3   n4
//                   point-to-point  |    |    |    |
//                                   *    *    *    *
//                                     wifi 10.1.2.0

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("Task_A_High_Rate");

std::ofstream throughput("./highrateSimulation/throughput.txt");
Ptr<PacketSink> sink;     /* Pointer to the packet sink application */
uint64_t lastTotalRx = 0; /* The value of the last total received bytes */

void CalculateThroughput()
{
    Time now = Simulator::Now();                                       /* Return the simulator's virtual time. */
    double cur = (sink->GetTotalRx() - lastTotalRx) * (double)8 / 1e5; /* Convert Application RX Packets to MBits. */
    std::cout << now.GetSeconds() << "s: \t" << cur << " Mbit/s" << std::endl;
    throughput << now.GetSeconds() << "s: \t" << cur << " Mbit/s" << std::endl;
    lastTotalRx = sink->GetTotalRx();
    Simulator::Schedule(MilliSeconds(100), &CalculateThroughput);
}

int main(int argc, char *argv[])
{
    /**
     * @brief variable section
    //  */
    int nodeSpeed = 10; /* Speed of nodes in m/s */
    // int nodePause = 0;                     /* Pause time in s */
    int nflows = 10;                       /* Number of flow */
    uint32_t nWifi = 50;                   /* Number of nodes */
    uint32_t nPackets = 2;                 /* Number of packets send per second */
    uint32_t payloadSize = 1024;           /* Transport layer payload size in bytes. */
    std::string dataRate = "4Mbps";        /* Application layer datarate. */
    std::string tcpVariant = "TcpNewReno"; /* TCP variant type. */
    std::string phyRate = "HtMcs7";        /* Physical layer bitrate. */
    double simulationTime = 120;           /* Simulation time in seconds. */

    /* this is for performance management */
    uint32_t SentPackets = 0;
    uint32_t ReceivedPackets = 0;
    uint32_t LostPackets = 0;
    /* variable declaration ends here */

    /* prosessing for cmd persing */
    CommandLine cmd(__FILE__);
    cmd.AddValue("nFlows", "Number of flow", nflows);
    cmd.AddValue("nWifi", "Number of wifi STA devices", nWifi);
    cmd.AddValue("payloadSize", "Payload size in bytes", payloadSize);
    cmd.AddValue("dataRate", "Application data ate", dataRate);
    cmd.AddValue("tcpVariant", "Transport protocol to use: TcpNewReno, TcpHybla, TcpHighSpeed, TcpHtcp, TcpVegas, TcpScalable, TcpVeno, TcpBic, TcpYeah, TcpIllinois, TcpWestwood, TcpWestwoodPlus, TcpLedbat ", tcpVariant);
    cmd.AddValue("phyRate", "Physical layer bitrate", phyRate);
    cmd.AddValue("nPackets", "Total number of packets", nPackets);
    cmd.AddValue("simulationTime", "Simulation time in seconds", simulationTime);
    cmd.Parse(argc, argv);
    /* cmd perse ends here */

    /* this is for selecting and configuring tcp client */
    tcpVariant = std::string("ns3::") + tcpVariant;
    // Select TCP variant
    if (tcpVariant.compare("ns3::TcpWestwoodPlus") == 0)
    {
        // TcpWestwoodPlus is not an actual TypeId name; we need TcpWestwood here
        Config::SetDefault("ns3::TcpL4Protocol::SocketType", TypeIdValue(TcpWestwood::GetTypeId()));
        // the default protocol type in ns3::TcpWestwood is WESTWOOD
        Config::SetDefault("ns3::TcpWestwood::ProtocolType", EnumValue(TcpWestwood::WESTWOODPLUS));
    }
    else
    {
        TypeId tcpTid;
        NS_ABORT_MSG_UNLESS(TypeId::LookupByNameFailSafe(tcpVariant, &tcpTid), "TypeId " << tcpVariant << " not found");
        Config::SetDefault("ns3::TcpL4Protocol::SocketType", TypeIdValue(TypeId::LookupByName(tcpVariant)));
    }

    /* Configure TCP Options */
    Config::SetDefault("ns3::TcpSocket::SegmentSize", UintegerValue(payloadSize));

    /* configuring TCP ends here */

    /* Setting wifi devices starts here */
    NodeContainer p2pNodes;
    p2pNodes.Create(2);

    PointToPointHelper pointToPoint;
    pointToPoint.SetDeviceAttribute("DataRate", StringValue(dataRate));
    pointToPoint.SetChannelAttribute("Delay", StringValue("10ms"));

    NetDeviceContainer p2pDevices;
    p2pDevices = pointToPoint.Install(p2pNodes);

    /**
     * @brief from here
     */
    NodeContainer leftStaNodes, rightStaNodes;
    leftStaNodes.Create(nWifi);
    rightStaNodes.Create(nWifi);

    NodeContainer leftAp = p2pNodes.Get(0);
    NodeContainer rightAp = p2pNodes.Get(1);

    YansWifiChannelHelper leftWifiChannel = YansWifiChannelHelper::Default();
    YansWifiChannelHelper rightWifiChannel = YansWifiChannelHelper::Default();

    YansWifiPhyHelper leftWifiPhy;
    YansWifiPhyHelper rightWifiPhy;
    leftWifiPhy.SetChannel(leftWifiChannel.Create());
    rightWifiPhy.SetChannel(rightWifiChannel.Create());
    leftWifiPhy.SetPcapDataLinkType(WifiPhyHelper::DLT_IEEE802_11_RADIO);
    rightWifiPhy.SetPcapDataLinkType(WifiPhyHelper::DLT_IEEE802_11_RADIO);

    WifiHelper leftWifiHelper;
    WifiHelper rightWifiHelper;
    leftWifiHelper.SetRemoteStationManager("ns3::AarfWifiManager");
    rightWifiHelper.SetRemoteStationManager("ns3::AarfWifiManager");
    // wifiHelper.SetStandard ("WIFI_STANDARD_80211n_5GHZ"); //TODO delete
    // wifiHelper.SetRemoteStationManager("ns3::ConstantRateWifiManager",
    //                                    "DataMode", StringValue(phyRate),
    //                                    "ControlMode", StringValue("HtMcs0")); // TODO delete

    WifiMacHelper leftWifiMac;
    WifiMacHelper rightWifiMac;

    /* Configure STA */
    Ssid ssid = Ssid("network");
    leftWifiMac.SetType("ns3::StaWifiMac",
                        "Ssid", SsidValue(ssid), "ActiveProbing", BooleanValue(false)); // TODO check
    rightWifiMac.SetType("ns3::StaWifiMac",
                         "Ssid", SsidValue(ssid), "ActiveProbing", BooleanValue(false)); // TODO check

    NetDeviceContainer leftStaDevices;
    NetDeviceContainer rightStaDevices;
    leftStaDevices = leftWifiHelper.Install(leftWifiPhy, leftWifiMac, leftStaNodes);
    rightStaDevices = rightWifiHelper.Install(rightWifiPhy, rightWifiMac, rightStaNodes);

    /* Configure AP */
    leftWifiMac.SetType("ns3::ApWifiMac", "Ssid", SsidValue(ssid));
    rightWifiMac.SetType("ns3::ApWifiMac", "Ssid", SsidValue(ssid));

    NetDeviceContainer leftApDevice, rightApDevice;
    leftApDevice = leftWifiHelper.Install(leftWifiPhy, leftWifiMac, leftAp);
    rightApDevice = rightWifiHelper.Install(rightWifiPhy, rightWifiMac, rightAp);

    /*  Mobility Model */
    MobilityHelper mobility;

    mobility.SetPositionAllocator("ns3::GridPositionAllocator",
                                  "MinX", DoubleValue(0.0),
                                  "MinY", DoubleValue(0.0),
                                  "DeltaX", DoubleValue(0.5),
                                  "DeltaY", DoubleValue(1.0),
                                  "GridWidth", UintegerValue(3),
                                  "LayoutType", StringValue("RowFirst"));

    mobility.SetMobilityModel("ns3::ConstantVelocityMobilityModel");
    mobility.Install(leftStaNodes);
    mobility.Install(rightStaNodes);
    for (uint n = 0; n < leftStaNodes.GetN(); n++)
    {
        Ptr<ConstantVelocityMobilityModel> mob = leftStaNodes.Get(n)->GetObject<ConstantVelocityMobilityModel>();
        mob->SetVelocity(Vector(nodeSpeed, 0, 0));
    }

    for (uint n = 0; n < rightStaNodes.GetN(); n++)
    {
        Ptr<ConstantVelocityMobilityModel> mob = rightStaNodes.Get(n)->GetObject<ConstantVelocityMobilityModel>();
        mob->SetVelocity(Vector(nodeSpeed, 0, 0));
    }

    mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
    mobility.Install(leftAp);
    mobility.Install(rightAp);

    // mobility.SetMobilityModel("ns3::RandomWalk2dMobilityModel", "Bounds", RectangleValue(Rectangle(-100, 100, -100, 100)));

    // MobilityHelper mobilityAdhoc;
    // int64_t streamIndex1 = 0; // used to get consistent mobility across scenarios
    // int64_t streamIndex2 = 0; // used to get consistent mobility across scenarios
    // int64_t streamIndex3 = 0; // used to get consistent mobility across scenarios
    // int64_t streamIndex4 = 0; // used to get consistent mobility across scenarios

    // ObjectFactory pos;
    // pos.SetTypeId("ns3::RandomRectanglePositionAllocator");
    // pos.Set("X", StringValue("ns3::UniformRandomVariable[Min=0.0|Max=300.0]"));
    // pos.Set("Y", StringValue("ns3::UniformRandomVariable[Min=0.0|Max=1500.0]"));

    // Ptr<PositionAllocator> taPositionAlloc = pos.Create()->GetObject<PositionAllocator>();
    // streamIndex1 += taPositionAlloc->AssignStreams(streamIndex1);
    // streamIndex2 += taPositionAlloc->AssignStreams(streamIndex2);
    // streamIndex3 += taPositionAlloc->AssignStreams(streamIndex3);
    // streamIndex4 += taPositionAlloc->AssignStreams(streamIndex4);

    // std::stringstream ssSpeed;
    // ssSpeed << "ns3::UniformRandomVariable[Min=0.0|Max=" << nodeSpeed << "]";
    // std::stringstream ssPause;
    // ssPause << "ns3::ConstantRandomVariable[Constant=" << nodePause << "]";
    // mobilityAdhoc.SetMobilityModel("ns3::RandomWaypointMobilityModel",
    //                                "Speed", StringValue(ssSpeed.str()),
    //                                "Pause", StringValue(ssPause.str()),
    //                                "PositionAllocator", PointerValue(taPositionAlloc));
    // mobilityAdhoc.SetPositionAllocator(taPositionAlloc);

    // mobilityAdhoc.Install(leftNodes);
    // mobilityAdhoc.Install(rightNodes);

    // mobilityAdhoc.Install(leftAp);
    // mobilityAdhoc.Install(rightAp);

    // streamIndex1 += mobilityAdhoc.AssignStreams(leftNodes, streamIndex1);
    // streamIndex2 += mobilityAdhoc.AssignStreams(rightNodes, streamIndex2);
    // streamIndex3 += mobilityAdhoc.AssignStreams(leftAp, streamIndex3);
    // streamIndex4 += mobilityAdhoc.AssignStreams(rightAp, streamIndex4);
    // NS_UNUSED(streamIndex1); // From this point, streamIndex is unused
    // NS_UNUSED(streamIndex2); // From this point, streamIndex is unused
    // NS_UNUSED(streamIndex3); // From this point, streamIndex is unused
    // NS_UNUSED(streamIndex4); // From this point, streamIndex is unused

    //     mobility.Install(leftNodes);
    // mobility.Install(rightNodes);

    // mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
    // mobility.Install(rightAp);
    // mobility.Install(leftAp);

    // TODO implement constantvelocityspeedmodel
    // mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
    // mobility.Install(apWifiNode);
    // mobility.Install(staWifiNodes);

    /* Internet Stack */
    InternetStackHelper stack;
    // stack.Install(p2pNodes);
    stack.Install(leftAp);
    stack.Install(leftStaNodes);
    stack.Install(rightAp);
    stack.Install(rightStaNodes);

    Ipv4AddressHelper address;
    address.SetBase("10.1.1.0", "255.255.255.0");

    Ipv4InterfaceContainer p2pInterfaces;
    p2pInterfaces = address.Assign(p2pDevices);

    address.SetBase("10.1.2.0", "255.255.255.0");

    Ipv4InterfaceContainer leftApInterface;
    leftApInterface = address.Assign(leftApDevice);
    Ipv4InterfaceContainer leftStaInterface;
    leftStaInterface = address.Assign(leftStaDevices);

    address.SetBase("10.1.3.0", "255.255.255.0");

    Ipv4InterfaceContainer rightApInterface;
    rightApInterface = address.Assign(rightApDevice);
    Ipv4InterfaceContainer rightStaInterface;
    rightStaInterface = address.Assign(rightStaDevices);

    /* Populate routing table */
    Ipv4GlobalRoutingHelper::PopulateRoutingTables();

    /* Install TCP Receiver on the access point */
    Ptr<RateErrorModel> em = CreateObject<RateErrorModel>();
    em->SetAttribute("ErrorRate", DoubleValue(0.00001));
    p2pDevices.Get(0)->SetAttribute("ReceiveErrorModel", PointerValue(em));
    // p2pDevices.Get(0)->SetAttribute("ReceiveErrorModel", PointerValue(em));
    p2pDevices.Get(1)->SetAttribute("ReceiveErrorModel", PointerValue(em));

    // after wifi netdevices are created
    Config::Set("/NodeList/1/DeviceList/0/$ns3::WifiNetDevice/Phy/$ns3::YansWifiPhy/PostReceptionErrorModel", PointerValue(em));
    Config::Set("/NodeList/2/DeviceList/0/$ns3::WifiNetDevice/Phy/$ns3::YansWifiPhy/PostReceptionErrorModel", PointerValue(em));
    Config::Set("/NodeList/3/DeviceList/0/$ns3::WifiNetDevice/Phy/$ns3::YansWifiPhy/PostReceptionErrorModel", PointerValue(em));
    for (int i = 0; i < nflows; i++)
    {
        PacketSinkHelper sinkHelper("ns3::TcpSocketFactory", InetSocketAddress(Ipv4Address::GetAny(), 9 + i));
        ApplicationContainer sinkApp = sinkHelper.Install(leftStaNodes.Get(i));
        sink = StaticCast<PacketSink>(sinkApp.Get(0));

        /* Install TCP/UDP Transmitter on the station */
        OnOffHelper server("ns3::TcpSocketFactory", (InetSocketAddress(leftStaInterface.GetAddress(i), 9 + i)));
        server.SetAttribute("PacketSize", UintegerValue(payloadSize));
        server.SetAttribute("OnTime", StringValue("ns3::ConstantRandomVariable[Constant=1]"));
        server.SetAttribute("OffTime", StringValue("ns3::ConstantRandomVariable[Constant=0]"));
        server.SetAttribute("DataRate", DataRateValue(DataRate(dataRate)));

        ApplicationContainer serverApp = server.Install(rightStaNodes.Get(i));

        /* Start Applications */
        sinkApp.Start(Seconds(0.0));
        serverApp.Start(Seconds(1.0));
    }

    // Simulator::Schedule(Seconds(1.1), &CalculateThroughput);

    /* Flow Monitor */
    FlowMonitorHelper flowmon;
    Ptr<FlowMonitor> monitor = flowmon.InstallAll();
    monitor->CheckForLostPackets();

    /* Start Simulation */
    Simulator::Stop(Seconds(simulationTime));
    Simulator::Run();

    // step 4: Add below code after Simulator::Run ();
    ///////////////////////////////////// Network Perfomance Calculation /////////////////////////////////////

    int j = 0;
    float AvgThroughput = 0;
    Time Jitter;
    Time Delay;

    Ptr<Ipv4FlowClassifier> classifier = DynamicCast<Ipv4FlowClassifier>(flowmon.GetClassifier());
    std::map<FlowId, FlowMonitor::FlowStats> stats = monitor->GetFlowStats();

    for (std::map<FlowId, FlowMonitor::FlowStats>::const_iterator iter = stats.begin(); iter != stats.end(); ++iter)
    {
        Ipv4FlowClassifier::FiveTuple t = classifier->FindFlow(iter->first);

        NS_LOG_UNCOND("----Flow ID:" << iter->first);
        NS_LOG_UNCOND("Src Addr " << t.sourceAddress << " Dst Addr " << t.destinationAddress);
        NS_LOG_UNCOND("Sent Packets=" << iter->second.txPackets);
        NS_LOG_UNCOND("Received Packets =" << iter->second.rxPackets);
        NS_LOG_UNCOND("Lost Packets =" << iter->second.txPackets - iter->second.rxPackets);
        NS_LOG_UNCOND("Packet delivery ratio =" << iter->second.rxPackets * 100 / iter->second.txPackets << "%");
        NS_LOG_UNCOND("Packet loss ratio =" << (iter->second.txPackets - iter->second.rxPackets) * 100 / iter->second.txPackets << "%");
        NS_LOG_UNCOND("Delay =" << iter->second.delaySum);
        NS_LOG_UNCOND("Jitter =" << iter->second.jitterSum);
        NS_LOG_UNCOND("Throughput =" << iter->second.rxBytes * 8.0 / (iter->second.timeLastRxPacket.GetSeconds() - iter->second.timeFirstTxPacket.GetSeconds()) / 1024 << "Kbps");

        SentPackets = SentPackets + (iter->second.txPackets);
        ReceivedPackets = ReceivedPackets + (iter->second.rxPackets);
        LostPackets = LostPackets + (iter->second.txPackets - iter->second.rxPackets);
        AvgThroughput = AvgThroughput + (iter->second.rxBytes * 8.0 / (iter->second.timeLastRxPacket.GetSeconds() - iter->second.timeFirstTxPacket.GetSeconds()) / 1024);
        Delay = Delay + (iter->second.delaySum);
        Jitter = Jitter + (iter->second.jitterSum);

        j = j + 1;
    }

    AvgThroughput = AvgThroughput / j;
    NS_LOG_UNCOND("--------Total Results of the simulation----------" << std::endl);
    NS_LOG_UNCOND("Total sent packets  =" << SentPackets);
    NS_LOG_UNCOND("Total Received Packets =" << ReceivedPackets);
    NS_LOG_UNCOND("Total Lost Packets =" << LostPackets);
    NS_LOG_UNCOND("Packet Loss ratio =" << ((LostPackets * 100) / SentPackets) << "%");
    NS_LOG_UNCOND("Packet delivery ratio =" << ((ReceivedPackets * 100) / SentPackets) << "%");
    NS_LOG_UNCOND("Average Throughput =" << AvgThroughput << "Kbps");
    NS_LOG_UNCOND("End to End Delay =" << Delay);
    NS_LOG_UNCOND("End to End Jitter delay =" << Jitter);
    NS_LOG_UNCOND("Total Flod id " << j);
    monitor->SerializeToXmlFile("wormhole.xml", true, true);

    //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

    /* Flow Monitor File  */
    // flowMonitor->SerializeToXmlFile("./highrateSimulation/flowstat.xml", false, false);

    // double averageThroughput = ((sink->GetTotalRx() * 8) / (1e6 * simulationTime));

    Simulator::Destroy();

    // if (averageThroughput < 50)
    // {
    //     NS_LOG_ERROR("Obtained throughput is not in the expected boundaries!");
    //     exit(1);
    // }
    // std::cout << "\nAverage throughput: " << averageThroughput << " Mbit/s" << std::endl;
    return 0;
}