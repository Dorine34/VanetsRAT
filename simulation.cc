/* -*-  Mode: C++; c-file-style: "gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2009 The Boeing Company
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
 */

#include "ns3/command-line.h"
#include "ns3/config.h"
#include "ns3/double.h"
#include "ns3/string.h"
#include "ns3/log.h"
#include "ns3/yans-wifi-helper.h"
#include "ns3/mobility-helper.h"
#include "ns3/ipv4-address-helper.h"
#include "ns3/ipv6-address-helper.h"
#include "ns3/yans-wifi-channel.h"
#include "ns3/mobility-model.h"
#include "ns3/internet-stack-helper.h"
#include "ns3/tag.h"
#include "ns3/packet.h"
#include "ns3/uinteger.h"
#include "ns3/simple-tag.h"
#include "ns3/wifi-80211p-helper.h"
#include "ns3/wave-mac-helper.h"
#include "ns3/wave-helper.h"
#include "ns3/yans-wifi-helper.h"

// for NS2 files
#include "ns3/ns2-mobility-helper.h"
#include "ns3/ocb-wifi-mac.h"
#include "ns3/wifi-80211p-helper.h"
#include "ns3/wave-mac-helper.h"

// NetAnim
#include "ns3/netanim-module.h"

// C file for mkdir
#include <sys/stat.h>
#include <sys/types.h>

using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("PTGLsimulator");

/** Physical mode for 802.11p */
#define DEFAULT_PHYSICALMODE "OfdmRate6MbpsBW10MHz"

// ************************************************************************************** Configuration parameters

/** Scenario file name with the absolute directory if needed */
std::string m_scenarioFileName = "scenario/eloignement.tcl" ;

/** Scenario file name with the absolute directory */
std::string m_trafficFileName = "scenario/trafic2noeuds.txt" ;

/** Clear to send and request to send parameter (default = 2200) */
int m_valRtsCtsThreshold = 2200 ;
  
/** Write per-device PCAP traces if true */
bool m_pcap = false ;
  
/** In cas of speed, calculate the LineOfSightDoppler */
double m_lineOfSightDoppler ;

/** Output directory */
std::string m_outputdir = "output" ;
uint32_t m_testNumber = 0 ;

/** Packet size */
#define PACKET_SIZE_DEFAULT 256
uint32_t m_packetSize = PACKET_SIZE_DEFAULT ;

/** Number of nodes */
uint32_t m_nNodes = 3 ;
uint32_t m_nNodes80211p = 3  ;
uint32_t m_nNodeswave = 0 ;
uint32_t m_nNodes80211b = 0 ;

/** Packet time interval */
double m_packetInterval = 1 ;

/** Messages sent */
/** Start of sending packet */
int m_startSendTime = 0 ;

/** Stop of sending packet and end of simulation */
int m_stopSendTime = 180 ;

/** Propagation model
 */
int m_lossModel = 1 ;

// ************************************************************************************** End of configuration parameters

/* Configuration string */
std::stringstream m_strPropagationLossConfig ;

/** */
double m_heightAboveZ = 1.5;


/** In case of Grid => define the speed */
double m_speed = 13.33;

// ---------------- To read scenario files
Ns2MobilityHelper *m_ns2Helper ;
  
// ---------------- name network
/**
 */

NodeContainer m_80211pContainer ;//1
NodeContainer m_waveContainer ;//3
NodeContainer m_80211bContainer ;//2

/**
 */
NetDeviceContainer m_netDeviceContainer80211p;
NetDeviceContainer m_netDeviceContainer80211b;
NetDeviceContainer m_netDeviceContainerwave;
  

/** Number of nodes which send a message
 */
uint32_t m_nNodesEmitter ;

/**
 */
Ipv4InterfaceContainer m_ipv4InterfaceContainer;
Ipv6InterfaceContainer m_ipv6InterfaceContainer;

std::string m_lossModelName ;
int m_80211mode = 1 ; // 802.11p
int m_fading = 0 ;
int m_verbose = 0 ;
std::string m_phyMode = "OfdmRate6MbpsBW10MHz" ;
int m_txPower = 0 ;
int m_asciiTrace = 1 ;
std::string m_trName = "trFile" ;

