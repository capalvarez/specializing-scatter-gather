#include "frontend.h"

#include <sstream>

#include "ns3/tcp-socket-factory.h"
#include "ns3/log.h"
#include "ns3/object-vector.h"
#include "ns3/address.h"


namespace ns3{
    NS_LOG_COMPONENT_DEFINE ("Frontend");

    NS_OBJECT_ENSURE_REGISTERED (Frontend);

    TypeId Frontend::GetTypeId (void) {
        static TypeId tid = TypeId ("ns3::Frontend")
                .SetParent<Application> ()
                .SetGroupName("Applications")
                .AddConstructor<Frontend> ()
                .AddAttribute ("Protocol",
                               "The type id of the protocol to use for the rx socket.",
                               TypeIdValue (TcpSocketFactory::GetTypeId ()),
                               MakeTypeIdAccessor (&Frontend::m_tid),
                               MakeTypeIdChecker ())
                .AddAttribute ("Local",
                               "Local address.",
                               AddressValue (),
                               MakeAddressAccessor (&Frontend::m_local),
                               MakeAddressChecker ())
                .AddAttribute ("PayloadSize",
                               "Number of bytes to request from each worker.",
                               UintegerValue (0),
                               MakeUintegerAccessor (&Frontend::m_payloadSize),
                               MakeUintegerChecker<uint32_t> ())
                .AddAttribute ("Workers",
                               "Number of workers that will connect to the frontend.",
                               UintegerValue (0),
                               MakeUintegerAccessor (&Frontend::m_n_workers),
                               MakeUintegerChecker<uint32_t> ())
                .AddAttribute ("RequestSpacing",
                               "Time between sending two requests (in microseconds).",
                               UintegerValue (1),
                               MakeUintegerAccessor (&Frontend::m_inter_request_spacing),
                               MakeUintegerChecker<uint32_t> ())
        ;
        return tid;
    }

    Frontend::Frontend ()
    {
        NS_LOG_FUNCTION (this);
        m_listen = 0;
        connected_workers = 0;
    }

    Frontend::~Frontend () {
        NS_LOG_FUNCTION(this);
        m_listen = 0;
        connected_workers = 0;
    }

    std::unordered_map<std::string, IncastTimestamp> Frontend::getTimestamps(){
        return timestamps;
    }

    std::string Frontend::AddressToString(const Address& address){
        InetSocketAddress socketAddress = InetSocketAddress::ConvertFrom(address);
        std::stringstream msg;
        msg << socketAddress.GetIpv4() << ":" << socketAddress.GetPort();

        return msg.str();
    }

    void Frontend::SendRequest(SocketInformation socket, Ptr<Packet> packet){
        IncastTimestamp& timestampInformation = timestamps[AddressToString(socket.address)];
        timestampInformation.sentRequest = Simulator::Now().GetNanoSeconds();

        socket.socket->Send (packet);
    }

    void Frontend::DoDispose (void)
    {
        NS_LOG_FUNCTION (this);
        m_listen = 0;
        connected_workers = 0;

        Application::DoDispose ();
    }

    void Frontend::SendingLoop( ){
        NS_LOG_FUNCTION (this);
    }

    void Frontend::ProcessResponse(){
        NS_LOG_FUNCTION (this);
    }

    void Frontend::StartApplication (void){
        NS_LOG_FUNCTION (this);

        if (!m_listen) {
            m_listen = Socket::CreateSocket (GetNode (), m_tid);

            if (m_listen->Bind (m_local) == -1) {
                NS_FATAL_ERROR ("Failed to bind socket");
            }

            m_listen->Listen ();
        }

        m_listen->SetAcceptCallback (
                MakeCallback (&Frontend::AcceptConnection, this),
                MakeCallback (&Frontend::HandleAccept, this));

        m_listen->SetCloseCallbacks (
                MakeCallback (&Frontend::SocketClosedNormal, this),
                MakeCallback (&Frontend::SocketClosedError, this));
    }

    void Frontend::StopApplication (void){
        NS_LOG_FUNCTION (this);
    }

    bool Frontend::AcceptConnection(Ptr<Socket> socket, const Address& from){
        NS_LOG_FUNCTION (this << socket);
        return true;
    }

    void Frontend::HandleAccept (Ptr<Socket> s, const Address& from){
        NS_LOG_FUNCTION (this << s << from);

        s->SetRecvCallback (MakeCallback (&Frontend::HandleRead, this));
        SocketInformation socketInformation;
        socketInformation.socket = s;
        socketInformation.address = from;

        IncastTimestamp newTimestamp;

        newTimestamp.worker_index = m_workerSockets.size();
        timestamps[AddressToString(from)] = newTimestamp;

        m_workerSockets.push_back (socketInformation);

        connected_workers++;
    }

    void Frontend::SendRequests() {
        NS_LOG_FUNCTION(this);

        SendingLoop();
    }

    void Frontend::HandleRead(Ptr<Socket> socket){
        NS_LOG_FUNCTION (this);

        Ptr<Packet> packet;
        Address from;
        Address localAddress;

        while((packet = socket->RecvFrom (from))){
            if (packet->GetSize () == 0)
            { //EOF
                break;
            }

            std::string stringAddress = AddressToString(from);
            IncastTimestamp& timestampInformation = timestamps[stringAddress];

            if (timestampInformation.bytes == 0) {
                timestampInformation.firstByte = Simulator::Now().GetNanoSeconds();
            }

            timestampInformation.bytes += packet->GetSize();

            if (timestampInformation.bytes == m_payloadSize) {
                timestampInformation.lastByte = Simulator::Now().GetNanoSeconds();

                ProcessResponse();
            }
        }
    }

    void Frontend::SocketClosedNormal(Ptr<Socket> socket){
        NS_LOG_FUNCTION (this);
    }

    void Frontend::SocketClosedError(Ptr<Socket> socket){
        NS_LOG_FUNCTION (this);
    }

}