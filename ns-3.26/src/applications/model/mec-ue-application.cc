/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright 2007 University of Washington
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
 */
#include "ns3/log.h"
#include "ns3/nstime.h"
#include "ns3/simulator.h"
#include "ns3/socket-factory.h"
#include "ns3/packet.h"
#include "ns3/uinteger.h"
#include "ns3/trace-source-accessor.h"
#include "mec-ue-application.h"
#include "ns3/udp-socket-factory.h"
#include <sstream>
#include <fstream>

namespace ns3 {

    NS_LOG_COMPONENT_DEFINE ("MecUeApplication");

    NS_OBJECT_ENSURE_REGISTERED (MecUeApplication);

    TypeId
    MecUeApplication::GetTypeId (void)
    {
        static TypeId tid = TypeId ("ns3::MecUeApplication")
                .SetParent<Application> ()
                .SetGroupName("Applications")
                .AddConstructor<MecUeApplication> ()
                .AddAttribute ("MaxPackets",
                               "The maximum number of packets the application will send",
                               UintegerValue (100),
                               MakeUintegerAccessor (&MecUeApplication::m_count),
                               MakeUintegerChecker<uint32_t> ())
                .AddAttribute ("PacketSize",
                               "The size of payload of a packet",
                               UintegerValue (),
                               MakeUintegerAccessor (&MecUeApplication::m_size),
                               MakeUintegerChecker<uint32_t> ())
                .AddAttribute ("ServiceInterval",
                               "The time to wait between serviceRequest packets",
                               TimeValue (Seconds (1.0)),
                               MakeTimeAccessor (&MecUeApplication::m_serviceInterval),
                               MakeTimeChecker ())
                .AddAttribute ("PingInterval",
                               "The time to wait between pingRequest packets",
                               TimeValue (Seconds (1.0)),
                               MakeTimeAccessor (&MecUeApplication::m_pingInterval),
                               MakeTimeChecker ())
                .AddAttribute ("MecIp",
                               "Ipv4Address of the server ti which this UE is connected",
                               Ipv4AddressValue (),
                               MakeIpv4AddressAccessor (&MecUeApplication::m_mecIp),
                               MakeIpv4AddressChecker ())
               .AddAttribute ("MecPort",
                               "Port of the server ti which this UE is connected",
                               UintegerValue (100),
                               MakeUintegerAccessor (&MecUeApplication::m_mecPort),
                               MakeUintegerChecker<uint32_t> ())
                .AddAttribute ("OrcIp",
                               "Ipv4Address of the ORC to which this UE is connected",
                               Ipv4AddressValue (),
                               MakeIpv4AddressAccessor (&MecUeApplication::m_orcIp),
                               MakeIpv4AddressChecker ())
                .AddAttribute ("OrcPort",
                               "Port of the ORC to which this UE is connected",
                               UintegerValue (),
                               MakeUintegerAccessor (&MecUeApplication::m_orcPort),
                               MakeUintegerChecker<uint32_t> ())
                .AddAttribute ("Local",
                               "Local IP address",
                               Ipv4AddressValue (),
                               MakeIpv4AddressAccessor (&MecUeApplication::m_thisIpAddress),
                               MakeIpv4AddressChecker ())
                .AddAttribute ("AllServers", "Container of all server addresses",
                               StringValue("x"),
                               MakeStringAccessor (&MecUeApplication::m_serverString),
                               MakeStringChecker())
                .AddAttribute ("Enb0", "nth enb in the system",
                               PointerValue(),
                               MakePointerAccessor (&MecUeApplication::m_enb0),
                               MakePointerChecker<LteEnbNetDevice> ())
                .AddAttribute ("Enb1", "nth enb in the system",
                               PointerValue(),
                               MakePointerAccessor (&MecUeApplication::m_enb1),
                               MakePointerChecker<LteEnbNetDevice> ())
                .AddAttribute ("Enb2", "nth enb in the system",
                               PointerValue(),
                               MakePointerAccessor (&MecUeApplication::m_enb2),
                               MakePointerChecker<LteEnbNetDevice> ())
                .AddAttribute ("Node", "Node on which this application will run",
                               PointerValue(),
                               MakePointerAccessor (&MecUeApplication::m_thisNode),
                               MakePointerChecker<Node> ())
                .AddAttribute ("ueImsi",
                               "IMSI of this UE",
                               UintegerValue (),
                               MakeUintegerAccessor (&MecUeApplication::ueImsi),
                               MakeUintegerChecker<uint64_t> ())
                .AddTraceSource ("Tx", "A new packet is created and is sent",
                                 MakeTraceSourceAccessor (&MecUeApplication::m_txTrace),
                                 "ns3::Packet::TracedCallback")
        ;
        return tid;
    }