/* -------------------------------------------------------------------------------------- */
/*                                                                                        */
/*                          Class to build nodes link                                     */
/*                                                                                        */
/* -------------------------------------------------------------------------------------- */
/**
 * Class to store information about links between nodes with emitter / receiver / packet size
 * and messages sent and received
 */
class EmitterReceiver {

public:

  EmitterReceiver(int emitter, int receiver) {

    this->emitter = emitter ;
    this->receiver = receiver ;
    this->packetSize = PACKET_SIZE_DEFAULT ;
    this->mode = 3 ;
    this->msgSent = 0 ;
    this->msgReceived = 0 ;
  }

  EmitterReceiver(int emitter, int receiver, int packetSize) {

    this->emitter = emitter ;
    this->receiver = receiver ;
    this->packetSize = packetSize ;
    this->mode = 3 ;
    this->msgSent = 0 ;
    this->msgReceived = 0 ;
  }

    EmitterReceiver(int emitter, int receiver, int packetSize, int mode) {

    this->emitter = emitter ;
    this->receiver = receiver ;
    this->packetSize = packetSize ;
    this->mode = mode ;
    this->msgSent = 0 ;
    this->msgReceived = 0 ;
  }

  void MessageSent() {

    this->msgSent ++ ;
  }

  void MessageReceived() {

    this->msgReceived ++ ;
  }

  uint32_t GetEmitter() {
    return this->emitter ;
  }

  uint32_t GetReceiver() {
    return this->receiver ;
  }

    uint32_t GetMode() {
    return this->mode ;
  }

  uint32_t GetPacketSize() {
    return this->packetSize ;
  }

  uint32_t GetMessageSent() {
    return this->msgSent ;
  }

  uint32_t GetMessageReceived() {
    return this->msgReceived ;
  }

private:
  /** Emitter
   */
  uint32_t emitter ;
  /** Receiver
   */
  uint32_t receiver ;
  /** Packet size
   */
  uint32_t packetSize ;
  /** Messages sent
   */
  uint32_t msgSent ;
  /** Messages received
   */
  uint32_t msgReceived ;

  uint32_t mode ;
} ;

// Vector of links
std::vector<EmitterReceiver> m_vlinks ;
// Hash map for EmitterReceiver results
std::map<std::pair<uint32_t, uint32_t>, EmitterReceiver> s_emitterReceiverResult ;

/* -------------------------------------------------------------------------------------- */
/*                                                                                        */
/*                       All packet information to be stored                              */
/*                                                                                        */
/* -------------------------------------------------------------------------------------- */
/**
 * All packet information to be stored
 */
class PacketInformation {

public:
  PacketInformation(double sentPacketTime) {
    m_sentPacketTime = sentPacketTime ;
    m_receivedPacketTime = -1 ;
    m_ttlValue = -1 ;
  }

  double GetSentPacketTime() {
    return m_sentPacketTime ;
  }

  void SetReceivedPacketTime(double receivedPacketTime) {
    m_receivedPacketTime = receivedPacketTime ;
  }

  double GetReceivedPacketTime() {
    return m_receivedPacketTime ;
  }

  void SetTtlValue(int ttlValue) {

    m_ttlValue = ttlValue ;
  }

  int GetTtlValue() {
    return (m_ttlValue) ;
  }

private:
  /** When the packet was sent
   */
  double m_sentPacketTime ;
  /** When the packet was received
   */
  double m_receivedPacketTime ;
  /** TTL value at end
   */
  int m_ttlValue ;
} ;

// Map to store information about sent time in application layer
std::map<uint64_t, PacketInformation> s_appPacketInformation ;

/* -------------------------------------------------------------------------------------- */
/*                                                                                        */
/*                          Class to build nodes link                                     */
/*                                                                                        */
/* -------------------------------------------------------------------------------------- */

/**
 * Generate the traffic
 */
