#include "pool.h"
#include "daemon.h"
#include "ddos.h"
#include "../core.h"
#include "../util.h"
#include "../base58.h"
#include "../statscollector.h"
#include "../LLD/record.h"
#include <math.h>
#include <iostream>

namespace LLP
{	

	void PoolConnection::Clear()
	{
		LOCK(BLOCK_MUTEX);
		
		nBlockRequests = 0;
		nBlocksWaiting = 0;
		
		while(!NEW_BLOCKS.empty())
		{
			NEW_BLOCKS.top();
			NEW_BLOCKS.pop();
		}
						
		MAP_BLOCKS.clear();
	}

	void PoolConnection::AddBlock(CBlock::Uptr BLOCK)
	{
		LOCK(BLOCK_MUTEX);
		NEW_BLOCKS.push(std::move(BLOCK));
		
		nBlocksWaiting--;
	}
	
	
	/** Event Function to Customize Code For Inheriting Class Happening on the LLP Data Threads. **/
	void PoolConnection::Event(unsigned char EVENT, unsigned int LENGTH)
	{	
	
		/** Handle any DDOS Packet Filters. **/
		if(EVENT == EVENT_HEADER)
		{
			Packet PACKET = this->INCOMING;
			
			/** Check a Block Packet once the Header has been Read. **/
			if(m_bDDOS)
			{
				if(PACKET.LENGTH > 136)
					DDOS->Ban("Max Packet Size Exceeded");
				
				if(PACKET.HEADER == SUBMIT_SHARE && PACKET.LENGTH != 136)
					DDOS->Ban("Packet of Share not 136 Bytes");
					
				if(PACKET.HEADER == LOGIN && PACKET.LENGTH > 55)
					DDOS->Ban("Login Message too Large");

				if(PACKET.HEADER == SUBMIT_PPS && PACKET.LENGTH != 16)
					DDOS->Ban("Submit PPS Packet not 16 Bytes");
					
				if(PACKET.HEADER == BLOCK_DATA)
					DDOS->Ban("Received Block Data Header. Invalid as Request");
					
				if(PACKET.HEADER == ACCOUNT_BALANCE)
					DDOS->Ban("Received Account Balance Header. Inavlid as Request");
					
				if(PACKET.HEADER == PENDING_PAYOUT)
					DDOS->Ban("Recieved Pending Payout Header. Invald as Request");
					
				if(PACKET.HEADER == ACCEPT)
					DDOS->Ban("Received Share Accepted Header. Invalid as Request");
				
				if(PACKET.HEADER == REJECT)
					DDOS->Ban("Received Share Rejected Header. Invalid as Request.");
					
				if(PACKET.HEADER == BLOCK)
					DDOS->Ban("Received Block Header. Invalid as Request.");
					
				if(PACKET.HEADER == STALE)
					DDOS->Ban("Recieved Stale Share Header. Invalid as Request");
			}
				
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
			if(fNewBlock)
			{
				if(Core::fSubmittingBlock)
					return;
				
				Respond(NEW_BLOCK);
				
				/** Only Delete Maps on Pool Connection on New Block. **/
				Clear();
				
				fNewBlock = false;
			}
			
			/** If there are new blocks on the stack, send them to connection and add to map. **/
			{ LOCK(BLOCK_MUTEX);
				while(!NEW_BLOCKS.empty())
				{
					CBlock::Sptr BLOCK{std::move(NEW_BLOCKS.top())};
					NEW_BLOCKS.pop();
					
					/** Add to the block map. **/
					MAP_BLOCKS[BLOCK->GetHash()] = BLOCK;

					/** Construct a response packet by serializing the Block. **/
					Packet RESPONSE = GetPacket(BLOCK_DATA);
					RESPONSE.DATA   = SerializeBlock(std::move(BLOCK));
					RESPONSE.LENGTH = RESPONSE.DATA.size();
					
					this->WritePacket(RESPONSE);
					//printf("[THREAD] Pool LLP: Sent Block %s\n", BLOCK->GetHash().ToString().substr(0, 30).c_str());
				}
			}
			
			return;
		}
		
		/** On Connect Event, Assign the Proper Daemon Handle. **/
		if(EVENT == EVENT_CONNECT)
		{
			/** Assign this thread to a Daemon Handle. **/
			DAEMON = Core::FindDaemon();
			nID = DAEMON->AssignConnection(this);

			GUID = to_string(DAEMON->ID) +"_" +to_string(nID);
			
			return;
		}
		
		/** On Disconnect Event, Reduce the Connection Count for Daemon **/
		if(EVENT == EVENT_DISCONNECT)
		{
			DAEMON->RemoveConnection(nID);
			
			return;
		}
    }
    
   
	/** This function is necessary for a template LLP server. It handles your 
		custom messaging system, and how to interpret it from raw packets. **/
	bool PoolConnection::ProcessPacket()
	{
		Packet PACKET   = this->INCOMING;
		
		/** Handle Login. **/
		if(PACKET.HEADER == LOGIN)
		{
			/** Multiply DDOS Score for Multiple Logins after Successful Login. **/
			if(fLoggedIn)
			{
				if(m_bDDOS)
					DDOS->rSCORE += 10;
					
				return true;
			}
			
			ADDRESS = bytes2string(PACKET.DATA);
			Core::NexusAddress cAddress(ADDRESS);

			Core::STATSCOLLECTOR.IncConnectionCount(ADDRESS, GUID);
			
			if(!cAddress.IsValid() )
			{
				printf("[THREAD] Pool LLP: Bad Account %s\n", ADDRESS.c_str());
				if(m_bDDOS)
					DDOS->Ban("Invalid Nexus Address on Login");
					
				return false;
			}            

			std::string ip_address = GetRemoteIPAddress();
			std::cout << "[THREAD] Pool Login: " << ADDRESS << "\t IP:" << ip_address
				<< "\t (" << Core::STATSCOLLECTOR.GetConnectionCount(ADDRESS) << " connections)" << std::endl;

			if(!Core::AccountDB.HasKey(ADDRESS))
			{
				LLD::Account cNewAccount(ADDRESS);
				Core::AccountDB.UpdateRecord(cNewAccount);
					
				printf("\n+++++++++++++++++++++++++++++++++++++++++++++++++++\n\n");
				printf("[ACCOUNT] New Account %s\n", ADDRESS.c_str());
				printf("\n+++++++++++++++++++++++++++++++++++++++++++++++++++\n\n");
			}
            
            if(IsBannedAccount(ADDRESS) )
            {
                printf("[ACCOUNT] Account: %s is banned\n", ADDRESS.c_str() );
                
                // check to see whether this IP is in the banned IP list
                // if not then add it so that we can perma ban the IP
                if( !IsBannedIPAddress(ip_address) )
                {
                    SaveBannedIPAddress(ip_address);
                }
                
                DDOS->Ban("Account is Banned");
                
                fLoggedIn = false;                
            }
            else
            {
                fLoggedIn = true;
                
                //PS
                // QUICK HACK these are google cloud and amazon AWS IP ranges which we should allow
                // so don't add CHECK to debug.log 
                if((ip_address.find("104.") != 0) && (ip_address.find("130.") != 0)
                        && (ip_address.find("23.") != 0)  && (ip_address.find("54.") != 0)  && (ip_address.find("52.") != 0))
                    std::cout << "[ACCOUNT] Account: CHECK Address: " << ADDRESS  << " IP: " << ip_address << std::endl; // this allows you to grep the debug.log for CHECK and eyeball the frequency and ip addresses                
            }            
            
			return true;
		}
		
		
		/** Reject Requests if not Logged In. **/
		if(!fLoggedIn)
		{
			/** Amplify rScore for Requests before Logged in [Prevent DDOS this way]. **/
			if(m_bDDOS)
				DDOS->rSCORE += 10;
			
			printf("[THREAD] Pool LLP: Not Logged In. Rejected Request.\n");
			
			return false;
		}
		
		
		/** Ignore any Requests from Clients while Setting up a New Round. **/
		//if(Core::fCoinbasePending || Core::fSubmittingBlock || Core::fNewRound)
		//	return true;
		
		
		/** New block Process:
			Keeps a map of requested blocks for this connection.
			Clears map once new block is submitted successfully. **/
		if(PACKET.HEADER == GET_BLOCK)
		{ 
			LOCK(BLOCK_MUTEX); 
			nBlockRequests++; 

			if(m_bDDOS)
				DDOS->rSCORE += 1;

			return true; 
		}
		
		
		
		/** Submit Share Process:
			Accepts a new block Merkle and nNonce for submit.
			This is to correlate where in memory the actual
			block is from MAP_BLOCKS. **/
		if(PACKET.HEADER == SUBMIT_SHARE)
		{
			uint1024 hashPrimeOrigin;
			hashPrimeOrigin.SetBytes(std::vector<unsigned char>(PACKET.DATA.begin(), PACKET.DATA.end() - 8));

				
			/** Don't Accept a Share with no Correlated Block. **/
			if(!MAP_BLOCKS.count(hashPrimeOrigin))
			{
				Respond(REJECT);
				printf("[THREAD] Pool LLP: Block Not Found %s\n", hashPrimeOrigin.ToString().substr(0, 30).c_str());
				Respond(NEW_BLOCK);
				
				if(m_bDDOS)
					DDOS->rSCORE += 1;
				
				return true;
			}
			
			/** Reject the Share if it is not of the Most Recent Block. **/
			if(MAP_BLOCKS[hashPrimeOrigin]->nHeight != Core::nBestHeight)
			{
				printf("[THREAD] Rejected Share - Share is Stale, submitted: %f  current: %f\n", MAP_BLOCKS[hashPrimeOrigin]->nHeight, Core::nBestHeight);
				Respond(STALE);
				Respond(NEW_BLOCK);
				
				/** Be Lenient on Stale Shares [But Still amplify score above normal 1 per request.] **/
				if(m_bDDOS)
					DDOS->rSCORE += 2;
					
				return true;
			}
				
				
			/** Get the Prime Number Found. **/
			uint64 nNonce = bytes2uint64(std::vector<unsigned char>(PACKET.DATA.end() - 8, PACKET.DATA.end()));
			uint1024 hashPrime = hashPrimeOrigin + nNonce;
		
		
			/** Check the Share that was just submitted. **/
			{ LOCK(Core::PRIMES_MUTEX);
				if(Core::PRIMES_MAP.count(hashPrime))
				{
					printf("[THREAD] Duplicate Share %s\n", hashPrime.ToString().substr(0, 30).c_str());
					Respond(REJECT);
					
					/** Give a Heavy Score for Duplicates. [To Amplify the already existing ++ per Request.] **/
					if(m_bDDOS)
						DDOS->rSCORE += 5;
					
					return true;
				}
			}
			
			
			/** Check the Difficulty of the Share. **/
			//Timer SHARE_TIMER;
			//SHARE_TIMER.Start();
			//double nDifficulty = Core::CheckPrimeDifficulty(CBigNum(hashPrime));
			//SHARE_TIMER.Stop();
			
			//Timer GMP_TIMER;
			//GMP_TIMER.Start();
			double nDifficulty = Core::GmpVerification(hashPrime);
			//GMP_TIMER.Stop();
			
			if(Core::SetBits(nDifficulty) >= Core::nMinimumShare)
			{
				{ LOCK(Core::PRIMES_MUTEX);
					if(!Core::PRIMES_MAP.count(hashPrime))
						Core::PRIMES_MAP[hashPrime] = nDifficulty;
				}
				
				if(nDifficulty < 8)
				{
					Core::nDifficultyShares[(unsigned int) floor(nDifficulty - 3)]++;
				}
				
				LLD::Account cAccount = Core::AccountDB.GetRecord(ADDRESS);
				uint64 nWeight = pow(25.0, nDifficulty - 3.0);
				cAccount.nRoundShares += nWeight;
				Core::AccountDB.UpdateRecord(cAccount);
				
				//printf("[THREAD] Share Accepted of Difficulty %f | Weight %llu\n", nDifficulty, nWeight);
				//printf("[THREAD] Share Accepted [%s] of Difficulty %f GMP [%u ms]\n", ADDRESS.substr(0, 5).c_str(), nDifficulty, GMP_TIMER.ElapsedMilliseconds());
				
				/** If the share is above the difficulty, give block finder bonus and add to Submit Stack. **/
				if(nDifficulty >= Core::GetDifficulty(MAP_BLOCKS[hashPrimeOrigin]->nBits))
				{
					MAP_BLOCKS[hashPrimeOrigin]->nNonce = nNonce;
					
					{ LOCK(SUBMISSION_MUTEX);
						if(!SUBMISSION_BLOCK)
						{
							SUBMISSION_BLOCK = MAP_BLOCKS[hashPrimeOrigin];
							
							Respond(BLOCK);
						}
					}
					
					return true;
				}
				else
					Respond(ACCEPT);
			}
			else
			{
				printf("[THREAD] Share Below Difficulty %f\n", nDifficulty);
				
				/** Give Heavy Score for Below Difficulty Shares. Would require at least 4 per Second to fulfill a score of 20. **/
				if(m_bDDOS)
					DDOS->rSCORE += 5;
				
				Respond(REJECT);
			}
				
			return true;
		}
			
		/** Send the Current Account balance to Pool Miner. **/
		if(PACKET.HEADER == GET_BALANCE)
		{
			Packet RESPONSE = GetPacket(ACCOUNT_BALANCE);
			RESPONSE.LENGTH = 8;
			RESPONSE.DATA = uint2bytes64(Core::AccountDB.GetRecord(ADDRESS).nAccountBalance);
				
			//printf("[THREAD] Pool LLP: Account %s --> Balance Requested.\n", ADDRESS.c_str());
			this->WritePacket(RESPONSE);
				
			return true;
		}
		
		/** Send the Current Account balance to Pool Miner. **/
		if(PACKET.HEADER == GET_PAYOUT)
		{
			Packet RESPONSE = GetPacket(PENDING_PAYOUT);
			RESPONSE.LENGTH = 8;
			
			uint64 nPendingPayout = 0;
			if(Core::cGlobalCoinbase.mapOutputs.count(ADDRESS))
				nPendingPayout = Core::cGlobalCoinbase.mapOutputs[ADDRESS];
				
			RESPONSE.DATA = uint2bytes64(nPendingPayout);
				
			//printf("[THREAD] Pool LLP: Account %s --> Payout Data Requested.\n", ADDRESS.c_str());
	this->WritePacket(RESPONSE);
				
			return true;
		}
			
		/** Handle a Ping from the Pool Miner. **/
		if(PACKET.HEADER == PING)
		{
			this->WritePacket(GetPacket(PING)); return true; 
		}

		if(PACKET.HEADER == SUBMIT_PPS)
		{ 
			// grab the current PPS from the miner
			double PPS = bytes2double(std::vector<unsigned char>(PACKET.DATA.begin(), PACKET.DATA.end() - 8));	
			double WPS = bytes2double(std::vector<unsigned char>(PACKET.DATA.begin() +8, PACKET.DATA.end()));	
			
			Core::STATSCOLLECTOR.UpdateConnectionData( ADDRESS, GUID, PPS, WPS);

			this->WritePacket(GetPacket(PING)); return true; 
		}
			
		return false;
	}
	
	/** Convert the Header of a Block into a Byte Stream for Reading and Writing Across Sockets. **/
	std::vector<unsigned char> PoolConnection::SerializeBlock(CBlock::Sptr BLOCK)
	{
		std::vector<unsigned char> HASH        = BLOCK->GetHash().GetBytes();
		std::vector<unsigned char> MINIMUM     = uint2bytes(Core::nMinimumShare);
		std::vector<unsigned char> DIFFICULTY  = uint2bytes(BLOCK->nBits);
		std::vector<unsigned char> HEIGHT      = uint2bytes(BLOCK->nHeight);
			
		std::vector<unsigned char> DATA;
		DATA.insert(DATA.end(), HASH.begin(),             HASH.end());
		DATA.insert(DATA.end(), MINIMUM.begin(),       MINIMUM.end());
		DATA.insert(DATA.end(), DIFFICULTY.begin(), DIFFICULTY.end());
		DATA.insert(DATA.end(), HEIGHT.begin(),         HEIGHT.end());
			
		return DATA;
	}
}
