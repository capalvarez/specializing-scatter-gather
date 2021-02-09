#include "worker.h"

namespace ns3 {
    NS_LOG_COMPONENT_DEFINE ("Worker");

    NS_OBJECT_ENSURE_REGISTERED (Worker);

TypeId Worker::GetTypeId (void) {
    static TypeId tid = TypeId ("ns3::Worker")
            .SetParent<Application> ()
            .SetGroupName("Applications")
            .AddConstructor<Worker> ()
            .AddAttribute ("Protocol",
                           "The type id of the protocol to use for the rx socket.",
                           TypeIdValue (TcpSocketFactory::GetTypeId ()),
                           MakeTypeIdAccessor (&Worker::m_tid),
                           MakeTypeIdChecker ())
            .AddAttribute ("ProcessingTime",
                           "Time necessary to create a response (in microseconds).",
                           UintegerValue (0),
                           MakeUintegerAccessor (&Worker::m_processing_time),
                           MakeUintegerChecker<uint32_t> ())
    ;
    return tid;
}

Worker::Worker ()
{
    NS_LOG_FUNCTION (this);
}

Worker::~Worker () {
    NS_LOG_FUNCTION(this);
}

std::string Worker::AddressToString(const Address& address){
    InetSocketAddress socketAddress = InetSocketAddress::ConvertFrom(address);
    std::stringstream msg;
    msg << socketAddress.GetIpv4() << ":" << socketAddress.GetPort();

    return msg.str();
}

void Worker::SetFrontends(std::vector<Address> frontends){
    m_frontends.assign(frontends.begin(), frontends.end());
    m_sockets = std::vector<Ptr<Socket>>(m_frontends.size());
}

void Worker::StartApplication (void){
    NS_LOG_FUNCTION (this);

    for (int i = 0; i < (int) m_frontends.size(); ++i) {
        if (!m_sockets[i]) {
            m_sockets[i] = Socket::CreateSocket (GetNode (), m_tid);
            m_sockets[i]->Connect (m_frontends[i]);

            frontend_indexes[AddressToString(m_frontends[i])] = i;

            m_sockets[i]->SetConnectCallback (
                    MakeCallback (&Worker::ConnectionSucceeded, this),
                    MakeCallback (&Worker::ConnectionFailed, this));

            m_sockets[i]->SetRecvCallback (MakeCallback (&Worker::HandleRead, this));

            m_sockets[i]->SetSendCallback (
                    MakeCallback (&Worker::HandleSend, this));

            m_sockets[i]->SetCloseCallbacks (
                    MakeCallback (&Worker::HandlePeerClose, this),
                    MakeCallback (&Worker::HandlePeerError, this));
        }
    }
}

void Worker::StopApplication () {
    NS_LOG_FUNCTION (this);
    for (int i = 0; i < (int) m_frontends.size(); ++i) {
        if (m_sockets[i])
        {
            m_sockets[i]->Close ();
            m_sockets[i]->SetRecvCallback (MakeNullCallback<void, Ptr<Socket> > ());
        }
    }
}

void Worker::DoDispose (void)
{
    NS_LOG_FUNCTION (this);
    Application::DoDispose ();
}


void Worker::SendIfData(Ptr<Socket> socket, uint32_t bytes){
    NS_LOG_FUNCTION (this);
}

void Worker::ReadPostProcess(Address from, Ptr<Packet> packet, uint8_t *buffer){
    NS_LOG_FUNCTION (this);
}

void Worker::HandleRead (Ptr<Socket> socket){
    NS_LOG_FUNCTION (this << socket);

    Ptr<Packet> packet;
    Address from;
    Address localAddress;

    // Ideally, you would get just 1 packet, but it's better to be on the safe side
    while ((packet = socket->RecvFrom (from))) {
        if (packet->GetSize () == 0) { //EOF
            break;
        }

        // Receive information from the packet
        uint8_t *buffer = new uint8_t[packet->GetSize ()];
        packet->CopyData(buffer, packet->GetSize ());

        ReadPostProcess(from, packet, buffer);
    }
}

void Worker::HandleSend(Ptr<Socket> socket, uint32_t bytes){
    SendIfData(socket, bytes);
}

void Worker::ConnectionSucceeded (Ptr<Socket> socket){
    NS_LOG_FUNCTION (this << socket);
}

void Worker::ConnectionFailed (Ptr<Socket> socket){
    NS_LOG_FUNCTION (this << socket);
}

void Worker::HandlePeerClose (Ptr<Socket> socket)
{
    NS_LOG_FUNCTION (this << socket);
}

void Worker::HandlePeerError (Ptr<Socket> socket)
{
    NS_LOG_FUNCTION (this << socket);
}

}