static void GenerateTraffic (uint32_t nReceiver, uint32_t nEmitter, Ptr<Socket> socket, uint32_t pktSize, double pktInterval ) {

  const uint8_t buffer[pktSize+3] = {1,2,3} ;

  
  // -------------------------------------------------------
  // Packet to be sent
  Ptr<Packet> packet = Create<Packet> (buffer,pktSize);

  PacketInformation packetInfo = PacketInformation(Simulator::Now().GetSeconds()) ;
  s_appPacketInformation.insert(std::pair<uint64_t, PacketInformation>(packet->GetUid(), packetInfo)) ;

  std::pair<uint32_t, uint32_t> pairER = std::make_pair(nEmitter, nReceiver) ;
  std::map<std::pair<uint32_t, uint32_t>, EmitterReceiver>::iterator itRes = s_emitterReceiverResult.find(pairER) ;

  if (itRes != s_emitterReceiverResult.end()) {
    itRes->second.MessageSent() ;
  }

  Ptr<MobilityModel> mob = m_80211pContainer.Get(nReceiver)->GetObject<MobilityModel>();


  Vector pos = mob->GetPosition ();
  //std::cout << "Node " << nReceiver << ": POS: x=" << pos.x << ", y=" << pos.y << std::endl;

  Ptr<MobilityModel> mob1 = m_80211pContainer.Get(nEmitter)->GetObject<MobilityModel>();
  

  Vector pos1 = mob1->GetPosition ();
  //std::cout << "Node " << nEmitter << ": POS: x=" << pos1.x << ", y=" << pos1.y << std::endl;
  

  double distance = sqrt((pos.x - pos1.x)*(pos.x - pos1.x)+(pos.y - pos1.y)*(pos.y - pos1.y)+(pos.z - pos1.z)*(pos.z - pos1.z));


  // ----------------------------------------------------------------------------
  // Display information about sending => does not work for tag to be checked
  double speed = mob1->GetRelativeSpeed (mob) ;
  
  NS_LOG_UNCOND ("++AtApp " << Simulator::Now().GetSeconds()
                 << " s packet " << packet->GetUid()
                 << " send from node " << nEmitter << " to " << nReceiver
                 << " dist: " << distance
                 << " relSpeed: " << speed
                 << " txSizeNet: " << pktSize
                 );


  // -----------------------------------------------------------------------------
  // Adding tag
  MyTag tag;
  tag.SetSNode(nEmitter) ;
  tag.SetRNode(nReceiver) ;
  packet->AddPacketTag (tag);


  // -------------------------------------------------------
  // Send the packet
  socket->Send (packet);


  // -------------------------------------------------------
  // Schedule next packet
  // Adding new random variable
  //double offset = myRandom->GetValue(pktInterval/100 - pktInterval, pktInterval - pktInterval/100) ;
  
  if (Simulator::Now().GetSeconds() < m_stopSendTime)  {
    double offset = 0 ;
    Simulator::Schedule (Seconds(pktInterval + offset), &GenerateTraffic, nReceiver, nEmitter, socket, pktSize, pktInterval);
  }


}


/** Socket application
 * \author Benoit Hilt
 */
void ReceivePacket (Ptr<Socket> socket) {

  // For the NODE id
  // Ptr<Node> node = socket->GetNode() ;

  Ptr<Packet> packet;

  NS_LOG_DEBUG ("--AtApp " << Simulator::Now().GetSeconds() <<" receive 1 Packet ") ;
  
  while ((packet = socket->Recv ())) {

    MyTag tagCopy1;
    if (packet->PeekPacketTag(tagCopy1)) {
      uint32_t sNode=tagCopy1.GetSNode();
      uint32_t rNode=tagCopy1.GetRNode();

      NS_LOG_UNCOND ("--AtApp " << Simulator::Now().GetSeconds()
                     << " s packet " << packet->GetUid()
                     << " send from node " << sNode << " to " << rNode
                     << "), txSizeNet: " << packet->GetSize()
                     );
    }
    else {
      NS_LOG_UNCOND ("--AtApp " << Simulator::Now().GetSeconds()
                     << " s packet " << packet->GetUid()
                     << " txSizeNet: " << packet->GetSize()
                     );
    }
  }
}


/** Load the scenario
 * \return true if scenario correctly loaded
 * \see ns3::Ns2MobilityHelper Class
 */
bool CreateNS2Nodes() {
  m_ns2Helper = new Ns2MobilityHelper(m_scenarioFileName) ;
  m_80211pContainer.Create (m_nNodes80211p) ;
  m_80211bContainer.Create (m_nNodes80211b) ;
  m_waveContainer.Create (m_nNodeswave) ;
  
  m_ns2Helper->Install(); //m_80211pContainer.Begin(), m_80211pContainer().End) ;

  return true ;
}

/**
 * Calculate LineOfSightDoppler
 */
double calculateLineOfSightDoppler(double speed) {

  double lineOfSightDoppler = (speed*5.9e9)/(0.3e9*10e6);

  return lineOfSightDoppler ;
}