    MecUeApplication::MecUeApplication ()
    {
        NS_LOG_FUNCTION (this);
        m_sent = 0;
        m_sendPingEvent = EventId ();
        m_sendServiceEvent = EventId ();
//        NS_LOG_DEBUG("SendServiceEvent: " << m_sendServiceEvent.GetUid());
        m_data_request = 0;
        m_data_ping = 0;
        m_data_report = 0;
        m_requestBlocked = false;
//        m_thisNetDevice = m_thisNode->GetDevice(0);

//        std::vector<std::string> args;
//        std::string tempString;
//        NS_LOG_DEBUG("Length serverstring: " << m_serverString.length());
//        for (int i = 0 ; i < int(m_serverString.length()); i++){
//            char c = m_serverString[i];
//            if(c == '/'){
//            args.push_back(tempString);
//            tempString = "";
//            }
//            else{
//            tempString.push_back(c);
//            }
//        }
//        for(int i = 0; i< int(args.size()) ; i++){
//            NS_LOG_DEBUG("Args loop");
//            Ipv4Address ipv4 = Ipv4Address();
//            std::string addrString = args[i];
//            char cstr[addrString.size() + 1];
//            addrString.copy(cstr, addrString.size()+1);
//            cstr[addrString.size()] = '\0';
//            ipv4.Set(cstr);
//            m_allServers.push_back(InetSocketAddress(ipv4,1001));
//
//
//        }
    }

    MecUeApplication::~MecUeApplication()
    {
        NS_LOG_FUNCTION (this);

        delete [] m_data_request;
        delete [] m_data_ping;
        delete [] m_data_report;
        m_data_request = 0;
        m_data_ping = 0;
        m_data_report = 0;

    }

    void
    MecUeApplication::DoDispose (void)
    {
        NS_LOG_FUNCTION (this);
        Application::DoDispose ();
    }

    void
    MecUeApplication::StartApplication (void)
    {
        NS_LOG_FUNCTION (this);

        std::vector<std::string> args;
        std::string tempString;
        for (int i = 0 ; i < int(m_serverString.length()); i++){
            char c = m_serverString[i];
            if(c == '/'){
                args.push_back(tempString);
                tempString = "";
            }
            else{
                tempString.push_back(c);
            }
        }
        for(int i = 0; i< int(args.size()) ; i++){
            Ipv4Address ipv4 = Ipv4Address();
            std::string addrString = args[i];
            char cstr[addrString.size() + 1];
            addrString.copy(cstr, addrString.size()+1);
            cstr[addrString.size()] = '\0';
            ipv4.Set(cstr);
            m_allServers.push_back(InetSocketAddress(ipv4,1001));


        }

        //Make ORC socket
        if (m_orcSocket == 0)
        {
            TypeId tid = TypeId::LookupByName ("ns3::UdpSocketFactory");
            m_orcSocket = Socket::CreateSocket (GetNode (), tid);
            m_orcSocket->Bind ();
            m_orcSocket->Connect (InetSocketAddress(m_orcIp, m_orcPort));
        }
        m_orcSocket->SetRecvCallback (MakeCallback (&MecUeApplication::HandleRead, this));
        m_orcSocket->SetAllowBroadcast (true);

        //Make socket for each server
        for (std::vector<InetSocketAddress>::iterator it = m_allServers.begin(); it != m_allServers.end(); ++it){
            TypeId tid = TypeId::LookupByName ("ns3::UdpSocketFactory");
            Ptr<Socket> tempSocket;
            tempSocket = Socket::CreateSocket (GetNode (), tid);
            tempSocket->Bind ();
            //TODO check that the below returns the correct type etc.
            InetSocketAddress inet = (*it);
            tempSocket->Connect (inet);
            tempSocket->SetRecvCallback (MakeCallback (&MecUeApplication::HandleRead, this));
            tempSocket->SetAllowBroadcast (true);
            std::pair<InetSocketAddress, Ptr<Socket>>  newPair = std::pair<InetSocketAddress, Ptr<Socket>>(inet, tempSocket);
            serverSocketMap.insert(newPair);

            //set currentMecSocket
            if ((*it).GetIpv4() == m_mecIp && (*it).GetPort() == m_mecPort){
                //this MEC is our current MEC
                currentMecSocket = tempSocket;
            }
        }



        m_sendServiceEvent = Simulator::Schedule (Seconds(2), &MecUeApplication::SendServiceRequest, this);
//        NS_LOG_DEBUG("SendServiceEvent 2: " << m_sendServiceEvent.GetUid());
        m_sendPingEvent = Simulator::Schedule(Seconds(2), &MecUeApplication::SendPing, this);
    }


