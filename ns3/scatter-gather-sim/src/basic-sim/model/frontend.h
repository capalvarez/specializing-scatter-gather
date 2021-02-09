#ifndef FRONTEND_H
#define FRONTEND_H

#include <unordered_map>

#include "ns3/application.h"
#include "ns3/event-id.h"
#include "ns3/ptr.h"
#include "ns3/traced-callback.h"
#include "ns3/address.h"
#include "ns3/socket.h"
#include "ns3/uinteger.h"
#include "ns3/socket-information.h"
#include "ns3/incast-timestamp.h"
#include "ns3/simulator.h"

namespace ns3 {

class Address;
class Socket;
class Packet;
class Simulator;


/**
 * Generic Frontend class with the basic behaviour for all concrete implementations.
 */
class Frontend : public Application {
public:
    static TypeId GetTypeId(void);

    Frontend();

    virtual ~Frontend();

    std::unordered_map <std::string, IncastTimestamp> getTimestamps();

    void SendRequest(SocketInformation socket, Ptr <Packet> packet);

protected:
    virtual void DoDispose(void);

    /**
     * Sends as many requests as the implemented algorithm allows. Must be implemented by subclasses.
     */
    virtual void SendingLoop();

    /**
     * Process a response after it has been received. Must be implemented by subclasses.
     */
    virtual void ProcessResponse();

    /**
     * Returns an address in string form.
     */
    std::string AddressToString(const Address &address);

    virtual void StartApplication(void);    // Called at time specified by Start
    virtual void StopApplication(void);     // Called at time specified by Stop

    TypeId m_tid;
    Address m_local;

    // All sockets connected to the workers
    Ptr <Socket> m_listen;
    // List of the network information related to the workers
    std::vector <SocketInformation> m_workerSockets;
    // Map with the timestamps of all events
    std::unordered_map <std::string, IncastTimestamp> timestamps;

    // Scatter-gather parameters: number of bytes to query, number of workers, number of worker connected, and the time
    // between each request (simulating processing time).
    uint32_t m_payloadSize;
    uint32_t m_n_workers;
    uint32_t connected_workers;
    uint32_t m_inter_request_spacing;

    bool AcceptConnection(Ptr<Socket> socket, const Address& from);
    virtual void HandleAccept (Ptr<Socket> socket, const Address& from);

    /**
     * Starts the process of sending the requests.
     */
    void SendRequests();
    virtual void HandleRead (Ptr<Socket> socket);

    void SocketClosedNormal(Ptr<Socket> socket);
    void SocketClosedError(Ptr<Socket> socket);
};

}


#endif