/**
 * Yans Wifi installation
 * see https://svn.ensisa.uha.fr/redmine/projects/vanet-ns3/wiki/Configurations_Specifiques
 */
void InstallYansWifi() {

     if (m_lossModel == 1)
     {
       m_lossModelName = "ns3::FriisPropagationLossModel";
     }
   else if (m_lossModel == 2)
     {
       m_lossModelName = "ns3::ItuR1411LosPropagationLossModel";
     }
   else if (m_lossModel == 3)
     {
       m_lossModelName = "ns3::TwoRayGroundPropagationLossModel";
     }
   else if (m_lossModel == 4)
     {
       m_lossModelName = "ns3::LogDistancePropagationLossModel";
     }
   else
     {
       // Unsupported propagation loss model.
       // Treating as ERROR
       NS_LOG_ERROR ("Invalid propagation loss model specified.  Values must be [1-4], where 1=Friis;2=ItuR1411Los;3=TwoRayGround;4=LogDistance");
     }
 
   // frequency
   double freq = 0.0;
   if ((m_80211mode == 1) || (m_80211mode == 3)) {
       // 802.11p 5.9 GHz
       freq = 5.9e9;
   }
   else {
     // 802.11b 2.4 GHz
     freq = 2.4e9;
   }
 
   // Setup propagation models
   YansWifiChannelHelper wifiChannel;
   wifiChannel.SetPropagationDelay ("ns3::ConstantSpeedPropagationDelayModel");
   if (m_lossModel == 3)
     {
       // two-ray requires antenna height (else defaults to Friss)
       wifiChannel.AddPropagationLoss (m_lossModelName, "Frequency", DoubleValue (freq), "HeightAboveZ", DoubleValue (1.5));
     }
   else
     {
       wifiChannel.AddPropagationLoss (m_lossModelName, "Frequency", DoubleValue (freq));
     }
 
   // Propagation loss models are additive.
   if (m_fading != 0)
     {
       // if no obstacle model, then use Nakagami fading if requested
       wifiChannel.AddPropagationLoss ("ns3::NakagamiPropagationLossModel");
     }
 
   // the channel
   Ptr<YansWifiChannel> channel = wifiChannel.Create ();
 
   // The below set of helpers will help us to put together the wifi NICs we want
   YansWifiPhyHelper wifiPhy =  YansWifiPhyHelper::Default ();
   wifiPhy.SetChannel (channel);
   // ns-3 supports generate a pcap trace
   wifiPhy.SetPcapDataLinkType (WifiPhyHelper::DLT_IEEE802_11);
 
   YansWavePhyHelper wavePhy =  YansWavePhyHelper::Default ();
   wavePhy.SetChannel (channel);
   wavePhy.SetPcapDataLinkType (WifiPhyHelper::DLT_IEEE802_11);
 
   // Setup WAVE PHY and MAC
   NqosWaveMacHelper wifi80211pMac = NqosWaveMacHelper::Default ();
   WaveHelper waveHelper = WaveHelper::Default ();
   Wifi80211pHelper wifi80211p = Wifi80211pHelper::Default ();
   if (m_verbose)
     {
       wifi80211p.EnableLogComponents ();      // Turn on all Wifi 802.11p logging
       // likewise, turn on WAVE PHY logging
       waveHelper.EnableLogComponents ();
     }
 
   WifiHelper wifi;
 
   // Setup 802.11b stuff
   wifi.SetStandard (WIFI_PHY_STANDARD_80211b);
 
   wifi.SetRemoteStationManager ("ns3::ConstantRateWifiManager",
                                 "DataMode",StringValue (m_phyMode),
                                 "ControlMode",StringValue (m_phyMode));
 
   // Setup 802.11p stuff
   wifi80211p.SetRemoteStationManager ("ns3::ConstantRateWifiManager",
                                       "DataMode",StringValue (m_phyMode),
                                       "ControlMode",StringValue (m_phyMode));
 
   // Setup WAVE-PHY stuff
   waveHelper.SetRemoteStationManager ("ns3::ConstantRateWifiManager",
                                       "DataMode",StringValue (m_phyMode),
                                       "ControlMode",StringValue (m_phyMode));
 
   // Set Tx Power
   wifiPhy.Set ("TxPowerStart",DoubleValue (m_txPower));
   wifiPhy.Set ("TxPowerEnd", DoubleValue (m_txPower));
   wavePhy.Set ("TxPowerStart",DoubleValue (m_txPower));
   wavePhy.Set ("TxPowerEnd", DoubleValue (m_txPower));
 
   // Add an upper mac and disable rate control
   WifiMacHelper wifiMac;
   wifiMac.SetType ("ns3::AdhocWifiMac");
   QosWaveMacHelper waveMac = QosWaveMacHelper::Default ();
 
   // Setup net devices

  
       m_netDeviceContainerwave= waveHelper.Install (wavePhy, waveMac, m_waveContainer) ;
       m_netDeviceContainer80211p = wifi80211p.Install (wifiPhy, wifi80211pMac, m_80211pContainer) ;
       m_netDeviceContainer80211b = wifi.Install (wifiPhy, wifiMac, m_80211bContainer) ;
 
   if (m_asciiTrace != 0) {
       std::ostringstream logFileName ;
       logFileName << m_outputdir.c_str() << "trFile" << ".tr" ;
       AsciiTraceHelper ascii;
       Ptr<OutputStreamWrapper> osw = ascii.CreateFileStream ( (logFileName.str().c_str()) );
       wifiPhy.EnableAsciiAll (osw);
       wavePhy.EnableAsciiAll (osw);
     }

   if (m_pcap != 0) {
     std::ostringstream fileName ;
     fileName << m_outputdir << "vanet-routing-compare-pcap" ;
     std::cout << fileName.str() << std::endl ;
     wifiPhy.EnablePcapAll (fileName.str().c_str()) ;
     wavePhy.EnablePcapAll (fileName.str().c_str()) ;
   }
}

