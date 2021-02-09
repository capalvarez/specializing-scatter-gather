#ifndef WORKER_H
#define WORKER_H

#include "ns3/application.h"
#include "ns3/event-id.h"
#include "ns3/ptr.h"
#include "ns3/traced-callback.h"
#include "ns3/address.h"
#include "ns3/uinteger.h"
#include "ns3/socket.h"
#include "ns3/simulator.h"
#include "ns3/double.h"
#include "ns3/tcp-socket.h"
#include "ns3/socket-factory.h"
#include "ns3/tcp-socket-factory.h"
#include "ns3/log.h"

#include <unordered_map>

namespace ns3{
    class Address;
    class Socket;
    class Packet;
    class Simulator;

class Worker: public Application {
public:
    static TypeId GetTypeId (void);

    Worker();

    virtual ~Worker();

    /**
     * Set all associated frontends that should be served.
    */
    void SetFrontends(std::vector<Address> frontends);

    /**
     * Returns an address in string form.
     */
    std::string AddressToString(const Address& address);

protected:
    virtual void DoDispose(void);

    virtual void StartApplication (void);
    virtual void StopApplication ();

    void ConnectionSucceeded (Ptr<Socket> socket);
    void ConnectionFailed (Ptr<Socket> socket);

    void HandlePeerClose (Ptr<Socket> socket);
    void HandlePeerError (Ptr<Socket> socket);

    // List of all the sockets to the frontends
    std::vector<Ptr<Socket>> m_sockets;
    // List of all the frontedn addresses
    std::vector<Address> m_frontends;

    //Map relating frontend address and index
    std::unordered_map<std::string, int> frontend_indexes;

    TypeId m_tid;          // Protocol TypeId: TCP/UDP (leave available just in case)

    // Time that it takes processing the request
    int64_t m_processing_time;

    /**
     * When the worker receives a packet.
     */
    void HandleRead (Ptr<Socket> socket);
    virtual void ReadPostProcess(Address from, Ptr<Packet> packet, uint8_t *buffer);

    void HandleSend(Ptr<Socket> socket, uint32_t bytes);
    virtual void SendIfData(Ptr<Socket> socket, uint32_t bytes);

};
}




#endif
