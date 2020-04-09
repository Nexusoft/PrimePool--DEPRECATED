#ifndef NEXUS_LLP_OUTBOUND_H
#define NEXUS_LLP_OUTBOUND_H

#include "connection.h"

namespace LLP
{
    class Outbound : public Connection
    {
    private:
      std::string IP, PORT;
      uint32_t TIMEOUT;

    public:
      /** Outgoing Client Connection Constructor **/
      Outbound(std::string ip, std::string port, uint32_t nTimeOut = 10);

      bool Connect();

      Packet ReadNextPacket(int nTimeout = 10);

    };
}

#endif