/**
 * No protocol installed for internet stack
 * \see MobilityIntegrationTest::InstallInternetStack
 */
void InstallNoProtocol () {

  InternetStackHelper stack;
  stack.Install (m_80211pContainer);
  
  Ipv4AddressHelper address;
  address.SetBase ("10.0.0.0", "255.0.0.0");
  m_ipv4InterfaceContainer = address.Assign (m_netDeviceContainer80211p);
  
}


/**
 * No protocol installed for internet stack
 * \see MobilityIntegrationTest::InstallInternetStack
 */
void InstallNoProtocolIpv6 () {
  InternetStackHelper stack;
  stack.Install (m_80211pContainer);
  stack.Install (m_80211bContainer);
  stack.Install (m_waveContainer);

  Ipv6AddressHelper ip1;
  Ipv6Address ipAddr1;
  ipAddr1 = ip1.NewAddress ();
  ip1.SetBase (Ipv6Address ("2001:db81::"), Ipv6Prefix (32), Ipv6Address ("::1"));
  m_ipv6InterfaceContainer = ip1.Assign (m_netDeviceContainer80211p);
  m_ipv6InterfaceContainer = ip1.Assign (m_netDeviceContainer80211b);
  m_ipv6InterfaceContainer = ip1.Assign (m_netDeviceContainerwave);
}



/**
 * Retrieve the IPV4 address from a node
 * \return IP address
 * \param index index of the node in NodeContainer
 */
Ipv4InterfaceAddress GetIpv4Address(uint32_t index) {

  for (uint32_t indexDevice = 0 ; indexDevice < m_netDeviceContainer80211p.GetN() ; indexDevice ++) {

    Ptr<NetDevice> netDevice=m_netDeviceContainer80211p.Get(indexDevice);
    Ptr<Node> node=netDevice->GetNode ();
    if (node != NULL) {
      Ptr<Ipv4> ipv4=node->GetObject<Ipv4> ();

      int32_t interface = ipv4->GetInterfaceForDevice (netDevice);
      uint32_t nb = ipv4->GetNAddresses(interface);

      for(uint32_t indexInterface = 0 ;  indexInterface < nb ; indexInterface ++) {
        Ipv4InterfaceAddress addr = ipv4->GetAddress(interface, indexInterface);

        if (index == node->GetId() ) {
          //std::cerr << "FD::Found the correct node " << node->GetId() << " has address " << addr.GetLocal() << " Mask " << addr.GetMask() << " Broadcast "<< addr.GetBroadcast() << std::endl;
          return addr ;
        }
      }
    }
    else {
      std::cerr << "FD:Node is null" << std::endl ;
    }
  }

  return Ipv4InterfaceAddress() ;
}

