#ifndef INCAST_WORKER_H
#define INCAST_WORKER_H


#include "ns3/event-id.h"
#include "ns3/ptr.h"
#include "ns3/traced-callback.h"
#include "ns3/address.h"
#include "ns3/uinteger.h"
#include "ns3/socket.h"
#include "ns3/simulator.h"
#include "ns3/worker.h"

#include <unordered_map>

namespace ns3 {

class Address;
class Socket;
class Packet;
class Simulator;

/**
 * Worker that sends their data as soon as a request is received.
 */
class IncastWorker: public Worker {
public:
    static TypeId GetTypeId (void);
    /**
    * Set all associated frontends that should be served.
   */
    void SetFrontends(std::vector<Address> frontends);

    IncastWorker();
    virtual ~IncastWorker();
protected:
    // Number of bytes still pending per worker
    std::vector<int> left_to_send;

    /**
     * Sends data if available.
     */
    void SendIfData (Ptr<Socket> socket, uint32_t bytes);

    /**
     * After receiving a query, the worker prepares and sends
     */
    void SendResponse (int index);
    /**
     * When the worker receives a packet.
     */
    virtual void ReadPostProcess(Address from, Ptr<Packet> packet, uint8_t *buffer);
};
}

#endif