    void
    MecUeApplication::StopApplication ()
    {
        NS_LOG_FUNCTION (this);

        if (m_orcSocket != 0)
        {
            m_orcSocket->Close ();
            m_orcSocket->SetRecvCallback (MakeNullCallback<void, Ptr<Socket> > ());
            m_orcSocket = 0;
        }

        std::map<InetSocketAddress, Ptr<Socket>>::iterator it;
        for (it = serverSocketMap.begin(); it != serverSocketMap.end(); ++it){
            Ptr<Socket> tempSocket = it->second;
            tempSocket->Close ();
            tempSocket->SetRecvCallback (MakeNullCallback<void, Ptr<Socket> > ());
            tempSocket = 0;
        }

        Simulator::Cancel (m_sendPingEvent);
        Simulator::Cancel (m_sendServiceEvent);
    }


    uint8_t*
    MecUeApplication::GetFilledString (std::string filler, int size) {
        NS_LOG_FUNCTION(this << filler);

        std::string result;
        uint8_t *val = (uint8_t *) malloc(size + 1);

        int fillSize = filler.size();

        if (fillSize >= int(size)) {
            StopApplication();
        } else {
            result.append(filler);
            for (int i = result.size(); i < int(size); i++) {
                result.append("#");
            }

            std::memset(val, 0, size + 1);
            std::memcpy(val, result.c_str(), size + 1);
        }
        return val;
    }


    uint16_t MecUeApplication::CheckEnb(Ptr<LteEnbNetDevice> enb) {
        NS_LOG_FUNCTION(this);
        uint16_t result = 65000;

        Ptr<LteEnbRrc> rrc = enb->GetRrc();
        std::map<uint16_t, Ptr<UeManager>> ueMap = rrc->GetUeMap();
        std::map<uint16_t, Ptr<UeManager>>::iterator it;
        for(it = ueMap.begin(); it != ueMap.end(); ++it){
            Ptr<UeManager> manager = it->second;
            uint64_t imsi = manager->GetImsi();
            if(ueImsi == imsi){
                result = enb->GetCellId();
                break;
            }
        }
        return result;
    }

    uint16_t MecUeApplication::GetCellId(){
        NS_LOG_FUNCTION(this);

        uint16_t result;
        uint16_t enb0_result = CheckEnb(m_enb0);

        if(int(enb0_result) != 65000){
            result = enb0_result;
        }
        else {
            uint16_t enb1_result = CheckEnb(m_enb1);
            if(int(enb1_result) != 65000){
                result = enb1_result;
            }
            else {
                uint16_t enb2_result = CheckEnb(m_enb2);
                if(int(enb2_result) != 65000){
                    result = enb2_result;
                }
                else{
                    NS_LOG_ERROR("No CellID found");
                    StopApplication();
                }
            }

        }
        return result;

    }

//    void
//    MecUeApplication::NextService(Time interval){
//        NS_LOG_FUNCTION(this);
//        m_sendServiceEvent = Simulator::Schedule (MilliSeconds(150), &MecUeApplication::SendServiceRequest, this);
////        NS_LOG_DEBUG("SendServiceEvent 3: " << m_sendServiceEvent.GetUid());
//    }

