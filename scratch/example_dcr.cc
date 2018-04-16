/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
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
 */

#include <fstream>
#include "ns3/core-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/network-module.h"
#include "ns3/applications-module.h"
#include "ns3/yans-wifi-phy.h"
#include "ns3/wifi-module.h"
#include "ns3/mobility-module.h"
#include "ns3/csma-module.h"
#include "ns3/internet-module.h"
#include "ns3/wifi-net-device.h"
#include "ns3/wifi-phy.h"
#include "ns3/packet.h"
#include "ns3/flow-monitor-module.h"
#include "ns3/flow-classifier.h"

// Default Network Topology
//
//   Wifi 10.1.3.0
//       AP
//  *    *
//  |    |    10.1.1.0
// n2   n0 -------------- n1
//         point-to-point

using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("SACK Evaluation");

uint64_t dropped = 0;
//uint64_t dropped1 = 0;


void RxDrop (Ptr<const Packet> p)
{
  dropped++;
}

/*void RxDrop1 (Ptr<const Packet> p)
{
  dropped1++;
}*/

int 
main (int argc, char *argv[])
{
  uint32_t nWifi = 1;
  bool tracing = false;
  bool dcr = true;
  double edt = 35.0;
  std::string tcpvariant = "TcpNewReno";
  std::string raa = "Aarf";

  CommandLine cmd;
  //cmd.AddValue ("nCsma", "Number of \"extra\" CSMA nodes/devices", nCsma);
  cmd.AddValue ("nWifi", "Number of wifi STA devices", nWifi);
  cmd.AddValue ("tracing", "Enable pcap tracing", tracing);
  cmd.AddValue ("DCR", "Enable DCR", dcr);
  cmd.AddValue ("edt", "ns3::YansWifiPhy::EnergyDetectionThreshold energy of a received signal", edt);
  cmd.AddValue ("tcp", "Transport protocol to use: TcpNewReno, "
                "TcpHybla, TcpHighSpeed, TcpHtcp, TcpVegas, TcpScalable, TcpVeno, "
                "TcpBic, TcpYeah, TcpIllinois, TcpWestwood, TcpWestwoodPlus, TcpLedbat ", tcpvariant);
  cmd.AddValue ("wifiManager", "Set wifi rate manager (Aarf, Aarfcd, Amrr, Arf, Cara, Ideal, Minstrel, MinstrelHt, Onoe, Rraa)", raa);

  cmd.Parse (argc,argv);

  // The underlying restriction of 18 is due to the grid position
  // allocator's configuration; the grid layout will exceed the
  // bounding box if more than 18 nodes are provided.

  NodeContainer p2pNodes;
  p2pNodes.Create (2);

  PointToPointHelper pointToPoint;
  pointToPoint.SetDeviceAttribute ("DataRate", StringValue ("10Mbps"));
  pointToPoint.SetChannelAttribute ("Delay", StringValue ("2ms"));

  NetDeviceContainer p2pDevices;
  p2pDevices = pointToPoint.Install (p2pNodes);

  NodeContainer wifiStaNodes;
  wifiStaNodes.Create (nWifi);
  NodeContainer wifiApNode = p2pNodes.Get (0);
  
  std::string tcpstring = "ns3::"+tcpvariant;
  
  if (tcpstring.compare ("ns3::TcpWestwoodPlus") == 0)
    { 
      // TcpWestwoodPlus is not an actual TypeId name; we need TcpWestwood here
      Config::SetDefault ("ns3::TcpL4Protocol::SocketType", TypeIdValue (TcpWestwood::GetTypeId ()));
      // the default protocol type in ns3::TcpWestwood is WESTWOOD
      Config::SetDefault ("ns3::TcpWestwood::ProtocolType", EnumValue (TcpWestwood::WESTWOODPLUS));
    }
  else
    {
      TypeId tcpTid;
      NS_ABORT_MSG_UNLESS (TypeId::LookupByNameFailSafe (tcpstring, &tcpTid), "TypeId " << tcpstring << " not found");
      Config::SetDefault ("ns3::TcpL4Protocol::SocketType", TypeIdValue (TypeId::LookupByName (tcpstring)));
    }

  YansWifiChannelHelper channel_h = YansWifiChannelHelper::Default ();
  channel_h.SetPropagationDelay ("ns3::ConstantSpeedPropagationDelayModel");
  channel_h.AddPropagationLoss ("ns3::JakesPropagationLossModel");

  Ptr<YansWifiChannel> channel = channel_h.Create();

  Config::SetDefault ("ns3::JakesProcess::DopplerFrequencyHz", DoubleValue (477.9));
  
  YansWifiPhyHelper phyAP = YansWifiPhyHelper::Default ();
  YansWifiPhyHelper phyST = YansWifiPhyHelper::Default ();

  phyAP.Set ("TxGain", DoubleValue (55));
  phyAP.Set ("RxGain", DoubleValue (90));
  phyST.Set("RxGain", DoubleValue (90));
  phyAP.SetChannel (channel);
  phyST.SetChannel (channel);
  
  phyAP.Set ("TxPowerStart", DoubleValue (20.0));
  phyAP.Set ("TxPowerEnd", DoubleValue (20.0));
  phyAP.Set ("TxPowerLevels", UintegerValue (1));
  phyST.Set ("RxNoiseFigure", DoubleValue(30.0));
  phyST.Set ("CcaMode1Threshold", DoubleValue (30.0));
  phyST.Set ("EnergyDetectionThreshold", DoubleValue (edt));
  phyAP.SetErrorRateModel ("ns3::YansErrorRateModel");
  phyST.SetErrorRateModel ("ns3::YansErrorRateModel");


  WifiHelper wifi;
  wifi.SetStandard (WIFI_PHY_STANDARD_80211b);
  //std::string remotestman = "ns3::"+raa;
  wifi.SetRemoteStationManager ("ns3::" + raa + "WifiManager");

  WifiMacHelper mac;
  Ssid ssid = Ssid ("ns-3-ssid");
  mac.SetType ("ns3::StaWifiMac",
               "Ssid", SsidValue (ssid),
               "ActiveProbing", BooleanValue (true));

  NetDeviceContainer staDevices;
  staDevices = wifi.Install (phyST, mac, wifiStaNodes);


  mac.SetType ("ns3::ApWifiMac",
               "Ssid", SsidValue (ssid));

  NetDeviceContainer apDevices;
  
  apDevices = wifi.Install (phyAP, mac, wifiApNode);

  Ptr<WifiNetDevice> wnd = staDevices.Get (0)->GetObject<WifiNetDevice> ();
  Ptr<WifiPhy> wp = wnd->GetPhy ();
  Ptr<YansWifiPhy> phySta = wp->GetObject<YansWifiPhy> ();

  //Ptr<WifiNetDevice> wnd1 = apDevices.Get (0)->GetObject<WifiNetDevice> ();
  //Ptr<WifiPhy> wp1 = wnd1->GetPhy ();
  //Ptr<YansWifiPhy> phySta1 = wp1->GetObject<YansWifiPhy> ();

  phySta->TraceConnectWithoutContext ("PhyRxDrop", MakeCallback(&RxDrop));

  //phySta1->TraceConnectWithoutContext ("PhyRxDrop", MakeCallback(&RxDrop1));


  MobilityHelper mobility;

  mobility.SetPositionAllocator ("ns3::GridPositionAllocator",
                                 "MinX", DoubleValue (0.0),
                                 "MinY", DoubleValue (0.0),
                                 "DeltaX", DoubleValue (100.0),
                                 "DeltaY", DoubleValue (10.0),
                                 "GridWidth", UintegerValue (3),
                                 "LayoutType", StringValue ("RowFirst"));

  mobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
  mobility.Install (wifiStaNodes);

  mobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
  mobility.Install (wifiApNode);
  
  mobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
  mobility.Install (p2pNodes.Get (1));

  InternetStackHelper stack;
  stack.Install (p2pNodes.Get (1));
  stack.Install (wifiApNode);
  stack.Install (wifiStaNodes);

  Ipv4AddressHelper address;

  address.SetBase ("10.1.1.0", "255.255.255.0");
  Ipv4InterfaceContainer p2pInterfaces, stInterface;
  p2pInterfaces = address.Assign (p2pDevices);

  address.SetBase ("10.1.3.0", "255.255.255.0");
  stInterface = address.Assign (staDevices);
  address.Assign (apDevices);
  Config::SetDefault ("ns3::TcpSocketBase::DCR", BooleanValue (dcr));
 

  uint16_t sinkPort = 8080;
  Address sinkAddress (InetSocketAddress (stInterface.GetAddress (0),sinkPort));
  PacketSinkHelper sink ("ns3::TcpSocketFactory",
                    InetSocketAddress (Ipv4Address::GetAny (), sinkPort));
  ApplicationContainer sinkApps = sink.Install (wifiStaNodes);
  sinkApps.Start (Seconds (0.0));
  sinkApps.Stop (Seconds (10.0));

  OnOffHelper onOffHelper ("ns3::TcpSocketFactory", sinkAddress);
  onOffHelper.SetAttribute ("OnTime", StringValue("ns3::ConstantRandomVariable[Constant=1]"));
  onOffHelper.SetAttribute ("OffTime", StringValue("ns3::ConstantRandomVariable[Constant=0]"));
  onOffHelper.SetAttribute ("DataRate",StringValue ("10Mbps"));
  onOffHelper.SetAttribute ("PacketSize", UintegerValue (1780));

  ApplicationContainer source;

  source.Add (onOffHelper.Install (p2pNodes.Get (1)));
  source.Start (Seconds (1.0));
  source.Stop (Seconds (10.0));

  Ipv4GlobalRoutingHelper::PopulateRoutingTables ();

  phyAP.SetPcapDataLinkType (YansWifiPhyHelper::DLT_IEEE802_11_RADIO);

  FlowMonitorHelper flowmon;
  Ptr<FlowMonitor> monitor = flowmon.InstallAll();

  Ptr<Ipv4> ipv4 = wifiStaNodes.Get (0)->GetObject<Ipv4>();
  Ipv4InterfaceAddress iaddr = ipv4->GetAddress (1,0);
  Ipv4Address addri = iaddr.GetLocal ();

  std::cout<<"Running simulation for:\n";
  std::cout<<"Energy Detection Threshold:" << edt <<"dBm\n";
  std::cout<<"DCR is " << ((dcr == true)?"enabled":"disabled") << "\n";
  std::cout<<"TCP variant is " << tcpvariant << "\n";
  std::cout<<"Remote Station Manager is " << raa << "\n";

  Simulator::Stop (Seconds (10.0));
  Simulator::Run ();

  monitor->CheckForLostPackets();
  Ptr<Ipv4FlowClassifier> classifier = DynamicCast<Ipv4FlowClassifier> (flowmon.GetClassifier ());
  std::map<FlowId, FlowMonitor::FlowStats> stats = monitor->GetFlowStats();
  for(std::map<FlowId, FlowMonitor::FlowStats>::const_iterator i = stats.begin(); i!= stats.end();i++)
  {
          Ipv4FlowClassifier::FiveTuple t = classifier->FindFlow(i->first);
          std::cout<<"Flow"<<i->first <<"("<<t.sourceAddress<<"->"<<t.destinationAddress <<")\n";
          std::cout<<" Tx Bytes: "<<i->second.txBytes<<"\n";
          std::cout<<" Rx Bytes: "<<i->second.rxBytes<<"\n";
          std::cout << "  Tx Packets: " << i->second.txPackets << "\n";
          std::cout << "  Rx Packets: " << i->second.rxPackets << "\n";
          //std::cout << "  lostPackets: " << i->second.lostPackets << "\n";
          if(addri == t.destinationAddress)
                std::cout << "  DroppedPackets: " << (dropped) << "\n";
          Time ReceptionTime = ((i->second.timeLastRxPacket)-(i->second.timeFirstRxPacket));
          double sec = ReceptionTime.GetSeconds();
          std::cout << "  Throughput: " << i->second.rxBytes * 8.0 / sec / 1000 / 1000  << " Mbps\n";
  }
 
  //std::cout << "  DroppedPackets1: " << (dropped1) << "\n";

  std::cout << "\n\n";

  if (tracing == true)
    {
      pointToPoint.EnablePcapAll ("eeosack");
      phyST.EnablePcap ("Station", staDevices.Get (0));
      phyAP.EnablePcap ("AccessPoint", apDevices);
    }

  Simulator::Destroy ();
  return 0;
}
