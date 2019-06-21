///* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
///*
// * Copyright 2007 University of Washington
// *
// * This program is free software; you can redistribute it and/or modify
// * it under the terms of the GNU General Public License version 2 as
// * published by the Free Software Foundation;
// *
// * This program is distributed in the hope that it will be useful,
// * but WITHOUT ANY WARRANTY; without even the implied warranty of
// * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// * GNU General Public License for more details.
// *
// * You should have received a copy of the GNU General Public License
// * along with this program; if not, write to the Free Software
// * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
// */
//#include "ns3/log.h"
//#include "ns3/ipv4-address.h"
//#include "ns3/ipv6-address.h"
//#include "ns3/nstime.h"
//#include "ns3/inet-socket-address.h"
//#include "ns3/inet6-socket-address.h"
//#include "ns3/socket.h"
//#include "ns3/simulator.h"
//#include "ns3/socket-factory.h"
//#include "ns3/packet.h"
//#include "ns3/uinteger.h"
//#include "ns3/trace-source-accessor.h"
//#include "mec-ho-server-application.h"
//#include "ns3/inet-socket-address.h"
//#include <sstream>
//
//namespace ns3 {
//
//    NS_LOG_COMPONENT_DEFINE ("MecHoServerApplication");
//
////    NS_OBJECT_ENSURE_REGISTERED (MecHoServerApplication);
//
////    std::vector<InetSocketAddress> m_mecAddresses;
//
//    int MEAS_FREQ = 10;
//    int MEC_RATE = 500;
//    int UE_SIZE = 200;
//
////    Ptr<Node> m_thisNode = this->GetNode();
////    Ptr<NetDevice> m_thisNetDevice = m_thisNode.GetDevice(0);
////    Ipv4Address m_thisIpAddress = InetSocketAddress::ConvertFrom(m_thisNetDevice.GetAddress());  //Used for logging purposes
//
//
//    uint32_t m_expectedWaitingTime = 0;
//    int m_noClients = 0;
//    int m_noUes = 800; //TODO hardcoded for now, finetune later
//    int m_noHandovers = 0;
//    Time m_startTime;
//    Time noSendUntil = Simulator::Now();
//
//    TypeId
//    MecHoServerApplication::GetTypeId (void)
//    {
//        static TypeId tid = TypeId ("ns3::MecHoServerApplication")
//                .SetParent<Application> ()
//                .SetGroupName("Applications")
//                .AddConstructor<MecHoServerApplication> ()
//                .AddAttribute ("MaxPackets",
//                               "The maximum number of packets the application will send",
//                               UintegerValue (100),
//                               MakeUintegerAccessor (&MecHoServerApplication::m_count),
//                               MakeUintegerChecker<uint32_t> ())
////                .AddAttribute ("UpdateInterval",
////                               "The time to wait between serviceRequest packets",
////                               TimeValue (Seconds (1.0)),
////                               MakeTimeAccessor (&MecHoServerApplication::m_updateInterval),
////                               MakeTimeChecker ())
////                .AddAttribute ("PacketSize", "Size of echo data in outbound packets",
////                               UintegerValue (100),
////                               MakeUintegerAccessor (&MecHoServerApplication::m_packetSize),
////                               MakeUintegerChecker<uint32_t> ())
////                .AddAttribute ("OrcAddress", "InetSocketAddress of the orchestrator",
////                               AddressValue(InetSocketAddress(Ipv4Address::GetAny(), uint16_t(2035))),
////                               MakeAddressAccessor (&MecHoServerApplication::m_orcAddress),
////                               MakeAddressChecker())
////                .AddAttribute ("cellID", "ID of the eNB this server is associated with",
////                               UintegerValue(-1),
////                               MakeUintegerAccessor (&MecHoServerApplication::m_cellId),
////                               MakeUintegerChecker<uint32_t> ())
////                .AddTraceSource ("Tx", "A new packet is created and is sent",
////                                 MakeTraceSourceAccessor (&MecHoServerApplication::m_txTrace),
////                                 "ns3::Packet::TracedCallback")
//        ;
//        return tid;
//    }
//
//    MecHoServerApplication::MecHoServerApplication ()
//    {
//        NS_LOG_FUNCTION (this);
//        m_sent = 0;
//        m_socket = 0;
//        m_sendEvent = EventId ();
//        m_startTime = Simulator::Now();
//    }
//
//    MecHoServerApplication::~MecHoServerApplication()
//    {
//        NS_LOG_FUNCTION (this);
//        m_socket = 0;
//
//    }
//
//    void
//    MecHoServerApplication::DoDispose (void)
//    {
//        NS_LOG_FUNCTION (this);
//        Application::DoDispose ();
//    }
//
//    void
//    MecHoServerApplication::StartApplication (void)
//    {
//        NS_LOG_FUNCTION (this);
//
//        if (m_socket == 0)
//        {
//            TypeId tid = TypeId::LookupByName ("ns3::UdpSocketFactory");
//            m_socket = Socket::CreateSocket (GetNode (), tid);
//            m_socket->Bind ();
//            m_socket->Connect (m_orcAddress);
//        }
//
//        m_socket->SetRecvCallback (MakeCallback (&MecHoServerApplication::HandleRead, this));
//        m_socket->SetAllowBroadcast (true);
//        SendWaitingTimeUpdate();
//    }
//
//    void
//    MecHoServerApplication::StopApplication ()
//    {
//        NS_LOG_FUNCTION (this);
//
//        if (m_socket != 0)
//        {
//            m_socket->Close ();
//            m_socket->SetRecvCallback (MakeNullCallback<void, Ptr<Socket> > ());
//            m_socket = 0;
//        }
//
//        Simulator::Cancel (m_sendEvent);
//    }
//
//    uint8_t
//    MecHoServerApplication::SetFill (uint8_t *fill, uint32_t fillSize, uint32_t packetSize)
//    {
//        NS_LOG_FUNCTION (this << fill << fillSize << packetSize);
//        uint8_t *result;
//
//        if (fillSize >= packetSize)
//        {
//            memcpy (result, fill, packetSize);
//            m_size = packetSize;
//            return;
//        }
//
//        //
//        // Do all but the final fill.
//        //
//        uint32_t filled = 0;
//        while (filled + fillSize < packetSize)
//        {
//            memcpy (&result[filled], fill, fillSize);
//            filled += fillSize;
//        }
//
//        //
//        // Last fill may be partial
//        //
//        memcpy (&result[filled], fill, packetSize - filled);
//
//        //
//        // Overwrite packet size attribute.
//        //
//        m_size = packetSize;
//        return result;
//    }
//
//    void
//    MecHoServerApplication::SendWaitingTimeUpdate (void) {
//        NS_ASSERT(m_sendEvent.IsExpired());
//
//        if (Simulator::Now() > noSendUntil) {
//            //Bind to correct destination (ORC)
//            m_socket->Bind();
//            m_socket->Connect(m_orcAddress);
//
//            //Calculate waiting time (in ms)
//            double serviceRho = (m_noClients * MSG_FREQ) / MEC_RATE;
//            double expectedServiceWaitingTime = (serviceRho / (1 - serviceRho)) * (1 / MEC_RATE);
//            int pingRho = (m_noUes * MEAS_FREQ) / MEC_RATE;
//            double expectedPingWaitingTime = (pingRho / (1 - pingRho)) * (1 / MEC_RATE);
//            double handoverFrequency = m_noHandovers / ((Simulator::Now() - m_startTime).GetSeconds());
//            int handoverRho = handoverFrequency / MEC_RATE;
//            double expectedHandoverWaitingTime = (handoverRho / (1 - handoverRho)) * (1 / MEC_RATE);
//            m_expectedWaitingTime = int(
//                    (expectedServiceWaitingTime + expectedPingWaitingTime + expectedHandoverWaitingTime) * 1000);
//
//            //Create packet payload
//            std::string fillString = "5/" + std::to_string(m_expectedWaitingTime) + "/";
//            uint8_t *buffer = fillString.c_str();
//            uint8_t *payload = SetFill(buffer, buffer.size(), m_packetSize);
//
//            //Send packet
//            Ptr <Packet> p = Create<Packet>(payload, m_packetSize);
//            m_txTrace(p);
//            m_socket->Send(p);
//
//            ++m_sent;
//
//            if (m_sent < m_count) {
//                m_sendEvent = Simulator::Schedule(m_updateInterval, &MecHoServerApplication::SendWaitingTimeUpdate, this);
//            }
//        }
//        else {
//            SendWaitingTimeUpdate();
//        }
//
//    }
//
//    void
//    MecHoServerApplication::SendUeTransfer (InetSocketAddress newMec)
//    {
//        if (Simulator::Now() > noSendUntil){
//            m_socket->Bind();
//            m_socket->Connect(newMec);
//
//            //Create packet payload
//            std::string fillString = "lorem ipsum";
//            uint8_t *buffer = fillString.c_str();
//            unint8_t *payload = SetFill(buffer, buffer.size(), UE_SIZE);
//
//            //Create packet
//            Ptr <Packet> p = Create<Packet>(payload, UE_SIZE);
//            // call to the trace sinks before the packet is actually sent,
//            // so that tags added to the packet can be sent as well
//            m_txTrace(p);
//            m_socket->Send(p);
//
//            ++m_sent;
//        }
//        else {
//            SendUeTransfer(newMec);
//        }
//
//    }
//
//
//        void
//        MecHoServerApplication::SendEcho (Packet packet)
//        {
//            if(Simulator::Now() > noSendUntil){
//                m_txTrace(packet);
//                m_socket->Send(packet);
//
//                ++m_sent;
//            }
//            else {
//                SendEcho(packet);
//            }
//        }
//
//    void
//    MecHoServerApplication::HandleRead (Ptr<Socket> socket)
//    {
//        NS_LOG_FUNCTION (this << socket);
//        Ptr<Packet> packet;
//        Address from;
//        while ((packet = socket->RecvFrom (from)))
//        {
//            if (InetSocketAddress::IsMatchingType (from))
//            {
//                InetSocketAddress inet_from = InetSocketAddress::ConvertFrom(from);
//                Ipv4Address from_ipv4 = inet_from.GetIpv4();
//
//                //Get payload from packet
//                uint32_t packetSize = packet->GetSize();
//                uint8_t *buffer = new uint8_t[packetSize];
//                packet->CopyData(buffer, packetSize);
//                std::string payloadString = std::string((char*)buffer);
//
//                //Split the payload string into arguments
//                std::string tempString;
//                std::vector<std::string> args;
//                for (int i = 0 ; i < payloadString.length(); i++){
//                    char c = payloadString[i];
//                    if(c == "/"){
//                        args.push_back(tempString);
//                        tempString = "";
//                    }
//                    else{
//                        tempString.push_back(c);
//                    }
//                }
//
//                switch(args[0]){
//                    case "1":
//                        //service request from ue
//                        int ue_cellId = int(args[1]);
//                        int delay = 0; //in ms
//
//                        if (m_cellId == ue_cellId){
//                            delay = m_expectedWaitingTime;
//                        }
//                        else {
//                            //UE is connected to another eNB; add "penalty" for having to go through network
//                            delay = m_expectedWaitingTime + 15;
//                        }
//                        //Bind to correct socket
//                        m_socket->Bind ();
//                        m_socket->Connect (inet_from);
//
//                        //Echo packet back to sender with appropriate delay
//                        Simulator::Schedule(Milliseconds(delay), &SendEcho, packet);
//
//                        break;
//                    case "4":
//                        //handover command from orc
//                        //Get new MEC address
//                        std::string addressString = args[1];
//                        std::string portString = args[2];
//
//                        Ipv4Address newAddress = Ipv4Address();
//                        newAdress.Set(addressString.c_str());
//
//                        uint16_t newPort = std::stoi(portString);
//
//                        //Initiate handover
//                        Simulator::Schedule(Seconds(0), &SendUeTransfer, InetSocketAddress(newAddress, newPort));
//                        break;
//                    case "7":
//                        //handover data from other mec
//                        noSendUntil = Simulator::Now() + m_expectedWaitingTime;
//                        break;
//                }
//            }
//        }
//    }
//} // Namespace ns3
