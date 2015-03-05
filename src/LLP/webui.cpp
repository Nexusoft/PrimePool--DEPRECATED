#include "webui.h"
#include "../core.h"

#include "../LLD/record.h"

namespace LLP
{	

	/** Event Function to Customize Code For Inheriting Class Happening on the LLP Data Threads. **/
	void UiConnection::Event(unsigned char EVENT, unsigned int LENGTH)
	{	
	
		/** Handle any DDOS Packet Filters. **/
		if(EVENT == EVENT_HEADER)
		{
			return;
		}
		
		
		/** Handle for a Packet Data Read. **/
		if(EVENT == EVENT_PACKET)
		{
			return;
		}
		
		
		/** On Generic Event, Broadcast new block if flagged. **/
		if(EVENT == EVENT_GENERIC)
		{
			return;
		}
		
		/** On Connect Event, Assign the Proper Daemon Handle. **/
		if(EVENT == EVENT_CONNECT)
		{
			printf("[THREAD] New Connection Added to Thread\n");
			
			/** Check the White-listed IP's, and reject any connection not White Listed. **/
			if(DDOS)
			{
				return;
			}
			
			return;
		}
		
		/** On Disconnect Event, Reduce the Connection Count for Daemon **/
		if(EVENT == EVENT_DISCONNECT)
		{
			printf("[THREAD] Connection Removed from Thread\n");
			
			return;
		}
	}	
	
	/** This function is necessary for a template LLP server. It handles your 
		custom messaging system, and how to interpret it from raw packets. **/
	bool UiConnection::ProcessPacket()
	{
		Packet PACKET   = this->INCOMING;		
		printf("[SERVER] Received Command %u\n", PACKET.HEADER);
		
		/** Send the Front-End Information Regarding the Account. **/
		if(PACKET.HEADER == GET_ACCOUNT)
		{
			std::string strAddress = bytes2string(PACKET.DATA);
			printf("[SERVER] Received Request for Account Data: %s\n", strAddress.c_str());
			
			if(!Core::AccountDB.HasKey(strAddress))
			{
				this->WritePacket(GetPacket(FAILURE));
				return true;
			}
			
			Packet RESPONSE = GetPacket(ACCOUNT);
			
			LLD::Account cAccount = Core::AccountDB.GetRecord(strAddress);
			RESPONSE.DATA = cAccount.Serialize();
			RESPONSE.LENGTH = RESPONSE.DATA.size();
			
			printf("[SERVER] Sending Data for Account %s\n", strAddress.c_str());
			
			this->WritePacket(RESPONSE);
			
			return true;
		}
		
		
		/** Send the Front-End Information Regarding Block. **/
		if(PACKET.HEADER == GET_BLOCK)
		{
			uint1024 hashBlock;
			hashBlock.SetBytes(PACKET.DATA);
			
			if(!Core::BlockDB.HasKey(hashBlock))
			{
				this->WritePacket(GetPacket(FAILURE));
				return true;
			}
			
			LLD::Block cBlock = Core::BlockDB.GetRecord(hashBlock);
			Packet RESPONSE = GetPacket(BLOCK);
			RESPONSE.DATA = cBlock.Serialize();
			RESPONSE.LENGTH = RESPONSE.DATA.size();
			
			this->WritePacket(RESPONSE);
			
			return true;
		}
		
		
		/** Handle a Close from the Front-End. [Return False means to Disconnect Socket] **/
		if(PACKET.HEADER == CLOSE){ return false; }
			
		return false;
	}
}
