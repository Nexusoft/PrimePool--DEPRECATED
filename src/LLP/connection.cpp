#include "connection.h"
#include "ddos.h"
#include "socket.h"
#include <algorithm>

namespace LLP
{
    /** Connection Constructors **/
    Connection::Connection() : SOCKET(), DDOS(NULL), INCOMING(), CONNECTED(false)
    {
        INCOMING.SetNull();
    }

    Connection::Connection(std::shared_ptr<Socket> SOCKET_IN, DDOS_Filter* DDOS_IN)
        : SOCKET(SOCKET_IN)
        , DDOS(DDOS_IN)
        , INCOMING()
        , CONNECTED(true)
    {
      TIMER.Start();
    }

    /** Checks for any flags in the Error Handle. **/
    bool Connection::Errors()
    {
        return SOCKET->Error() != 0;
    }

    /** Determines if nTime seconds have elapsed since last Read / Write. **/
    bool Connection::Timeout(unsigned int nTime)
    {
        return (TIMER.Elapsed() >= nTime);
    }

    /** Flag to determine if TCP Connection is still alive. **/
    bool Connection::Connected()
    {
        return CONNECTED;
    }

    /** Handles two types of packets, requests which are of header >= 128, and data which are of header < 128. **/
    bool Connection::PacketComplete()
    {
        return INCOMING.Complete();
    }

    /** Used to reset the packet to Null after it has been processed. This then flags the Connection to read another packet. **/
    void Connection::ResetPacket()
    {
        INCOMING.SetNull();
    }

    /** Write a single packet to the TCP stream. **/
    void Connection::WritePacket(Packet PACKET)
    {
        if (Errors())
            return;

        Write(PACKET.GetBytes());
    }

    /** Non-Blocking Packet reader to build a packet from TCP Connection.
      This keeps thread from spending too much time for each Connection. **/
    void Connection::ReadPacket()
    {
        /** Handle Reading Packet Type Header. **/
        if (SOCKET->Available() >= 1 && INCOMING.IsNull())
        {
            std::vector<uint8_t> HEADER(1, 255);
            if (Read(HEADER, 1) == 1)
                INCOMING.HEADER = HEADER[0];

        }

        if (!INCOMING.IsNull() && !INCOMING.Complete())
        {

            /** Handle Reading Packet Length Header. **/
            if (SOCKET->Available() >= 4 && INCOMING.LENGTH == 0)
            {
                std::vector<uint8_t> BYTES(4, 0);
                if (Read(BYTES, 4) == 4)
                {
                    INCOMING.SetLength(BYTES);
                    Event(0);
                }
            }

            /** Handle Reading Packet Data. **/
            uint32_t nAvailable = SOCKET->Available();
            if (nAvailable > 0 && INCOMING.LENGTH > 0 && INCOMING.DATA.size() < INCOMING.LENGTH)
            {
                std::vector<uint8_t> DATA( std::min( std::min(nAvailable, 512u), (uint32_t)(INCOMING.LENGTH - INCOMING.DATA.size())), 0);

                if (Read(DATA, DATA.size()) == DATA.size())
                {
                    INCOMING.DATA.insert(INCOMING.DATA.end(), DATA.begin(), DATA.end());
                    //Event(EVENT_PACKET, DATA.size());
                }
            }
        }
    }

    /** Disconnect Socket. **/
    void Connection::Disconnect()
    {
        SOCKET->Close();

        CONNECTED = false;
    }

    /** Lower level network communications: Read. Interacts with OS sockets. **/
    size_t Connection::Read(std::vector<uint8_t> &DATA, size_t nBytes)
    {
        TIMER.Reset();

        return SOCKET->Read(DATA, nBytes);
    }

    /** Lower level network communications: Write. Interacts with OS sockets. **/
    void Connection::Write(std::vector<uint8_t> DATA)
    {
        SOCKET->Write(DATA, DATA.size());
    }

	std::string Connection::GetRemoteIPAddress() const
	{ 
		SOCKET->addr.ToStringIP(); 
	}
}
