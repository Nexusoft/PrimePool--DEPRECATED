#include "outbound.h"
#include "socket.h"

namespace LLP
{
    /** Outgoing Client Connection Constructor **/
    Outbound::Outbound(std::string ip, std::string port, uint32_t nTimeout)
    : IP(ip)
    , PORT(port)
    , TIMEOUT(nTimeout)
    , Connection() { }

    bool Outbound::Connect()
    {
        try
        {
            uint16_t port = (uint16_t)atoi(PORT.c_str());
            CNetAddr ip(IP);
            CService addr(ip, port);
            SOCKET = std::make_shared<Socket>();
            SOCKET->Connect(addr, TIMEOUT);

            if(Errors())
            {
                Disconnect();

                printf("Failed to Connect to Mining LLP Server...\n");
                return false;
            }

            CONNECTED = true;
            TIMER.Start();

            printf("Connected to %s:%s...\n", IP.c_str(), PORT.c_str());

            return true;
        }
        catch(...) { }

        CONNECTED = false;
        return false;
    }

    Packet Outbound::ReadNextPacket(int nTimeout)
    {
        Packet NULL_PACKET;
        while(!PacketComplete())
        {
            if(Timeout(nTimeout) || Errors())
                return NULL_PACKET;

            ReadPacket();

        }

        return INCOMING;
    }

}