    void
    MecUeApplication::SendServiceRequest (void) {
        NS_LOG_FUNCTION(this);
//        NS_LOG_DEBUG("Delay left: " << Simulator::GetDelayLeft(m_sendServiceEvent));
        NS_ASSERT (m_sendServiceEvent.IsExpired ());

        if (Simulator::Now() < m_noSendUntil){
            m_requestSent = Simulator::Now();
            m_requestBlocked = true;

        }
        else {
            if (!m_requestBlocked){
                m_requestSent = Simulator::Now();
            }
            //Create packet payloadU
            std::string fillString = "1/" + std::to_string(GetCellId()) + "/";
            uint8_t *buffer = GetFilledString(fillString, m_size);
//            NS_LOG_DEBUG("After getfilled in sendservice");


            //Create packet
            Ptr<Packet> p = Create<Packet> (buffer, m_size);
            // call to the trace sinks before the packet is actually sent,
            // so that tags added to the packet can be sent as well
            m_txTrace (p);
//            NS_LOG_DEBUG("test2");
//            NS_LOG_DEBUG("currentMecSocket: " << currentMecSocket->GetSocketType());
            currentMecSocket->Send (p);
//            NS_LOG_DEBUG("test3");

            ++m_sent;
//            NS_LOG_DEBUG("After sending packet");


            if (m_sent < m_count)
            {
                m_sendServiceEvent = Simulator::Schedule (m_serviceInterval, &MecUeApplication::SendServiceRequest, this);
//                NextService(m_serviceInterval);
            }

            m_requestBlocked = false;

        }
    }

    void
    MecUeApplication::SendPing (void)
    {
        NS_ASSERT (m_sendPingEvent.IsExpired ());
        m_pingSent.clear();
        for (InetSocketAddress mec: m_allServers){
            if (Simulator::Now() < m_noSendUntil){
                m_requestSent = Simulator::Now();
                m_requestBlocked = true;
            }
            else {
                m_pingSent.insert(std::pair<Ipv4Address, Time>(mec.GetIpv4(), Simulator::Now()));


                //Create packet payload
                std::string fillString = "1/";
                fillString.append(std::to_string(GetCellId()) + "/");
                uint8_t *buffer = GetFilledString(fillString, m_size);
//                NS_LOG_DEBUG("After getfilled in sendping");

                //Create packet
                Ptr <Packet> p = Create<Packet>(buffer, m_size);
                // call to the trace sinks before the packet is actually sent,
                // so that tags added to the packet can be sent as well
                m_txTrace(p);

                //Determine correct server socket and send
                Ptr<Socket> pingSocket = serverSocketMap.find(mec)->second;
                pingSocket->Send(p);

                ++m_sent;

                if (m_sent < m_count) {
                    NS_LOG_DEBUG("Ping interval: " << m_pingInterval);
                    m_sendPingEvent = Simulator::Schedule(MilliSeconds(m_pingInterval), &MecUeApplication::SendPing, this);
//                    NS_LOG_DEBUG("SendServiceEvent 4: " << m_sendServiceEvent.GetUid());
                }

                m_requestBlocked = false;
            }
        }
    }

