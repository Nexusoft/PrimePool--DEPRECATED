#ifndef COINSHIELD_POOLSERVER_H
#define COINSHIELD_POOLSERVER_H

#include <stack>

#include "types.h"
#include "block.h"
#include "../hash/uint1024.h"

namespace LLP
{
	/** Forward Declarations. **/
	class DaemonHandle;
	
	
	/** Template Connection to be used by LLP Server. **/
	class PoolConnection : public Connection
	{	
		std::map<uint1024, CBlock::Sptr> MAP_BLOCKS;
		std::stack<CBlock::Uptr> NEW_BLOCKS;
	
		DaemonHandle* DAEMON;
		
		Timer BLOCK_TIMER;
		bool fLoggedIn = false;
		
		enum
		{
			/** DATA PACKETS **/
			LOGIN            = 0,
			BLOCK_DATA       = 1,
			SUBMIT_SHARE     = 2,
			ACCOUNT_BALANCE  = 3,
			PENDING_PAYOUT   = 4,
			SUBMIT_PPS     	 = 5,
					
			/** REQUEST PACKETS **/
			GET_BLOCK    = 129,
			NEW_BLOCK    = 130,
			GET_BALANCE  = 131,
			GET_PAYOUT   = 132,
			
			
			/** RESPONSE PACKETS **/
			ACCEPT     = 200,
			REJECT     = 201,
			BLOCK      = 202,
			STALE      = 203,
			
					
			/** GENERIC **/
			PING     = 253,
			CLOSE    = 254
		};
		
		
		/** Clear the Map Data from Connection. **/
		void Clear();
	
	public:
		boost::mutex             BLOCK_MUTEX;
		
		PoolConnection() : Connection(), BLOCK_TIMER() {}
		PoolConnection( Socket_t SOCKET_IN, DDOS_Filter* DDOS_IN, bool isDDOS) : Connection( SOCKET_IN, DDOS_IN, isDDOS ), BLOCK_TIMER() {}
		
		~PoolConnection()
		{				
			MAP_BLOCKS.clear();
		}
		
		/** Total Block Requests to tell the Daemon to Send some Blocks to the Stack. **/
		unsigned int nBlockRequests = 0, nBlocksWaiting = 0, nID = 0;
		
		/** Flag to tell Pool Server to Broadcast a new Block. **/
		bool fNewBlock = false;
		
		/** Address of Account Logged in on this Connection. **/
		std::string ADDRESS;
		
		/** Unique ID of this connection within the server.  Composite of Daemon ID and connection ID **/
		std::string GUID;

		/** Block Set by Connection if it is Above Difficulty. **/
		CBlock::Sptr SUBMISSION_BLOCK = nullptr;
		
		/** Mutex for Modifying Submission Block Pointer. **/
		boost::mutex  SUBMISSION_MUTEX;
		
		/** Add a Block to the Pool Connection Stacks. **/
		void AddBlock(CBlock::Uptr BLOCK);
		
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
		
		/** Convert the Header of a Block into a Byte Stream for Reading and Writing Across Sockets. **/
		inline std::vector<unsigned char> SerializeBlock(CBlock::Sptr BLOCK);
	};
}

#endif
