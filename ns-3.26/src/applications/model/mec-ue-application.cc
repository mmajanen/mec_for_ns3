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
#include <sstream>
#include <fstream>

namespace ns3 {

    NS_LOG_COMPONENT_DEFINE ("MecUeApplication");

//    NS_OBJECT_ENSURE_REGISTERED (MecUeApplication);
//
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
                               UintegerValue (100),
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
                               UintegerValue (),
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
                .AddAttribute ("PacketSize", "Size of echo data in outbound packets",
                               UintegerValue (100),
                               MakeUintegerAccessor (&MecUeApplication::m_packetSize),
                               MakeUintegerChecker<uint32_t> ())
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
        m_socket = 0;
        m_sendPingEvent = EventId ();
        m_sendServiceEvent = EventId ();
        m_data_request = 0;
        m_data_ping = 0;
        m_data_report = 0;
        m_thisNode = GetNode();
        m_requestBlocked = false;
        m_thisNetDevice = m_thisNode->GetDevice(0);

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
            m_allServers.push_back(ipv4);
        }
        m_enbDevices.push_back(m_enb0);
        m_enbDevices.push_back(m_enb1);
        m_enbDevices.push_back(m_enb2);
    }

    MecUeApplication::~MecUeApplication()
    {
        NS_LOG_FUNCTION (this);
        m_socket = 0;

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
        InetSocketAddress m_mecAddress = InetSocketAddress(m_mecIp, m_mecPort);

        if (m_socket == 0)
        {
            TypeId tid = TypeId::LookupByName ("ns3::UdpSocketFactory");
            m_socket = Socket::CreateSocket (GetNode (), tid);
            if (Ipv4Address::IsMatchingType(m_mecAddress) == true)
            {
                m_socket->Bind();
                m_socket->Connect (m_mecAddress);
            }
            else if (InetSocketAddress::IsMatchingType (m_mecAddress) == true)
            {
                m_socket->Bind ();
                m_socket->Connect (m_mecAddress);
            }
            else
            {
                NS_ASSERT_MSG (false, "Incompatible address type: " << m_mecAddress);
            }
        }

        m_socket->SetRecvCallback (MakeCallback (&MecUeApplication::HandleRead, this));
        m_socket->SetAllowBroadcast (true);
        SendServiceRequest ();
        SendPing ();
    }

    void
    MecUeApplication::StopApplication ()
    {
        NS_LOG_FUNCTION (this);

        if (m_socket != 0)
        {
            m_socket->Close ();
            m_socket->SetRecvCallback (MakeNullCallback<void, Ptr<Socket> > ());
            m_socket = 0;
        }

        Simulator::Cancel (m_sendPingEvent);
        Simulator::Cancel (m_sendServiceEvent);
    }

    uint8_t*
    MecUeApplication::GetFilledString (std::string filler) {
        //dest can either be m_data_request or m_data_ping or m_data_report
        NS_LOG_FUNCTION(this << filler);

        std::string result;
        uint8_t *val = (uint8_t *) malloc(m_packetSize + 1);

        int fillSize = filler.size();

        if (fillSize >= int(m_packetSize)) {
            NS_LOG_ERROR("Filler for packet larger than packet size");
            StopApplication();
        } else {
            result.append(filler);
            for (int i = result.size(); i < int(m_packetSize); i++) {
                result.append("#");
            }

            std::memset(val, 0, m_packetSize + 1);
            std::memcpy(val, result.c_str(), m_packetSize + 1);
        }
        return val;
    }


    uint16_t MecUeApplication::GetCellId(){
        uint16_t result = -1;

        LteUeNetDevice * pLteDevice;
        Ptr<NetDevice> thisDevice = (m_thisNode->GetDevice(0));
        pLteDevice = (LteUeNetDevice*) &thisDevice;
        uint64_t ueImsi = pLteDevice->GetImsi();

        //TODO fix unreachable code!
        bool found = false;
        for(Ptr<LteEnbNetDevice> enb: m_enbDevices){
            if (!found) {
                Ptr<LteEnbRrc> rrc = enb->GetRrc();
                std::map<uint16_t, Ptr<UeManager>> ueMap = rrc->GetUeMap();
                for(std::map<uint16_t, Ptr<UeManager>>::iterator it = ueMap.begin(); (it != ueMap.end() && !found); ++it){
                    Ptr<UeManager> manager = it->second;
                    uint64_t imsi = manager->GetImsi();
                    if(ueImsi == imsi){
                        result = enb->GetCellId();
                        found = true;
                    }
                }
            }
            else{
                break;
            }

        }
        return result;
    }

    void
    MecUeApplication::SendServiceRequest (void) {
        NS_ASSERT (m_sendServiceEvent.IsExpired ());

        m_socket->Bind();
        m_socket->Connect (InetSocketAddress(m_mecIp, m_mecPort));

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
            uint8_t *buffer = GetFilledString(fillString);

            //Create packet
            Ptr<Packet> p = Create<Packet> (buffer, m_packetSize);
            // call to the trace sinks before the packet is actually sent,
            // so that tags added to the packet can be sent as well
            m_txTrace (p);
            m_socket->Send (p);

            ++m_sent;

            if (m_sent < m_count)
            {
                m_sendServiceEvent = Simulator::Schedule (m_serviceInterval, &MecUeApplication::SendServiceRequest, this);
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
                m_socket->Bind();
                m_socket->Connect(mec);


                //Create packet payload
                std::string fillString = "1/";
                fillString.append(std::to_string(GetCellId()) + "/");
                uint8_t *buffer = GetFilledString(fillString);

                //Create packet
                Ptr <Packet> p = Create<Packet>(buffer, m_packetSize);
                // call to the trace sinks before the packet is actually sent,
                // so that tags added to the packet can be sent as well
                m_txTrace(p);
                m_socket->Send(p);

                ++m_sent;

                if (m_sent < m_count) {
                    m_sendServiceEvent = Simulator::Schedule(m_serviceInterval, &MecUeApplication::SendServiceRequest, this);
                }

                m_requestBlocked = false;
            }
        }
    }

    void
    MecUeApplication::SendMeasurementReport (void){


        //Bind to ORC address
        m_socket->Bind();
        m_socket->Connect (InetSocketAddress(m_orcIp, m_orcPort));

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

        uint8_t *buffer = GetFilledString(payload);

        //Create packet
        Ptr<Packet> p = Create<Packet> (buffer, m_packetSize);
        // call to the trace sinks before the packet is actually sent,
        // so that tags added to the packet can be sent as well
        m_txTrace (p);
        m_socket->Send (p);

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
                        m_socket->Bind();
                        m_socket->Connect (InetSocketAddress(m_mecIp, m_mecPort));

                        //Set no-send period
                        m_noSendUntil = Simulator::Now() + Time(args[3]);
                        break;
                }
            }
        }
    }

} // Namespace ns3