/**
 * Retrieve the IPV6 address from a node
 * \return IP address
 * \param index index of the node in NodeContainer
 */
Ipv6InterfaceAddress GetIpv6Address(uint32_t index) {

  for (uint32_t indexDevice = 0 ; indexDevice < m_netDeviceContainer80211p.GetN() ; indexDevice ++) {

    Ptr<NetDevice> netDevice=m_netDeviceContainer80211p.Get(indexDevice);
    Ptr<Node> node=netDevice->GetNode ();
    if (node != NULL) {
      Ptr<Ipv6> ipv6=node->GetObject<Ipv6> ();

      int32_t interface = ipv6->GetInterfaceForDevice (netDevice);
      uint32_t nb = ipv6->GetNAddresses(interface);

      for(uint32_t indexInterface = 0 ;  indexInterface < nb ; indexInterface ++) {
        Ipv6InterfaceAddress addr = ipv6->GetAddress(interface, indexInterface);

        if (index == node->GetId() ) {
          //std::cerr << "FD::Found the correct node " << node->GetId() << " has address " << addr.GetLocal() << " Mask " << addr.GetMask() << " Broadcast "<< addr.GetBroadcast() << std::endl;
          return addr ;
        }
      }
    }
    else {
      std::cerr << "FD:Node is null" << std::endl ;
    }
  }

  return Ipv6InterfaceAddress() ;
}
Ipv6InterfaceAddress GetIpv6Address(uint32_t index, uint32_t mode) {
  if (mode==1)
  {

  for (uint32_t indexDevice = 0 ; indexDevice < m_netDeviceContainer80211p.GetN() ; indexDevice ++) {

    Ptr<NetDevice> netDevice=m_netDeviceContainer80211p.Get(indexDevice);
    Ptr<Node> node=netDevice->GetNode ();
    if (node != NULL) {
      Ptr<Ipv6> ipv6=node->GetObject<Ipv6> ();

      int32_t interface = ipv6->GetInterfaceForDevice (netDevice);
      uint32_t nb = ipv6->GetNAddresses(interface);

      for(uint32_t indexInterface = 0 ;  indexInterface < nb ; indexInterface ++) {
        Ipv6InterfaceAddress addr = ipv6->GetAddress(interface, indexInterface);

        if (index == node->GetId() ) {
          //std::cerr << "FD::Found the correct node " << node->GetId() << " has address " << addr.GetLocal() << " Mask " << addr.GetMask() << " Broadcast "<< addr.GetBroadcast() << std::endl;
          return addr ;
        }
      }
    }
    else {
      std::cerr << "FD:Node is null" << std::endl ;
    }
  }
}
  if (mode==2)
  {

  for (uint32_t indexDevice = 0 ; indexDevice < m_netDeviceContainer80211b.GetN() ; indexDevice ++) {

    Ptr<NetDevice> netDevice=m_netDeviceContainer80211b.Get(indexDevice);
    Ptr<Node> node=netDevice->GetNode ();
    if (node != NULL) {
      Ptr<Ipv6> ipv6=node->GetObject<Ipv6> ();

      int32_t interface = ipv6->GetInterfaceForDevice (netDevice);
      uint32_t nb = ipv6->GetNAddresses(interface);

      for(uint32_t indexInterface = 0 ;  indexInterface < nb ; indexInterface ++) {
        Ipv6InterfaceAddress addr = ipv6->GetAddress(interface, indexInterface);

        if (index == node->GetId() ) {
          //std::cerr << "FD::Found the correct node " << node->GetId() << " has address " << addr.GetLocal() << " Mask " << addr.GetMask() << " Broadcast "<< addr.GetBroadcast() << std::endl;
          return addr ;
        }
      }
    }
    else {
      std::cerr << "FD:Node is null" << std::endl ;
    }
  }
} 
 if (mode==3)
  {

  for (uint32_t indexDevice = 0 ; indexDevice < m_netDeviceContainerwave.GetN() ; indexDevice ++) {

    Ptr<NetDevice> netDevice=m_netDeviceContainerwave.Get(indexDevice);
    Ptr<Node> node=netDevice->GetNode ();
    if (node != NULL) {
      Ptr<Ipv6> ipv6=node->GetObject<Ipv6> ();

      int32_t interface = ipv6->GetInterfaceForDevice (netDevice);
      uint32_t nb = ipv6->GetNAddresses(interface);

      for(uint32_t indexInterface = 0 ;  indexInterface < nb ; indexInterface ++) {
        Ipv6InterfaceAddress addr = ipv6->GetAddress(interface, indexInterface);

        if (index == node->GetId() ) {
          //std::cerr << "FD::Found the correct node " << node->GetId() << " has address " << addr.GetLocal() << " Mask " << addr.GetMask() << " Broadcast "<< addr.GetBroadcast() << std::endl;
          return addr ;
        }
      }
    }
    else {
      std::cerr << "FD:Node is null" << std::endl ;
    }
  }
}
  return Ipv6InterfaceAddress() ;
}

