/* -*-  Mode: C++; c-file-style: "gnu"; indent-tabs-mode:nil; -*- */
 /*
  * Copyright (c) 2011-2018 Centre Tecnologic de Telecomunicacions de Catalunya (CTTC)
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
  * Authors: Jaume Nin <jaume.nin@cttc.cat>
  *          Manuel Requena <manuel.requena@cttc.es>
  */
 
#include "ns3/core-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/internet-module.h"
#include "ns3/applications-module.h"
#include "ns3/mobility-module.h"
#include "ns3/config-store-module.h"
#include "ns3/lte-module.h"
#include "ns3/wifi-module.h"
#include "ns3/wifi-80211p-helper.h"
#include "ns3/wave-mac-helper.h"
 //#include "ns3/gtk-config-store.h"
 
using namespace ns3;
 
NS_LOG_COMPONENT_DEFINE ("LenaSimpleEpc");
 
int
main (int argc, char *argv[])
{
  uint16_t numNodePairs = 1;
  Time simTime = MilliSeconds (1100);
  double distance = 60.0;
  Time interPacketInterval = MilliSeconds (100);
  bool useCa = false;
  bool disableDl = false;
  bool disableUl = false;
  bool disablePl = false;
 
   // Command line arguments
  CommandLine cmd;
  cmd.AddValue ("numNodePairs", "Number of eNodeBs + UE pairs", numNodePairs);
  cmd.AddValue ("simTime", "Total duration of the simulation", simTime);
  cmd.AddValue ("distance", "Distance between eNBs [m]", distance);
  cmd.AddValue ("interPacketInterval", "Inter packet interval", interPacketInterval);
  cmd.AddValue ("useCa", "Whether to use carrier aggregation.", useCa);
  cmd.AddValue ("disableDl", "Disable downlink data flows", disableDl);
  cmd.AddValue ("disableUl", "Disable uplink data flows", disableUl);
  cmd.AddValue ("disablePl", "Disable data flows between peer UEs", disablePl);
  cmd.Parse (argc, argv);
 
  ConfigStore inputConfig;
  inputConfig.ConfigureDefaults ();
 
   // parse again so you can override default values from the command line
  cmd.Parse(argc, argv);
 
  if (useCa)
  {
    Config::SetDefault ("ns3::LteHelper::UseCa", BooleanValue (useCa));
    Config::SetDefault ("ns3::LteHelper::NumberOfComponentCarriers", UintegerValue (2));
    Config::SetDefault ("ns3::LteHelper::EnbComponentCarrierManager", StringValue ("ns3::RrComponentCarrierManager"));
  }

  /**************************************************/
  /*                                                */
  /*   LTE : between ue and eNb                     */
  /*   EPC : between eNb and pgw                    */
  /*                                                */
  /*         LTE          EPC                       */
  /*    UE----------eNb-----------pgw               */
  /*                                                */
  /**************************************************/
 
  Ptr<LteHelper> lteHelper = CreateObject<LteHelper> ();
  Ptr<PointToPointEpcHelper> epcHelper = CreateObject<PointToPointEpcHelper> ();
  lteHelper->SetEpcHelper (epcHelper);
  Ptr<Node> pgw = epcHelper->GetPgwNode ();

  NodeContainer ueNodes;
  NodeContainer enbNodes;
  enbNodes.Create (1);
  ueNodes.Create (1);

  /**************************************************/
  /*                                                */
  /*   P2P : between pgw and remotehost             */
  /*          internet                              */
  /*        -datarate :100Gb/s                      */
  /*        -Mtu :1500                              */
  /*        -Delay :10 ms                           */
  /*                                                */
  /*           p2p                                  */
  /*    pgw-----------remoteHost                    */
  /*                                                */
  /**************************************************/

 
  // Create a single RemoteHost
  NodeContainer remoteHostContainer;
  remoteHostContainer.Create (1);
  Ptr<Node> remoteHost = remoteHostContainer.Get (0);
 
  // Create the Internet
  InternetStackHelper internet;
  internet.Install (remoteHostContainer);

  PointToPointHelper p2ph;
  p2ph.SetDeviceAttribute ("DataRate", DataRateValue (DataRate ("100Gb/s")));
  p2ph.SetDeviceAttribute ("Mtu", UintegerValue (1500));
  p2ph.SetChannelAttribute ("Delay", TimeValue (MilliSeconds (10)));
  NetDeviceContainer internetDevices = p2ph.Install (pgw, remoteHost);

  Ipv4AddressHelper ipv4h;
  ipv4h.SetBase ("1.0.0.0", "255.0.0.0");
  Ipv4InterfaceContainer internetIpIfaces = ipv4h.Assign (internetDevices);

  // interface 0 is localhost, 1 is the p2p device
  Ipv4Address remoteHostAddr = internetIpIfaces.GetAddress (1);
  Ipv4StaticRoutingHelper ipv4RoutingHelper;
  Ptr<Ipv4StaticRouting> remoteHostStaticRouting = ipv4RoutingHelper.GetStaticRouting (remoteHost->GetObject<Ipv4> ());
  remoteHostStaticRouting->AddNetworkRouteTo (Ipv4Address ("7.0.0.0"), Ipv4Mask ("255.0.0.0"), 1);
  


  /**************************************************/
  /*                                                */
  /*   Wifi :802.11p installation                   */
  /*                                                */
  /*                                                */
  /*        80211p       80211p                     */
  /*   ue-------------ss-----------remoteHost       */
  /*                                                */
  /**************************************************/



  double freq = 5.9e9;

  NodeContainer ssNodes;
  ssNodes.Create (1);
  YansWifiChannelHelper channel = YansWifiChannelHelper::Default ();
  channel.SetPropagationDelay ("ns3::ConstantSpeedPropagationDelayModel");
  std::string m_lossModelName = "ns3::FriisPropagationLossModel";
  channel.AddPropagationLoss (m_lossModelName, "Frequency", DoubleValue (freq));
  YansWifiPhyHelper phy = YansWifiPhyHelper::Default ();
  phy.SetChannel (channel.Create ());
  phy.SetPcapDataLinkType (WifiPhyHelper::DLT_IEEE802_11);

/*******************Install devices *******************/
std::string m_phyMode = "OfdmRate6MbpsBW10MHz" ;
Wifi80211pHelper wifi80211p = Wifi80211pHelper::Default ();
wifi80211p.SetRemoteStationManager ("ns3::ConstantRateWifiManager",
                                       "DataMode",StringValue (m_phyMode),
                                       "ControlMode",StringValue (m_phyMode));
  /*******************************************/
  NqosWaveMacHelper mac = NqosWaveMacHelper::Default ();
  /* device for wifi alone */
  NetDeviceContainer apDevices;
  //apDevices = wifi.Install (phy, mac, ssNodes);
 
  apDevices = wifi80211p.Install (phy, mac, ssNodes);
  apDevices = wifi80211p.Install (phy, mac, remoteHost);
  apDevices = wifi80211p.Install (phy, mac, ueNodes);





  /**************************************************/
  /*                                                */
  /*        Mobility Installation                   */
  /*                                                */
  /**************************************************/
   

   /* position*/
  MobilityHelper mobility;
  mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
  mobility.Install(enbNodes);
  (enbNodes.Get(0) -> GetObject<ConstantPositionMobilityModel>()) -> SetPosition(Vector(250, 500.0, 0.0));
   mobility.Install(ueNodes);
  (ueNodes.Get(0) -> GetObject<ConstantPositionMobilityModel>()) -> SetPosition(Vector(0, 250, 0.0));
  mobility.Install(remoteHostContainer);
  (remoteHostContainer.Get(0) -> GetObject<ConstantPositionMobilityModel>()) -> SetPosition(Vector(500, 250, 0.0));


  mobility.Install(ssNodes);
  (ssNodes.Get(0) -> GetObject<ConstantPositionMobilityModel>()) -> SetPosition(Vector(250, 0, 0.0));

  // Install LTE Devices to the nodes
  NetDeviceContainer enbLteDevs = lteHelper->InstallEnbDevice (enbNodes);
  NetDeviceContainer ueLteDevs = lteHelper->InstallUeDevice (ueNodes);
 
  // Install the IP stack on the UEs
  internet.Install (ueNodes);
  Ipv4InterfaceContainer ueIpIface;
  ueIpIface = epcHelper->AssignUeIpv4Address (NetDeviceContainer (ueLteDevs));
  // Assign IP address to UEs, and install applications

  Ptr<Node> ueNode = ueNodes.Get (0);
  // Set the default gateway for the UE
  Ptr<Ipv4StaticRouting> ueStaticRouting = ipv4RoutingHelper.GetStaticRouting (ueNode->GetObject<Ipv4> ());
  ueStaticRouting->SetDefaultRoute (epcHelper->GetUeDefaultGatewayAddress (), 1);
  
  // Attach one UE per eNodeB
  lteHelper->Attach (ueLteDevs.Get(0), enbLteDevs.Get(0));


  /****************************************************************************/
  /*                                                                          */
  /*                             Application client serveur                   */
  /*                                       with different ports                         */
  /*                                                                          */
  /*                                                                          */   
  /****************************************************************************/


  // Install and start applications on UEs and remote host
  uint16_t dlPort = 1100;
  uint16_t ulPort = 2000;

  ApplicationContainer clientApps;
  ApplicationContainer serverApps;

  if (!disableDl)
  {
    PacketSinkHelper dlPacketSinkHelper ("ns3::UdpSocketFactory", InetSocketAddress (Ipv4Address::GetAny (), dlPort));
    serverApps.Add (dlPacketSinkHelper.Install (ueNodes.Get (0)));
    UdpClientHelper dlClient (ueIpIface.GetAddress (0), dlPort);
    dlClient.SetAttribute ("Interval", TimeValue (interPacketInterval));
    dlClient.SetAttribute ("MaxPackets", UintegerValue (1000000));
    clientApps.Add (dlClient.Install (remoteHost));
  }
 
  if (!disableUl)
  {
    ++ulPort;
    PacketSinkHelper ulPacketSinkHelper ("ns3::UdpSocketFactory", InetSocketAddress (Ipv4Address::GetAny (), ulPort));
    serverApps.Add (ulPacketSinkHelper.Install (remoteHost));
 
    UdpClientHelper ulClient (remoteHostAddr, ulPort);
    ulClient.SetAttribute ("Interval", TimeValue (interPacketInterval));
    ulClient.SetAttribute ("MaxPackets", UintegerValue (1000000));
    clientApps.Add (ulClient.Install (ueNodes.Get(0)));
  }
     
  /****************************************************************************/
  /*                                                                          */
  /*                             Application                                  */
  /*                                                                          */
  /*                                                                          */   
  /****************************************************************************/



   serverApps.Start (MilliSeconds (500));
   clientApps.Start (MilliSeconds (500));
   lteHelper->EnableTraces ();
 
   Simulator::Stop (simTime);
   Simulator::Run ();
 


   Simulator::Destroy ();
   return 0;
 }

