/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2019 Universita' di Firenze, Italy
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation;
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * Author: Tommaso Pecorella <tommaso.pecorella@unifi.it>
 */

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

#include <fstream>
#include "ns3/command-line.h"

#include "ns3/config.h"
#include "ns3/string.h"
#include "ns3/log.h"

#include "ns3/core-module.h"
#include "ns3/internet-module.h"
#include "ns3/internet-apps-module.h"
#include "ns3/mobility-module.h"
#include "ns3/spectrum-module.h"
#include "ns3/propagation-module.h"
#include "ns3/sixlowpan-module.h"
#include "ns3/lr-wpan-module.h"
#include "ns3/csma-module.h"
#include "ns3/applications-module.h"
#include "ns3/ipv6-flow-classifier.h"
#include "ns3/flow-monitor-helper.h"
#include "ns3/flow-monitor.h"
#include "ns3/point-to-point-module.h"
#include "ns3/traffic-control-module.h"
#include "ns3/packet-sink.h"
#include "ns3/packet-sink-helper.h"
#include "ns3/on-off-helper.h"
#include <ns3/lr-wpan-error-model.h>

using namespace ns3;

Ptr<PacketSink> sink; /* Pointer to the packet sink application */

int main(int argc, char **argv)
{
#pragma GCC diagnostic ignored "-Wunused-variable" // "-Wstringop-overflow"
  bool verbose = false;
  /**
   * @brief variable section
  //  */
  int nflows = 5;                        /* Number of flow */
  uint32_t nWifi = 10;                   /* Number of nodes */
  uint32_t nPackets = 100;               /* Number of packets send per second */
  uint32_t payloadSize = 4096;           /* Transport layer payload size in bytes. */
  std::string dataRate = "4Mbps";        /* Application layer datarate. */
  std::string tcpVariant = "TcpNewReno"; /* TCP variant type. */
  std::string phyRate = "HtMcs7";        /* Physical layer bitrate. */
  double simulationTime = 10;            /* Simulation time in seconds. */
  uint32_t txArea = 5;
  uint32_t MaxCoverageRange = 10;

  /* this is for performance management */
  uint32_t SentPackets = 0;
  uint32_t ReceivedPackets = 0;
  uint32_t LostPackets = 0;
  /* variable declaration ends here */
  /* Calculate actual datarate here */
  dataRate = std::to_string((8 * nPackets * payloadSize) / 1024) + "Kbps";

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

  /* For changing coverage area */
  // creating a channel with range propagation loss model
  Config::SetDefault("ns3::RangePropagationLossModel::MaxRange", DoubleValue(MaxCoverageRange * txArea));
  Ptr<SingleModelSpectrumChannel> channel0 = CreateObject<SingleModelSpectrumChannel>();
  Ptr<RangePropagationLossModel> propModel0 = CreateObject<RangePropagationLossModel>();
  Ptr<ConstantSpeedPropagationDelayModel> delayModel0 = CreateObject<ConstantSpeedPropagationDelayModel>();
  channel0->AddPropagationLossModel(propModel0);
  channel0->SetPropagationDelayModel(delayModel0);
  // setting the channel in helper
  // lrWpanHelper.SetChannel(channel);
  /* coverage area code ends here */
  /* For changing coverage area */
  // creating a channel with range propagation loss model
  Config::SetDefault("ns3::RangePropagationLossModel::MaxRange", DoubleValue(MaxCoverageRange * txArea));
  Ptr<SingleModelSpectrumChannel> channel1 = CreateObject<SingleModelSpectrumChannel>();
  Ptr<RangePropagationLossModel> propModel1 = CreateObject<RangePropagationLossModel>();
  Ptr<ConstantSpeedPropagationDelayModel> delayModel1 = CreateObject<ConstantSpeedPropagationDelayModel>();
  channel1->AddPropagationLossModel(propModel1);
  channel1->SetPropagationDelayModel(delayModel1);
  // setting the channel in helper
  // lrWpanHelper.SetChannel(channel);
  /* coverage area code ends here */

  Packet::EnablePrinting();

  if (verbose)
  {
    LogComponentEnable("Ping6Application", LOG_LEVEL_ALL);
    LogComponentEnable("LrWpanMac", LOG_LEVEL_ALL);
    LogComponentEnable("LrWpanPhy", LOG_LEVEL_ALL);
    LogComponentEnable("LrWpanNetDevice", LOG_LEVEL_ALL);
    LogComponentEnable("SixLowPanNetDevice", LOG_LEVEL_ALL);
  }

  NodeContainer p2pNodes;
  p2pNodes.Create(2);

  PointToPointHelper pointToPoint;
  pointToPoint.SetDeviceAttribute("DataRate", StringValue(dataRate));
  pointToPoint.SetChannelAttribute("Delay", StringValue("10ms"));

  NetDeviceContainer p2pDevices;
  p2pDevices = pointToPoint.Install(p2pNodes);

  NodeContainer leftNodes, leftAp;
  NodeContainer rightNodes, rightAp;

  leftNodes.Add(p2pNodes.Get(0));
  rightNodes.Add(p2pNodes.Get(1));
  leftNodes.Create(nWifi);
  rightNodes.Create(nWifi);

  LrWpanHelper leftlrWpanHelper, rightlrWpanHelper;
  // Add and install the LrWpanNetDevice for each node
  NetDeviceContainer leftlrwpanDevices = leftlrWpanHelper.Install(leftNodes);
  NetDeviceContainer rightlrwpanDevices = rightlrWpanHelper.Install(rightNodes);

  // setting channel for range coverage
  // setting the channel in helper
  leftlrWpanHelper.SetChannel(channel0);
  rightlrWpanHelper.SetChannel(channel1);

  // Fake PAN association and short address assignment.
  // This is needed because the lr-wpan module does not provide (yet)
  // a full PAN association procedure.
  leftlrWpanHelper.AssociateToPan(leftlrwpanDevices, 0);
  rightlrWpanHelper.AssociateToPan(rightlrwpanDevices, 0);

  SixLowPanHelper leftsixLowPanHelper, rightsixLowPanHelper;
  NetDeviceContainer leftsixLowPanDevices = leftsixLowPanHelper.Install(leftlrwpanDevices);
  NetDeviceContainer rightsixLowPanDevices = rightsixLowPanHelper.Install(rightlrwpanDevices);

  for (uint32_t i = 0; i < leftsixLowPanDevices.GetN(); i++)
  {
    Ptr<NetDevice> dev = leftsixLowPanDevices.Get(i);
    dev->SetAttribute("UseMeshUnder", BooleanValue(true));
    dev->SetAttribute("MeshUnderRadius", UintegerValue(10));
  }
  for (uint32_t i = 0; i < rightsixLowPanDevices.GetN(); i++)
  {
    Ptr<NetDevice> dev = rightsixLowPanDevices.Get(i);
    dev->SetAttribute("UseMeshUnder", BooleanValue(true));
    dev->SetAttribute("MeshUnderRadius", UintegerValue(10));
  }

  InternetStackHelper internetv6;
  internetv6.Install(leftNodes);
  internetv6.Install(rightNodes);

  Ipv6AddressHelper ipv6;
  ipv6.SetBase(Ipv6Address("2001:cafe::"), Ipv6Prefix(64));

  Ipv6InterfaceContainer leftInterfaces;
  leftInterfaces = ipv6.Assign(leftsixLowPanDevices);
  leftInterfaces.SetForwarding(1, true);
  leftInterfaces.SetDefaultRouteInAllNodes(1);

  ipv6.SetBase(Ipv6Address("2001:f00d::"), Ipv6Prefix(64));
  Ipv6InterfaceContainer rightInterfaces;
  rightInterfaces = ipv6.Assign(rightsixLowPanDevices);
  rightInterfaces.SetForwarding(0, true);
  rightInterfaces.SetDefaultRouteInAllNodes(0);

  ipv6.SetBase(Ipv6Address("2001:baab::"), Ipv6Prefix(64));
  Ipv6InterfaceContainer p2pInterfaces;
  p2pInterfaces = ipv6.Assign(p2pDevices);
  p2pInterfaces.SetForwarding(0, true);
  p2pInterfaces.SetDefaultRouteInAllNodes(0);
  p2pInterfaces.SetForwarding(1, true);
  p2pInterfaces.SetDefaultRouteInAllNodes(1);

  // uint32_t packetSize = 10;
  // uint32_t maxPacketCount = 5;
  Time interPacketInterval = Seconds(1.);
  // Ping6Helper ping6;

  uint32_t tcp_adu_size = 160;
  Config::SetDefault("ns3::TcpSocket::SegmentSize", UintegerValue(tcp_adu_size));
  for (int i = 0; i < nflows; i++)
  {
    PacketSinkHelper sinkHelper("ns3::TcpSocketFactory", Inet6SocketAddress(Ipv6Address::GetAny(), 9 + i));
    ApplicationContainer sinkApp = sinkHelper.Install(leftNodes.Get(i));
    sink = StaticCast<PacketSink>(sinkApp.Get(0));

    /* Install TCP/UDP Transmitter on the station */
    OnOffHelper server("ns3::TcpSocketFactory", (Inet6SocketAddress(leftInterfaces.GetAddress(i, 1), 9 + i)));
    server.SetAttribute("PacketSize", UintegerValue(payloadSize));
    server.SetAttribute("OnTime", StringValue("ns3::ConstantRandomVariable[Constant=1]"));
    server.SetAttribute("OffTime", StringValue("ns3::ConstantRandomVariable[Constant=0]"));
    server.SetAttribute("DataRate", DataRateValue(DataRate(dataRate)));

    ApplicationContainer serverApp = server.Install(rightNodes.Get(i));

    /* Start Applications */
    sinkApp.Start(Seconds(0.0));
    serverApp.Start(Seconds(1.0));
  }

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

  Ptr<Ipv6FlowClassifier> classifier = DynamicCast<Ipv6FlowClassifier>(flowmon.GetClassifier());
  std::map<FlowId, FlowMonitor::FlowStats> stats = monitor->GetFlowStats();
  std::cout << "------------------------------------Stats size = " << stats.size() << std::endl;

  for (std::map<FlowId, FlowMonitor::FlowStats>::const_iterator iter = stats.begin(); iter != stats.end(); ++iter)
  {
    // Ipv6FlowClassifier::FiveTuple t = classifier->FindFlow(iter->first);

    NS_LOG_UNCOND("----Flow ID:" << iter->first);
    // NS_LOG_UNCOND("Src Addr " << t.sourceAddress << " Dst Addr " << t.destinationAddress);
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
  monitor->SerializeToXmlFile("lowrate.xml", true, true);
#pragma GCC diagnostic pop

  std::ofstream o1, o2, o3, o4;

  o1.open("task_a_flow_throughput.txt", std::ios_base::app); // append instead of overwrite
  o1 << 2 * nflows << " " << AvgThroughput << std::endl;
  o2.open("task_a_flow_eed.txt", std::ios_base::app); // append instead of overwrite
  o2 << 2 * nflows << " " << Delay.GetSeconds() - tempDelay.GetSeconds() << std::endl;
  o3.open("task_a_flow_pacdelratio.txt", std::ios_base::app); // append instead of overwrite
  o3 << 2 * nflows << " " << (((double)ReceivedPackets * 100.0) / (double)SentPackets) << std::endl;
  o4.open("task_a_flow_pacdrpratio.txt", std::ios_base::app); // append instead of overwrite
  o4 << 2 * nflows << " " << (((double)LostPackets * 100.0) / (double)SentPackets) << std::endl;

  Simulator::Destroy();
  return 0;
}
