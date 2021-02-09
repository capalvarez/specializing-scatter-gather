#ifndef INCAST_FRONTEND_H
#define INCAST_FRONTEND_H

#include <unordered_map>

#include "ns3/frontend.h"
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
 * Basic Frontend class, it sends the requests as fast as possible.
 */
class IncastFrontend : public Frontend {
public:
    static TypeId GetTypeId (void);

    IncastFrontend();
    virtual ~IncastFrontend();

    /**
     * Sends all the requests as the implemented algorithm allows.
     */
    void SendingLoop();
protected:
    /**
     * Logs a response after it has been received.
     */
    void ProcessResponse();
};

}
#endif