/**
 * Install socket for node communication
 */
void InstallSocketApplications() {

  uint32_t port = 4000 ;
  TypeId tid = TypeId::LookupByName ("ns3::UdpSocketFactory");
    
  if (m_vlinks.size() == 0) {
    NS_FATAL_ERROR ("No traffic defined, please used or a file or random (see parameters: NodesEmitter or TrafficFileName)");
  }

  // Configuration of the applications
  // UdpClient -> UdpServer
  int i = 1 ;
  for (std::vector<EmitterReceiver>::iterator it = m_vlinks.begin() ; it != m_vlinks.end() ; it ++, i++) {
   
    uint32_t nNodeServer = it->GetReceiver() ;
    uint32_t nNodeClient = it->GetEmitter() ;
    uint32_t packetSize = it->GetPacketSize() ;
    uint32_t mode = it->GetMode() ;
    NS_LOG_UNCOND("Link " << i << ": node " << it->GetEmitter() << " send information to " << it->GetReceiver()<< " with " << it->GetMode() ) ;
    // Configure the server
    Ipv6InterfaceAddress destAddress = GetIpv6Address(nNodeServer, mode) ;
    Ptr<Socket> recvSink;
    Ptr<Socket> source ;
    

    if (mode==3)
    {

      recvSink = Socket::CreateSocket (m_waveContainer.Get (nNodeServer), tid);

      // Configure the client
       source = Socket::CreateSocket ( m_waveContainer.Get(nNodeClient), tid);
    }
    if (mode==2)
      {

        recvSink = Socket::CreateSocket (m_80211bContainer.Get (nNodeServer), tid);
        source = Socket::CreateSocket ( m_80211bContainer.Get(nNodeClient), tid);
      }
    if (mode==1)
    {
      

        recvSink = Socket::CreateSocket (m_80211pContainer.Get (nNodeServer), tid);
        source = Socket::CreateSocket ( m_80211pContainer.Get(nNodeClient), tid);

    }
    

    //Ptr<Socket> recvSink = Socket::CreateSocket (m_80211pContainer.Get (nNodeServer), tid);
      Inet6SocketAddress local = Inet6SocketAddress (Ipv6Address::GetAny(), port);
      recvSink->Bind (local);
      recvSink->SetRecvCallback (MakeCallback (&ReceivePacket));  



    Inet6SocketAddress remote = Inet6SocketAddress (destAddress.GetAddress(), port);
    source->SetAllowBroadcast (true);
    source->Bind();
    source->Connect (remote);  // Used to set the destination address of the outgoing packets
    

    // Display
    m_nNodesEmitter = m_vlinks.size() ;
    double offset = 0.001*(i-1) ; //* nNodeClient ;
    Ipv6InterfaceAddress srcAddress = GetIpv6Address(nNodeClient, mode) ;
    NS_LOG_UNCOND("\tClient " << srcAddress.GetAddress() << " (" << nNodeClient << ") sends information to " << destAddress.GetAddress() << " (" << nNodeServer << ")" << " (start time = " << Seconds(m_startSendTime + offset).GetSeconds() << " s)") ;

    Simulator::Schedule (Seconds(m_startSendTime + offset), &GenerateTraffic, nNodeServer, nNodeClient, source, packetSize, m_packetInterval);
  }
}


/**
 * Les applications => depend of the traffic file name => DEFAULT -> RANDOM applications, NOT DEFAULT => take the filename and parse it
 */