    void
    MecUeApplication::SendMeasurementReport (void){

        //Create packet payload
        //Convert current mec address into string
        std::stringstream ss;
        std::ofstream os ("temp.txt", std::ofstream::out);
        m_mecIp.Print(os);
        ss << os.rdbuf();
        std::string m_mecIpString = ss.str();
        std::string payload = "2/" + m_mecIpString + "/";

        for (std::map<Ipv4Address,int64_t>::iterator it = m_measurementReport.begin(); it != m_measurementReport.end(); ++it){
            //Convert Address into string
            Ipv4Address addr = it->first;
            addr.Print(os);
            ss << os.rdbuf();
            std::string addrString = ss.str();

            payload.append(addrString + "!" + std::to_string(it->second) + "!");
        }

        payload.push_back('/'); //! is separator character for the measurementReport

        uint8_t *buffer = GetFilledString(payload, m_size);
//        NS_LOG_DEBUG("after getfilled in sendmeasurementreport");

        //Create packet
        Ptr<Packet> p = Create<Packet> (buffer, m_size);
        // call to the trace sinks before the packet is actually sent,
        // so that tags added to the packet can be sent as well
        m_txTrace (p);
        m_orcSocket->Send (p);

        ++m_sent;

    }

    void
    MecUeApplication::HandleRead (Ptr<Socket> socket)
    {
        NS_LOG_FUNCTION (this << socket);
        Ptr<Packet> packet;
        Address from;
        while ((packet = socket->RecvFrom (from)))
        {
            if (InetSocketAddress::IsMatchingType (from))
            {
                InetSocketAddress inet_from = InetSocketAddress::ConvertFrom(from);
                Ipv4Address from_ipv4 = inet_from.GetIpv4();

                //Get payload from packet
                uint32_t packetSize = packet->GetSize();
                uint8_t *buffer = new uint8_t[packetSize];
                packet->CopyData(buffer, packetSize);
                std::string payloadString = std::string((char*)buffer);

                //Split the payload string into arguments
                std::string tempString;
                std::vector<std::string> args;
                for (int i = 0 ; i < int(payloadString.length()); i++){
                    char c = payloadString[i];
                    if(c == '/'){
                        args.push_back(tempString);
                        tempString = "";
                    }
                    else{
                        tempString.push_back(c);
                    }
                }

                switch(std::stoi(args[0])){
                    case 2:
                        //This request came from a MEC
                        if (inet_from == InetSocketAddress(m_mecIp, m_mecPort)){
                            int64_t delay = (m_requestSent - Simulator::Now()).GetMilliSeconds();
                            NS_LOG_INFO("Delay," << Simulator::Now() << "," << m_thisIpAddress << "," << from_ipv4 << "," << delay << "\n");
                        }
                        else{
                            Time sendTime;
                            for (std::map<Ipv4Address,Time>::iterator it = m_pingSent.begin(); it != m_pingSent.end(); ++it){
                                if((it->first) == from_ipv4){
                                    sendTime = it->second;
                                    break;
                                }
                            }

                            int64_t delay = (Simulator::Now() - sendTime).GetMilliSeconds();
                            m_measurementReport.insert(std::pair<Ipv4Address, int64_t>(from_ipv4, delay));

                            if(m_measurementReport.size() == m_allServers.size()){
                                //There is a measurement for each mec, i.e. the report is now complete and ready to be sent to the ORC
                                Simulator::Schedule(Seconds(0), &MecUeApplication::SendMeasurementReport, this);
                                m_measurementReport.clear(); //Start with an empty report for the next iteration


                            }
                        }
                        break;
                    case 6:
                        //Handover command
                        //Get new MEC address
                        std::string addressString = args[1];
                        std::string portString = args[2];

                        Ipv4Address newAddress = Ipv4Address();
                        newAddress.Set(addressString.c_str());

                        uint16_t newPort = std::stoi(portString);

                        //Update MEC address
                        NS_LOG_INFO("Handover," << Simulator::Now()<< "," << m_thisIpAddress << "," << m_mecIp << "," << newAddress << "\n");
                        m_mecIp = newAddress;
                        m_mecPort = newPort;
                        //Set current_server socket to new server address
                        currentMecSocket = serverSocketMap.find(InetSocketAddress(m_mecIp, m_mecPort))->second;
                        
                        //Set no-send period
                        m_noSendUntil = Simulator::Now() + Time(args[3]);
                        break;
                }
            }
        }
    }

} // Namespace ns3
