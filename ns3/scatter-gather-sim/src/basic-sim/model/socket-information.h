#ifndef SOCKET_INFORMATION_H
#define SOCKET_INFORMATION_H

#include "ns3/address.h"
#include "ns3/socket.h"

namespace ns3{

class Socket;
class Address;

/**
 * Contains a socket structure and its associated address to simplify distinguishing the origin of replies from different
 * workers.
 */
class SocketInformation {
public:
    Ptr<Socket> socket;
    Address address;

    SocketInformation (){}
};

}

#endif
