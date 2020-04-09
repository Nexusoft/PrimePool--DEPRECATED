#ifndef NEXUS_LLP_TYPES_H
#define NEXUS_LLP_TYPES_H

#include <string>
#include <vector>
#include <stdio.h>   
#include <thread>
#include <chrono>

#include "../util.h"
	
namespace LLP
{
	
	/** Event Enumeration. Used to determine each Event from LLP Server. **/
	enum 
	{
		EVENT_HEADER         = 0,
		EVENT_PACKET         = 1,
		EVENT_CONNECT        = 2,
		EVENT_DISCONNECT     = 3,
		EVENT_GENERIC        = 4
	};


	/** Type Definitions for LLP Functions **/
	//typedef boost::shared_ptr<boost::asio::ip::tcp::socket>      Socket_t;
	typedef std::thread                                        Thread_t;
	typedef std::mutex                                         Mutex_t;
	
	/** Sleep for a duration in Milliseconds. **/
	void Sleep(unsigned int nMilliseconds)
	{ 
		std::this_thread::sleep_for(std::chrono::milliseconds(nMilliseconds));
	}	

	///** Class to handle sending and receiving of LLP Packets. **/
	//class Packet
	//{
	//public:
	//	Packet() { SetNull(); }
	//
	//	/** Components of an LLP Packet.
	//		BYTE 0       : Header
	//		BYTE 1 - 5   : Length
	//		BYTE 6 - End : Data      **/
	//	unsigned char    HEADER;
	//	unsigned int     LENGTH;
	//	std::vector<unsigned char> DATA;
	//	
	//	
	//	/** Set the Packet Null Flags. **/
	//	inline void SetNull()
	//	{
	//		HEADER   = 255;
	//		LENGTH   = 0;
	//		
	//		DATA.clear();
	//	}
	//	
	//	
	//	/** Packet Null Flag. Header = 255. **/
	//	bool IsNull() { return (HEADER == 255); }
	//	
	//	
	//	/** Determine if a packet is fully read. **/
	//	bool Complete() { return (Header() && DATA.size() == LENGTH); }
	//	
	//	
	//	/** Determine if header is fully read **/
	//	bool Header() { return IsNull() ? false : (HEADER < 128 && LENGTH > 0) || (HEADER >= 128 && HEADER < 255 && LENGTH == 0); }
	//	
	//	
	//	/** Sets the size of the packet from Byte Vector. **/
	//	void SetLength(std::vector<unsigned char> BYTES) { LENGTH = (BYTES[0] << 24) + (BYTES[1] << 16) + (BYTES[2] << 8) + (BYTES[3] ); }
	//	
	//	
	//	/** Serializes class into a Byte Vector. Used to write Packet to Sockets. **/
	//	std::vector<unsigned char> GetBytes()
	//	{
	//		std::vector<unsigned char> BYTES(1, HEADER);
	//		
	//		/** Handle for Data Packets. **/
	//		if(HEADER < 128)
	//		{
	//			BYTES.push_back((LENGTH >> 24)); BYTES.push_back((LENGTH >> 16));
	//			BYTES.push_back((LENGTH >> 8));  BYTES.push_back(LENGTH);
	//			
	//			BYTES.insert(BYTES.end(),  DATA.begin(), DATA.end());
	//		}
	//		
	//		return BYTES;
	//	}
	//};
	//
	//