void InstallApplications () {

  std::ifstream trafficFile ;
  trafficFile.open(m_trafficFileName.c_str()) ;
  if (!trafficFile.is_open()) {
    NS_FATAL_ERROR("Cannot read traffic file " << m_trafficFileName << " please check the file name") ;
  }
  else {
    int iNodeEmit, iNodeRec, packetSize, mode ;
    std::string line ;
    while (std::getline(trafficFile,line)) {
      iNodeEmit = -1 ; iNodeRec = -1 ; packetSize = -1 ;
      if (line.find("#") == std::string::npos && !line.empty()) {
        std::istringstream iss(line);
        iss >> iNodeEmit >> iNodeRec >> packetSize >>mode;

        if ((iNodeEmit < 0) || (iNodeRec < 0)) {
          trafficFile.close() ;
          NS_FATAL_ERROR("Incorrect traffic file, should be node number and found \"" << line << "\", please review traffic file") ;
        }
        else {
          if (((uint32_t)iNodeEmit >= m_nNodes) || ((uint32_t)iNodeRec >= m_nNodes)) {
            trafficFile.close() ;
            NS_FATAL_ERROR("Incorrect traffic file, node number should be number of nodes, found emitter = " << iNodeEmit << " and receiver = " << iNodeRec << " regarding total node number of " << m_nNodes) ;
          }
          else {
            if (packetSize <= 0) packetSize = m_packetSize ;
            m_vlinks.push_back(EmitterReceiver(iNodeEmit, iNodeRec, packetSize, mode)) ;
          }
        }
      }
    }

    trafficFile.close();
  }

  // Add the emitter receiver from m_vlinks into map
  for (std::vector<EmitterReceiver>::iterator it = m_vlinks.begin() ; it != m_vlinks.end() ; it ++) {
    std::pair<uint32_t, uint32_t> pairER = std::make_pair(it->GetEmitter(), it->GetReceiver()) ;
    s_emitterReceiverResult.insert(std::make_pair(pairER, *it)) ;
  }

  InstallSocketApplications() ;
}


/* ********************************************************************************************* */
/* ********************************************************************************************* */
/* ********************************************************************************************* */
/* ********************************************************************************************* */
int main (int argc, char *argv[]) {

  CommandLine cmd;
  cmd.AddValue ("scenario", "Scenario file name",  m_scenarioFileName) ;
  cmd.AddValue ("traffic", "Traffic file name", m_trafficFileName) ;
  cmd.AddValue ("packetSize", "Packet Size", m_packetSize) ;
  cmd.AddValue ("nodes", "Number of nodes", m_nNodes) ;
  cmd.AddValue ("pcap", "PCAP files enable or disable", m_pcap) ;
  cmd.AddValue ("test", "Numéro du test", m_testNumber) ;
  cmd.Parse (argc, argv);

  std::cout << "Scenario File Name = " << m_scenarioFileName << std::endl ;
  std::cout << "Traffic File Name = " << m_trafficFileName << std::endl ;
  std::cout << "Packet size = " << m_packetSize << std::endl ;
  std::cout << "Number of nodes = " << m_nNodes << std::endl ;
  std::cout << "PCAP files = " << m_pcap << std::endl ;
  //std::cout << "Test number = " << m_testNumber << std::endl ;

  std::ostringstream outputdir ;
  outputdir << m_outputdir << "/test" << m_testNumber << "/" ;
  m_outputdir = outputdir.str() ;
  std::cout << "Output dir " << m_outputdir << std::endl ;

  if (mkdir(m_outputdir.c_str(), S_IRUSR | S_IWUSR | S_IXUSR | S_IRGRP | S_IXGRP) != 0) {
    //if (error == EEXIST)
    NS_LOG_UNCOND("Directory " << m_outputdir << " exists, all files will be replaced") ;
        //else
        //NS_FATAL_ERROR ("Unable to create dir " << m_outputdir << " error " << error << " / " << EEXIST) ;
    }

  CreateNS2Nodes();
  InstallYansWifi();
  InstallNoProtocolIpv6() ;
  InstallApplications() ;

  
  // -------------------------------------------------------------------------------------           
  
  /*
  std::ostringstream logFileNameAnim ;
  logFileNameAnim << m_outputdir.c_str() << "animation" << ".xml" ;
  //std::cout << "Opening file " << logFileNameAnim.str() << std::endl ;                             
  AnimationInterface anim (logFileNameAnim.str().c_str()); // Mandatory    
  */

  Simulator::Run ();
  Simulator::Destroy ();

  return 0;
}
