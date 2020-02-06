#include "ns3/lte-helper.h"
#include "ns3/epc-helper.h"

#include "ns3/core-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/wifi-module.h"
#include "ns3/csma-module.h"
#include "ns3/network-module.h"
#include "ns3/applications-module.h"
#include "ns3/mobility-module.h"
#include "ns3/config-store-module.h"
#include "ns3/wimax-module.h"
#include "ns3/internet-module.h"
#include "ns3/global-route-manager.h"
#include "ns3/ipcs-classifier-record.h"
#include "ns3/service-flow.h"
#include <iostream>
#include "ns3/ipv4-global-routing-helper.h"
#include "ns3/mobility-module.h"
#include "ns3/lte-module.h"
#include "ns3/point-to-point-helper.h"
#include <iomanip>
#include <string>
#include <fstream>
#include <vector>

NS_LOG_COMPONENT_DEFINE ("WimaxSimpleExample");

using namespace ns3;



int main (int argc, char *argv[])
{
Config::SetDefault ("ns3::LteAmc::AmcModel", EnumValue (LteAmc::PiroEW2010));
  bool verbose = false;

  int duration = 500, schedType = 0;


  uint16_t numberOfUEs=2;               //Default number of ues attached to each eNodeB         

  Ptr<LteHelper> lteHelper;     //Define LTE    
  Ptr<EpcHelper>  epcHelper;    //Define EPC

  NodeContainer remoteHostContainer;            //Define the Remote Host
  NetDeviceContainer internetDevices;   //Define the Network Devices in the Connection between EPC and the remote host

  Ptr<Node> pgw;                                //Define the Packet Data Network Gateway(P-GW)  
  Ptr<Node> remoteHost;         //Define the node of remote Host

  InternetStackHelper internet;                 //Define the internet stack     
  PointToPointHelper p2ph;                              //Define Connection between EPC and the Remote Host
  Ipv4AddressHelper ipv4h;                              //Ipv4 address helper
  Ipv4StaticRoutingHelper ipv4RoutingHelper;    //Ipv4 static routing helper    
  Ptr<Ipv4StaticRouting> remoteHostStaticRouting;

  Ipv4InterfaceContainer internetIpIfaces;      //Ipv4 interfaces




  CommandLine cmd;
  cmd.AddValue ("scheduler", "type of scheduler to use with the network devices", schedType);
  cmd.AddValue ("duration", "duration of the simulation in seconds", duration);
  cmd.AddValue ("verbose", "turn on all WimaxNetDevice log components", verbose);
  cmd.Parse (argc, argv);
  LogComponentEnable ("UdpClient", LOG_LEVEL_INFO);
  LogComponentEnable ("UdpServer", LOG_LEVEL_INFO);
  //LogComponentEnable ("UdpEchoClientApplication", LOG_LEVEL_INFO);
  //LogComponentEnable ("UdpEchoServerApplication", LOG_LEVEL_INFO);

  NodeContainer ssNodes;
  NodeContainer bsNodes;

  ssNodes.Create (2);
  bsNodes.Create (1);


uint32_t nCsma = 3;

  NodeContainer p2pNodes;
  p2pNodes.Create (2);

  PointToPointHelper pointToPoint;
  pointToPoint.SetDeviceAttribute ("DataRate", StringValue ("5Mbps"));
  pointToPoint.SetChannelAttribute ("Delay", StringValue ("2ms"));

  NetDeviceContainer p2pDevices;
  p2pDevices = pointToPoint.Install (p2pNodes);

  NodeContainer csmaNodes;
  csmaNodes.Add (p2pNodes.Get (1));
  csmaNodes.Create (nCsma);

  CsmaHelper csma;
  csma.SetChannelAttribute ("DataRate", StringValue ("100Mbps"));
  csma.SetChannelAttribute ("Delay", TimeValue (NanoSeconds (6560)));

  NetDeviceContainer csmaDevices;
  csmaDevices = csma.Install (csmaNodes);

  NodeContainer wifiApNode = p2pNodes.Get (0);

  YansWifiChannelHelper channel = YansWifiChannelHelper::Default ();
  YansWifiPhyHelper phy = YansWifiPhyHelper::Default ();
  phy.SetChannel (channel.Create ());

  WifiHelper wifi ;
  //WifiHelper wifi = WifiHelper::Default ();
  
  wifi.SetRemoteStationManager ("ns3::AarfWifiManager");

  WifiMacHelper mac ;

  Ssid ssid = Ssid ("ns-3-ssid");
  mac.SetType ("ns3::StaWifiMac",
           "Ssid", SsidValue (ssid),
           "ActiveProbing", BooleanValue (false));

  NetDeviceContainer staDevices;
  staDevices = wifi.Install (phy, mac, ssNodes);

  mac.SetType ("ns3::ApWifiMac",
           "Ssid", SsidValue (ssid));

  NetDeviceContainer apDevices;
  apDevices = wifi.Install (phy, mac, wifiApNode);

  MobilityHelper mobility1;

  mobility1.SetPositionAllocator ("ns3::GridPositionAllocator",
                             "MinX", DoubleValue (0.0),
                             "MinY", DoubleValue (0.0),
                             "DeltaX", DoubleValue (5.0),
                             "DeltaY", DoubleValue (10.0),
                             "GridWidth", UintegerValue (3),
                             "LayoutType", StringValue ("RowFirst"));




  mobility1.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
  mobility1.Install (wifiApNode);
(wifiApNode.Get(0) -> GetObject<ConstantPositionMobilityModel>()) -> SetPosition(Vector(100.0, 501.0, 0.0));
  InternetStackHelper stack1;
  stack1.Install (csmaNodes);
  stack1.Install (wifiApNode);
  stack1.Install (ssNodes);

  Ipv4AddressHelper address1;

  address1.SetBase ("11.1.1.0", "255.255.255.0");
  Ipv4InterfaceContainer p2pInterfaces;
  p2pInterfaces = address1.Assign (p2pDevices);

  address1.SetBase ("11.1.2.0", "255.255.255.0");
  Ipv4InterfaceContainer csmaInterfaces;
  csmaInterfaces = address1.Assign (csmaDevices);

  address1.SetBase ("11.1.3.0", "255.255.255.0");
  address1.Assign (staDevices);
  address1.Assign (apDevices);

  UdpEchoServerHelper echoServer (9);

  ApplicationContainer serverApps1 = echoServer.Install (csmaNodes.Get (nCsma));
  serverApps1.Start (Seconds (1.0));
  serverApps1.Stop (Seconds (duration+0.1));

  UdpEchoClientHelper echoClient (csmaInterfaces.GetAddress (nCsma), 9);
  echoClient.SetAttribute ("MaxPackets", UintegerValue (1000));
  echoClient.SetAttribute ("Interval", TimeValue (Seconds (1.)));
  echoClient.SetAttribute ("PacketSize", UintegerValue (1024));

  ApplicationContainer clientApps1 = 
    echoClient.Install (ssNodes.Get (0));
  clientApps1.Start (Seconds (2.0));
  clientApps1.Stop (Seconds (duration+0.1));

  Ipv4GlobalRoutingHelper::PopulateRoutingTables ();


  //pointToPoint.EnablePcapAll ("third");
  phy.EnablePcap ("third", apDevices.Get (0));
  //csma.EnablePcap ("third", csmaDevices.Get (0), true);


    lteHelper = CreateObject<LteHelper> ();
    epcHelper = CreateObject<PointToPointEpcHelper> ();
    lteHelper->SetEpcHelper (epcHelper);
    lteHelper->SetSchedulerType("ns3::RrFfMacScheduler");
    lteHelper->SetAttribute ("PathlossModel",
                           StringValue ("ns3::FriisPropagationLossModel"));
    pgw = epcHelper->GetPgwNode ();

    remoteHostContainer.Create (1);
    remoteHost = remoteHostContainer.Get (0);
    internet.Install (remoteHostContainer);

      p2ph.SetDeviceAttribute ("DataRate", DataRateValue (DataRate ("100Gb/s")));
      p2ph.SetDeviceAttribute ("Mtu", UintegerValue (1500));
      p2ph.SetChannelAttribute ("Delay", TimeValue (Seconds (0.010))); 
      internetDevices = p2ph.Install (pgw, remoteHost);

      ipv4h.SetBase ("1.0.0.0", "255.0.0.0");
      internetIpIfaces = ipv4h.Assign (internetDevices);

      remoteHostStaticRouting = ipv4RoutingHelper.GetStaticRouting (remoteHost->GetObject<Ipv4> ());
      remoteHostStaticRouting->AddNetworkRouteTo (Ipv4Address ("7.0.0.0"), Ipv4Mask ("255.0.0.0"), 1);

      std::cout << "2. Installing LTE+EPC+remotehost. Done!" << std::endl;


MobilityHelper mobility;
  Ptr<ListPositionAllocator> positionAlloc;
  positionAlloc = CreateObject<ListPositionAllocator> ();

  positionAlloc->Add (Vector (0.0, 500.0, 0.0)); //STA

  mobility.SetPositionAllocator (positionAlloc);
  mobility.SetMobilityModel ("ns3::ConstantVelocityMobilityModel");  
  mobility.Install(ssNodes.Get(0));

  Ptr<ConstantVelocityMobilityModel> cvm = ssNodes.Get(0)->GetObject<ConstantVelocityMobilityModel>();
  cvm->SetVelocity(Vector (5, 0, 0)); //move to left to right 10.0m/s

  positionAlloc = CreateObject<ListPositionAllocator> ();

  positionAlloc->Add (Vector (0.0, 500.0, 10.0)); //MAG1AP
  positionAlloc->Add (Vector (0.0, 510.0, 0.0));  //MAG2AP

  mobility.SetPositionAllocator (positionAlloc);
  mobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");

  mobility.Install (NodeContainer(bsNodes.Get(0),ssNodes.Get(1)));





NetDeviceContainer ssDevs, bsDevs;

            bsDevs = lteHelper->InstallEnbDevice (bsNodes);
            ssDevs=lteHelper->InstallUeDevice (ssNodes);

            for (uint16_t j=0; j < numberOfUEs; j++)
            {
                    lteHelper->Attach (ssDevs.Get(j), bsDevs.Get(0));  
            }


            Ipv4InterfaceContainer iueIpIface;
            iueIpIface = epcHelper->AssignUeIpv4Address (NetDeviceContainer (ssDevs));

  lteHelper->ActivateDedicatedEpsBearer (ssDevs, EpsBearer (EpsBearer::NGBR_VIDEO_TCP_DEFAULT), EpcTft::Default ());

UdpServerHelper udpServer;
  ApplicationContainer serverApps;
  UdpClientHelper udpClient;
  ApplicationContainer clientApps;

  udpServer = UdpServerHelper (100);

  serverApps = udpServer.Install (ssNodes.Get (0));
  serverApps.Start (Seconds (6));
  serverApps.Stop (Seconds (duration));

  udpClient = UdpClientHelper (iueIpIface.GetAddress (0), 100);
  udpClient.SetAttribute ("MaxPackets", UintegerValue (200000));
  udpClient.SetAttribute ("Interval", TimeValue (Seconds (0.004)));
  udpClient.SetAttribute ("PacketSize", UintegerValue (1024));

  clientApps = udpClient.Install (remoteHost);
  clientApps.Start (Seconds (6));
  clientApps.Stop (Seconds (duration));
    lteHelper->EnableTraces ();


  NS_LOG_INFO ("Starting simulation.....");
  Simulator::Stop(Seconds(duration));

  Simulator::Run ();


  Simulator::Destroy ();
  NS_LOG_INFO ("Done.");

  return 0;
}