	///** Base Template class to handle outgoing / incoming LLP data for both Client and Server. **/
	//class Connection
	//{
	//protected:
	//	
	//	/** Basic Connection Variables. **/
	//	Timer         TIMER;
	//	Error_t       ERROR_HANDLE;
	//	Socket_t      SOCKET;
	//	
	//	
	//	/** 
	//		Virtual Event Function to be Overridden allowing Custom Read Events. 
	//		Each event fired on Header Complete, and each time data is read to fill packet.
	//		Useful to check Header length to maximum size of packet type for DDOS protection, 
	//		sending a keep-alive ping while downloading large files, etc.
	//		
	//		LENGTH == 0: General Events
	//		LENGTH  > 0 && PACKET: Read nSize Bytes into Data Packet
	//	**/
	//	virtual void Event(unsigned char EVENT, unsigned int LENGTH = 0){ }
	//	
	//	/** Virtual Process Function. To be overridden with your own custom packet processing. **/
	//	virtual bool ProcessPacket() { return true; }
	//public:
	//
	//
	//	/** Incoming Packet Being Built. **/
	//	Packet        INCOMING;
	//	
	//	
	//	/** DDOS Score if a Incoming Server Connection. **/
	//	DDOS_Filter*   DDOS;
	//	
	//	
	//	/** Connected Flag. **/
	//	bool CONNECTED;
	//	
	//	
	//	/** Flag to Determine if DDOS is Enabled. **/
	//	bool fDDOS;
	//	
	//	
	//	/** Connection Constructors **/
	//	Connection() : SOCKET(), DDOS(NULL), INCOMING(), CONNECTED(false), fDDOS(false) { INCOMING.SetNull(); }
	//	Connection( Socket_t SOCKET_IN, DDOS_Filter* DDOS_IN, bool isDDOS ) : SOCKET(SOCKET_IN), fDDOS(isDDOS), DDOS(DDOS_IN), INCOMING(), CONNECTED(false) { TIMER.Start(); }
	//	
	//	
	//	/** Checks for any flags in the Error Handle. **/
	//	bool Errors(){ return (ERROR_HANDLE == boost::asio::error::eof || ERROR_HANDLE); }
	//			
	//			
	//	/** Determines if nTime seconds have elapsed since last Read / Write. **/
	//	bool Timeout(unsigned int nTime){ return (TIMER.Elapsed() >= nTime); }
	//	
	//	
	//	/** Determines if Connected or Not. **/
	//	bool Connected(){ return CONNECTED; }
	//	
	//	
	//	/** Handles two types of packets, requests which are of header >= 128, and data which are of header < 128. **/
	//	bool PacketComplete(){ return INCOMING.Complete(); }
	//	
	//	
	//	/** Used to reset the packet to Null after it has been processed. This then flags the Connection to read another packet. **/
	//	void ResetPacket(){ INCOMING.SetNull(); }
	//	
	//	
	//	/** Write a single packet to the TCP stream. **/
	//	void WritePacket(Packet PACKET) { Write(PACKET.GetBytes()); }
	//	
	//	
	//	/** Non-Blocking Packet reader to build a packet from TCP Connection.
	//		This keeps thread from spending too much time for each Connection. **/
	//	void ReadPacket()
	//	{
	//			
	//		/** Handle Reading Packet Type Header. **/
	//		if(SOCKET->available() > 0 && INCOMING.IsNull())
	//		{
	//			std::vector<unsigned char> HEADER(1, 255);
	//			if(Read(HEADER, 1) == 1)
	//				INCOMING.HEADER = HEADER[0];
	//				
	//		}
	//			
	//		if(!INCOMING.IsNull() && !INCOMING.Complete())
	//		{
	//			/** Handle Reading Packet Length Header. **/
	//			if(SOCKET->available() >= 4 && INCOMING.LENGTH == 0)
	//			{
	//				std::vector<unsigned char> BYTES(4, 0);
	//				if(Read(BYTES, 4) == 4)
	//				{
	//					INCOMING.SetLength(BYTES);
	//					Event(EVENT_HEADER);
	//				}
	//			}
	//				
	//			/** Handle Reading Packet Data. **/
	//			unsigned int nAvailable = SOCKET->available();
	//			if(nAvailable > 0 && INCOMING.LENGTH > 0 && INCOMING.DATA.size() < INCOMING.LENGTH)
	//			{
	//				std::vector<unsigned char> DATA( std::min(nAvailable, (unsigned int)(INCOMING.LENGTH - INCOMING.DATA.size())), 0);
	//				unsigned int nRead = Read(DATA, DATA.size());
	//				
	//				if(nRead == DATA.size())
	//				{
	//					INCOMING.DATA.insert(INCOMING.DATA.end(), DATA.begin(), DATA.end());
	//					Event(EVENT_PACKET, nRead);
	//				}
	//			}
	//		}
	//	}
	//	
	//	
	//	/** Disconnect Socket. Cleans up memory usage to prevent "memory runs" from poor memory management. **/
	//	void Disconnect()
	//	{
	//		if(!CONNECTED)
	//			return;
	//			
	//		try
	//		{
	//			SOCKET -> shutdown(boost::asio::ip::tcp::socket::shutdown_both, ERROR_HANDLE);
	//			SOCKET -> close();
	//		}
	//		catch(...){}
	//		
	//		CONNECTED = false;
	//	}

	//	std::string GetIPAddress()
	//	{
	//		boost::system::error_code ec;
	//		return SOCKET->remote_endpoint(ec).address().to_string();
	//	}
	//	
	//	
	//private:
	//	
	//	/** Lower level network communications: Read. Interacts with OS sockets. **/
	//	size_t Read(std::vector<unsigned char> &DATA, size_t nBytes) { if(Errors()) return 0; TIMER.Reset(); return  boost::asio::read(*SOCKET, boost::asio::buffer(DATA, nBytes), ERROR_HANDLE); }
	//						
	//			
	//			
	//	/** Lower level network communications: Write. Interacts with OS sockets. **/
	//	void Write(std::vector<unsigned char> DATA) { if(Errors()) return; TIMER.Reset(); boost::asio::write(*SOCKET, boost::asio::buffer(DATA, DATA.size()), ERROR_HANDLE); }

	//};

	

}

#endif
	
	