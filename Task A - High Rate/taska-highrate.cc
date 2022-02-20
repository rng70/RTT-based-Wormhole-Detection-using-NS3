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
#include "ns3/csma-module.h"
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

NS_LOG_COMPONENT_DEFINE("TaskAHighRate");

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
    uint32_t nWifi = 10;
    int no_of_flow = 10;
    uint32_t packets = 2;

    uint32_t payloadSize = 1024;           /* Transport layer payload size in bytes. */
    std::string dataRate = "100Mbps";      /* Application layer datarate. */
    std::string tcpVariant = "TcpNewReno"; /* TCP variant type. */
    std::string phyRate = "HtMcs7";        /* Physical layer bitrate. */
    double simulationTime = 2;             /* Simulation time in seconds. */

    CommandLine cmd(__FILE__);
    cmd.AddValue("nWifi", "Number of wifi STA devices", nWifi);
    cmd.AddValue("no_of_flow", "Number of flow", no_of_flow);
    cmd.AddValue("payloadSize", "Payload size in bytes", payloadSize);
    cmd.AddValue("dataRate", "Application data ate", dataRate);
    cmd.AddValue("tcpVariant", "Transport protocol to use: TcpNewReno, "
                               "TcpHybla, TcpHighSpeed, TcpHtcp, TcpVegas, TcpScalable, TcpVeno, "
                               "TcpBic, TcpYeah, TcpIllinois, TcpWestwood, TcpWestwoodPlus, TcpLedbat ",
                 tcpVariant);
    cmd.AddValue("phyRate", "Physical layer bitrate", phyRate);
    cmd.AddValue("numPackets", "Total number of packets", packets);
    cmd.AddValue("simulationTime", "Simulation time in seconds", simulationTime);
    cmd.Parse(argc, argv);

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

    NodeContainer p2pNodes;
    p2pNodes.Create(2);

    PointToPointHelper pointToPoint;
    pointToPoint.SetDeviceAttribute("DataRate", StringValue("100Mbps"));
    pointToPoint.SetChannelAttribute("Delay", StringValue("10ms"));
    // pointToPoint.SetQueue("ns3::DropTailQueue"); //TODO delete

    NetDeviceContainer p2pDevices;
    p2pDevices = pointToPoint.Install(p2pNodes);

    /**
     * @brief from here
     */
    NodeContainer leftNodes, rightNodes;
    leftNodes.Create(nWifi / 2);
    rightNodes.Create(nWifi / 2);

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
    leftStaDevices = leftWifiHelper.Install(leftWifiPhy, leftWifiMac, leftNodes);
    githStaDevices = rightWifiHelper.Install(rightWifiPhy, rightWifiMac, rightNodes);

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
                                  "DeltaX", DoubleValue(5.0),
                                  "DeltaY", DoubleValue(10.0),
                                  "GridWidth", UintegerValue(3),
                                  "LayoutType", StringValue("RowFirst"));

    mobility.SetMobilityModel("ns3::RandomWalk2dMobilityModel",
                              "Bounds", RectangleValue(Rectangle(-50, 50, -50, 50)));
    mobility.Install(leftNodes);
    mobility.Install(rightNodes);

    mobility.Install(leftAp);
    mobility.Install(rightAp);

    // TODO implement constantvelocityspeedmodel
    // mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
    // mobility.Install(apWifiNode);
    // mobility.Install(staWifiNodes);

    /* Internet Stack */
    InternetStackHelper stack;
    // stack.Install(p2pNodes);
    stack.Install(csmaNodes); // TODO check
    stack.Install(leftAp);
    stack.Install(leftNodes);
    stack.Install(rightAp);
    stack.Install(rightNodes);

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
    for (int i = 0; i < no_of_flow; i++)
    {
        PacketSinkHelper sinkHelper("ns3::TcpSocketFactory", InetSocketAddress(Ipv4Address::GetAny(), 9 + i));
        ApplicationContainer sinkApp = sinkHelper.Install(leftNodes.Get(i));
        sink = StaticCast<PacketSink>(sinkApp.Get(0));

        /* Install TCP/UDP Transmitter on the station */
        OnOffHelper server("ns3::TcpSocketFactory", (InetSocketAddress(staInterface.GetAddress(i), 9 + i)));
        server.SetAttribute("PacketSize", UintegerValue(payloadSize));
        server.SetAttribute("OnTime", StringValue("ns3::ConstantRandomVariable[Constant=1]"));
        server.SetAttribute("OffTime", StringValue("ns3::ConstantRandomVariable[Constant=0]"));
        server.SetAttribute("DataRate", DataRateValue(DataRate(dataRate)));

        /* Start Applications */
        sinkApp.Start(Seconds(0.0));
        serverApp.Start(Seconds(1.0));
    }

    Simulator::Schedule(Seconds(1.1), &CalculateThroughput);

    /* Flow Monitor */
    Ptr<FlowMonitor> flowMonitor;
    FlowMonitorHelper flowHelper;
    flowMonitor = flowHelper.InstallAll();

    flowMonitor->CheckForLostPackets();
    // Ptr<Ipv4FlowClassifier> classifier = DynamicCast<Ipv4FlowClassifier>(flowHelper.GetClassifier());
    // std::map<FlowId, FlowMonitor::FlowStats> stats = flowMonitor->GetFlowStats();

    /* Start Simulation */
    Simulator::Stop(Seconds(simulationTime + 1));
    Simulator::Run();

    /* Flow Monitor File  */
    flowMonitor->SerializeToXmlFile("./highrateSimulation/flowstat.xml", false, false);

    double averageThroughput = ((sink->GetTotalRx() * 8) / (1e6 * simulationTime));

    Simulator::Destroy();

    if (averageThroughput < 50)
    {
        NS_LOG_ERROR("Obtained throughput is not in the expected boundaries!");
        exit(1);
    }
    std::cout << "\nAverage throughput: " << averageThroughput << " Mbit/s" << std::endl;
    return 0;
}