#ifndef NEXUS_DAEMON_H
#define NEXUS_DAEMON_H

#include "types.h"
#include "block.h"
#include "connection.h"
#include "network.h"
#include "socket.h"
#include "../hash/uint1024.h"

#include <mutex>

class Coinbase;
namespace Core { class CBlock; }
namespace LLP
{

	/** Forward Declartion of Pool Connection. **/
	class PoolConnection;
	
	
	/** Daemon Connection Class. Handles all the Reading / Writing from the Daemon Server. **/
	class DaemonConnection : public Connection
	{
		std::string IP, PORT;
		uint32_t TIMEOUT;
		
	public:
		DaemonConnection(std::string ip, std::string port, uint32_t timeout = 10) 
			: IP{ip}
			, PORT{port}
			, TIMEOUT{timeout}		
			{}
		
		/** Enumeration to interpret Daemon Packets. **/
		enum
		{
			/** DATA PACKETS **/
			BLOCK_DATA   = 0,
			SUBMIT_BLOCK = 1,
			BLOCK_HEIGHT = 2,
			SET_CHANNEL  = 3,
			BLOCK_REWARD = 4,
			SET_COINBASE = 5,
			GOOD_BLOCK   = 6,
			ORPHAN_BLOCK = 7,
			
			
			/** DATA REQUESTS **/
			CHECK_BLOCK  = 64,
			
					
					
			/** REQUEST PACKETS **/
			GET_BLOCK    = 129,
			GET_HEIGHT   = 130,
			GET_REWARD   = 131,
			
			
			/** SERVER COMMANDS **/
			CLEAR_MAP    = 132,
			GET_ROUND    = 133,
			
			
			/** RESPONSE PACKETS **/
			GOOD       = 200,
			FAIL       = 201,

			
			/** VALIDATION RESPONSES **/
			COINBASE_SET  = 202,
			COINBASE_FAIL = 203,
			
			NEW_ROUND     = 204,
			OLD_ROUND     = 205,
					
					
			/** GENERIC **/
			PING     = 253,
			CLOSE    = 254
		};
		
		
		/** Current Newly Read Packet Access. **/
		inline Packet NewPacket() { return this->INCOMING; }
		
		
		/** Create a Packet with the Given Header. **/
		inline Packet GetPacket(unsigned char HEADER)
		{
			Packet PACKET;
			PACKET.HEADER = HEADER;
			
			return PACKET;
		}
		
		
		/** Get a new Block from the Daemon Server. **/
		inline void GetBlock()  { this -> WritePacket(GetPacket(GET_BLOCK));  }
		
		
		/** Check the current Chain Height from the Daemon Server. **/
		inline void GetHeight() { this -> WritePacket(GetPacket(GET_HEIGHT)); }
		
		
		/** Check the current Chain Height from the Daemon Server. **/
		inline void GetReward() { this -> WritePacket(GetPacket(GET_REWARD)); }
		
		
		/** Check if it is a new Round. **/
		inline void GetRound() { this -> WritePacket(GetPacket(GET_ROUND)); }
		
		
		/** Clear the Blocks Map from the Server. **/
		inline void ClearMaps() { this -> WritePacket(GetPacket(CLEAR_MAP)); }
		
		
		/** Check the current Chain Height from the Daemon Server. **/
		inline void Ping() { this -> WritePacket(GetPacket(PING)); }
		
		
		/** Set the Channel that will be Mined For. **/
		void SetChannel(unsigned int nChannel);
		
		
		/** Submit a new block to the Daemon Server. **/
		void SubmitBlock(uint512 hashMerkleRoot, uint64 nNonce);
		
		
		/** Send the Completed Coinbase Transaction to Server. **/
		void SetCoinbase();
		
		
		/** Get a new Block from the Daemon Server. **/
		void CheckBlock(uint1024 hashBlock);
		
		
		/** Make the Outgoing Connection to the Daemon Server. **/
		bool Connect()
		{
			try
			{
				uint16_t port = (uint16_t)atoi(PORT.c_str());
				CNetAddr ip(IP);
				CService addr(ip, port);
				SOCKET = std::make_shared<Socket>();
				SOCKET->Connect(addr, TIMEOUT);

				if (Errors())
				{
					Disconnect();
					return false;
				}

				CONNECTED = true;
				TIMER.Start();


				return true;
			}
			catch (...) {}

			CONNECTED = false;
			return false;
		}
		
		CBlock::Uptr DeserializeBlock(std::vector<unsigned char> const& DATA);
	};
	
	
	/** Handle of the Daemon Connection. **/
	class DaemonHandle
	{
		Thread_t THREAD;
		
		/** Incoming / Outgoing Blocks and Mutex Locks. **/
		std::vector<PoolConnection*> CONNECTIONS;
		std::mutex            CONNECTION_MUTEX;
	
		/** Daemon Outgoing Connection Handle **/
		DaemonConnection* CLIENT;
		
	public:
		
		unsigned int nTotalConnections = 0, ID = 0;
		bool fNewBlock = false;
		
		DaemonHandle(unsigned int nID, std::string IP, std::string PORT) 
			: ID(nID)
			, THREAD(std::bind(&DaemonHandle::DaemonThread, this))
		{ 
			CLIENT = new LLP::DaemonConnection(std::move(IP), std::move(PORT));
		}

		/** Flag to allow Handle to Resupply on New Block. **/
		bool fCoinbasePending = false;
		
		/** Set a New Block on Daemon and its corresponding Pool Connections. **/
		void NewBlock();
		
		/** Find a Vacant Pool Connection. **/
		int FindConnection();
		
		/** Assign a pool LLP connection to the Daemon Handle. **/
		int AssignConnection(PoolConnection* pConnection);
		
		/** Remove a Pool Connection from Daemon Handle. **/
		void RemoveConnection(int nIndex);
		
		/** Set the Coinbase Transaction for the Round. **/
		void SetCoinbase();
		
		/** Thread to handle Daemon Communications. **/
		void DaemonThread();
	};
}

#endif