#ifndef NEXUS_LLP_CONNECTION_H
#define NEXUS_LLP_CONNECTION_H

#include "timer.h"
#include "packet.h"
#include <memory>

namespace LLP
{
    /* forward declarations */
    class DDOS_Filter;
    class Socket;

    /** Base Template class to handle outgoing / incoming LLP data for both Client and Server. **/
    class Connection
    {
    protected:
        /** Incoming Packet Being Built. **/
        Packet        INCOMING;

        /** Basic Connection Variables. **/
        Timer                    TIMER;
        std::shared_ptr<Socket>  SOCKET;

        /** Connected Flag. **/
        bool CONNECTED;

        /**
          Virtual Event Function to be Overridden allowing Custom Read Events.
          Each event fired on Header Complete, and each time data is read to fill packet.
          Useful to check Header length to maximum size of packet type for DDOS protection,
          sending a keep-alive ping while downloading large files, etc.

          nSize == 0 : Header Is Complete for Data Packet
          nSize  > 0 : Read nSize Bytes into Data Packet
        **/
        virtual inline void Event(unsigned int nSize = 0) { }

    public:

        /** DDOS Score if a Incoming Server Connection. **/
        DDOS_Filter *DDOS;

        /** Connection Constructors **/
        Connection();

        Connection(std::shared_ptr<Socket> SOCKET_IN, DDOS_Filter* DDOS_IN);

        /** Checks for any flags in the Error Handle. **/
        bool Errors();

        /** Determines if nTime seconds have elapsed since last Read / Write. **/
        bool Timeout(unsigned int nTime);

        /** Flag to determine if TCP Connection is still alive. **/
        bool Connected();

        /** Handles two types of packets, requests which are of header >= 128, and data which are of header < 128. **/
        bool PacketComplete();

        /** Used to reset the packet to Null after it has been processed. This then flags the Connection to read another packet. **/
        void ResetPacket();

        /** Write a single packet to the TCP stream. **/
        void WritePacket(Packet PACKET);

        /** Non-Blocking Packet reader to build a packet from TCP Connection.
          This keeps thread from spending too much time for each Connection. **/
        void ReadPacket();

        /** Disconnect Socket. **/
        void Disconnect();

		/** Get the IP address from remote endpoint **/
		inline std::string GetRemoteIPAddress() const;

    private:

        /** Lower level network communications: Read. Interacts with OS sockets. **/
        size_t Read(std::vector<unsigned char> &DATA, size_t nBytes);

        /** Lower level network communications: Write. Interacts with OS sockets. **/
        void Write(std::vector<unsigned char> DATA);

    };
}

#endif
