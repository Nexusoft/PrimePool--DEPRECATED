#ifndef COINSHIELD_WEBUI_H
#define COINSHIELD_WEBUI_H

#include "types.h"
#include "connection.h"

namespace LLP
{
	
	
	/** Template Connection to be used by LLP Server. **/
	class UiConnection : public Connection
	{	
		
		enum
		{
			/** DATA PACKETS **/
			GET_ACCOUNT        = 0,
			GET_BLOCK          = 1,
			BLOCK_LIST         = 2,
			POOL_WEIGHT        = 3,
			ACCOUNT            = 4,
			BLOCK              = 5,
					
					
			/** REQUEST PACKETS **/
			GET_BLOCK_LIST   = 129,
			GET_POOL_WEIGHT  = 130,
			
			
			/** RESPONSE PACKETS. **/
			FAILURE          = 200,
			
					
			/** GENERIC **/
			CLOSE    = 254
		};
		
		
		/** Clear the Map Data from Connection. **/
		void Clear();
	
	public:
		UiConnection() : Connection() {}
//		UiConnection( Socket_t SOCKET_IN, DDOS_Filter* DDOS_IN, bool isDDOS) : Connection( SOCKET_IN, DDOS_IN, isDDOS ) {}
		
		/** Event Function to Customize Code For Inheriting Class Happening on the LLP Data Threads. **/
		void Event(unsigned char EVENT, unsigned int LENGTH = 0);
		
		/** This function is necessary for a template LLP server. It handles your 
			custom messaging system, and how to interpret it from raw packets. **/
		bool ProcessPacket();
		
	private:
	
		inline Packet GetPacket(unsigned char HEADER)
		{
			Packet PACKET;
			PACKET.HEADER = HEADER;
			
			return PACKET;
		}
	
		inline void Respond(unsigned char HEADER) { this->WritePacket(GetPacket(HEADER)); }
		
	};
}

#endif
