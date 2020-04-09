#include "packet.h"

namespace LLP
{
    Packet::Packet()
    {
        SetNull();
    }

    /** Set the Packet Null Flags. **/
    void Packet::SetNull()
    {
        HEADER = 255;
        LENGTH = 0;

        DATA.clear();
    }

    /** Packet Null Flag. Header = 255. **/
    bool Packet::IsNull()
    {
        return (HEADER == 255);
    }

    /** Determine if a packet is fully read. **/
    bool Packet::Complete()
    {
        return (Header() && DATA.size() == LENGTH);
    }

    /** Determine if header is fully read **/
    bool Packet::Header()
    {
        return IsNull() ? false : (HEADER < 128 && LENGTH > 0) ||
                                  (HEADER >= 128 && HEADER < 255 && LENGTH == 0);
    }

    /** Sets the size of the packet from Byte Vector. **/
    void Packet::SetLength(std::vector<unsigned char> BYTES)
    {
        LENGTH = (BYTES[0] << 24) + (BYTES[1] << 16) + (BYTES[2] << 8) + (BYTES[3]);
    }

    /** Serializes class into a Byte Vector. Used to write Packet to Sockets. **/
    std::vector<unsigned char> Packet::GetBytes()
    {
        std::vector<unsigned char> BYTES(1, HEADER);

        /** Handle for Data Packets. **/
        if (HEADER < 128)
        {
            BYTES.push_back((LENGTH >> 24)); BYTES.push_back((LENGTH >> 16));
            BYTES.push_back((LENGTH >> 8));  BYTES.push_back(LENGTH);

            BYTES.insert(BYTES.end(), DATA.begin(), DATA.end());
        }

        return BYTES;
    }
}
