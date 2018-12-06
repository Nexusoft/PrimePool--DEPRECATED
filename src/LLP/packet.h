#ifndef NEXUS_LLP_PACKET_H
#define NEXUS_LLP_PACKET_H

#include <vector>

namespace LLP
{
    /** Class to handle sending and receiving of LLP Packets. **/
    class Packet
    {
    public:
        Packet();

        /** Components of an LLP Packet.
            BYTE 0       : Header
            BYTE 1 - 5   : Length
            BYTE 6 - End : Data      **/
        unsigned char    HEADER;
        unsigned int     LENGTH;
        std::vector<unsigned char> DATA;

        /** Set the Packet Null Flags. **/
        void SetNull();

        /** Packet Null Flag. Header = 255. **/
        bool IsNull();

        /** Determine if a packet is fully read. **/
        bool Complete();

        /** Determine if header is fully read **/
        bool Header();

        /** Sets the size of the packet from Byte Vector. **/
        void SetLength(std::vector<unsigned char> BYTES);

        /** Serializes class into a Byte Vector. Used to write Packet to Sockets. **/
        std::vector<unsigned char> GetBytes();
    };

}

#endif
