#ifndef FIXED_FRONTEND_H
#define FIXED_FRONTEND_H

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
 * Batching Frontend class, it sends the requests by batches of predefined size.
 */
class FixedFrontend : public Frontend {
public:
    static TypeId GetTypeId (void);

    FixedFrontend();
    virtual ~FixedFrontend();

    /**
     * Sends as many requests as there are tokens available.
     */
    void SendingLoop();
protected:
    /**
     * Receives a response and "recovers" a token.
     */
    void ProcessResponse();

    // Maximum number of tokens available
    int64_t m_tokens;

    // Number of tokens currently used
    int64_t used_tokens;

    // Index of the last worker contacted
    int current_index;
};

}

#endif
