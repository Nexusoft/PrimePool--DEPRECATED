#include "daemon.h"
#include "pool.h"
#include "../coinbase.h"
#include "../core.h"
#include "../util.h"
#include "../statscollector.h"

#include <iostream>

namespace LLP
{

	void DaemonConnection::SetChannel(unsigned int nChannel)
	{
		Packet packet = GetPacket(SET_CHANNEL);
		packet.LENGTH = 4;
		packet.DATA   = uint2bytes(nChannel);
			
		this -> WritePacket(packet);
	}
	
	void DaemonConnection::SetCoinbase()
	{
		Packet packet = GetPacket(SET_COINBASE);
		packet.DATA   = Core::cGlobalCoinbase.Serialize();
		packet.LENGTH = packet.DATA.size();
			
		this -> WritePacket(packet);
	}
	
	void DaemonConnection::CheckBlock(uint1024 hashBlock)
	{
		Packet packet = GetPacket(CHECK_BLOCK);
		packet.DATA   = hashBlock.GetBytes();
		packet.LENGTH = packet.DATA.size();
		
		this -> WritePacket(packet);
	}
	

	/** Submit a new block to the Daemon Server. **/
	void DaemonConnection::SubmitBlock(uint512 hashMerkleRoot, uint64 nNonce)
	{
		Packet PACKET = GetPacket(SUBMIT_BLOCK);
		PACKET.DATA = hashMerkleRoot.GetBytes();
		std::vector<unsigned char> NONCE  = uint2bytes64(nNonce);
			
		PACKET.DATA.insert(PACKET.DATA.end(), NONCE.begin(), NONCE.end());
		PACKET.LENGTH = 72;
			
		this->WritePacket(PACKET);
	}
	

	CBlock::Uptr DaemonConnection::DeserializeBlock(std::vector<unsigned char> const& DATA)
	{
		CBlock::Uptr BLOCK   = std::make_unique<CBlock>();
		BLOCK->nVersion      = bytes2uint(std::vector<unsigned char>(DATA.begin(), DATA.begin() + 4));
			
		BLOCK->hashPrevBlock.SetBytes (std::vector<unsigned char>(DATA.begin() + 4, DATA.begin() + 132));
		BLOCK->hashMerkleRoot.SetBytes(std::vector<unsigned char>(DATA.begin() + 132, DATA.end() - 20));
			
		BLOCK->nChannel      = bytes2uint(std::vector<unsigned char>(DATA.end() - 20, DATA.end() - 16));
		BLOCK->nHeight       = bytes2uint(std::vector<unsigned char>(DATA.end() - 16, DATA.end() - 12));
		BLOCK->nBits         = bytes2uint(std::vector<unsigned char>(DATA.end() - 12,  DATA.end() - 8));
		BLOCK->nNonce        = bytes2uint64(std::vector<unsigned char>(DATA.end() - 8,  DATA.end()));
			
		return BLOCK;
	}
	
	
	int DaemonHandle::AssignConnection(PoolConnection* pConnection)
	{
		LOCK(CONNECTION_MUTEX);
		
		int nID = FindConnection();
		if(nID == -1)
		{
			nID = CONNECTIONS.size();
			CONNECTIONS.push_back(NULL);
		}
		
		CONNECTIONS[nID] = pConnection;
		nTotalConnections++;
		
		std::cout << "[DAEMON] Pool Connection " << nID << " Added to Daemon Handle " << ID << " IP: " <<  CONNECTIONS[nID]->GetRemoteIPAddress() << std::endl;
		return nID;
	}	
	
	void DaemonHandle::NewBlock()
	{
		LOCK(CONNECTION_MUTEX);
		
		for(int nIndex = 0; nIndex < CONNECTIONS.size(); nIndex++)
			if(CONNECTIONS[nIndex])
				CONNECTIONS[nIndex]->fNewBlock = true;
				
		CLIENT->ClearMaps();
	}	
	
	void DaemonHandle::RemoveConnection(int nIndex)
	{
		LOCK(CONNECTION_MUTEX);
		
		if( CONNECTIONS[nIndex]->ADDRESS != "")
			Core::STATSCOLLECTOR.DecConnectionCount(CONNECTIONS[nIndex]->ADDRESS, CONNECTIONS[nIndex]->GUID);

		std::cout << "[DAEMON] Pool Connection " << nIndex << " Removed from Daemon Handle " << ID << std::endl;
	//	printf("[DAEMON] Pool Connection %i Removed from Daemon Handle %u.  IP: %s\n", nIndex, ID, CONNECTIONS[nIndex]->GetIPAddress().c_str());
		
		CONNECTIONS[nIndex] = NULL;
		nTotalConnections--;
	}		

	int DaemonHandle::FindConnection()
	{
		for(int nIndex = 0; nIndex < CONNECTIONS.size(); nIndex++)
			if(!CONNECTIONS[nIndex])
				return nIndex;
			
		return -1;
	}
	
	
	/** Thread to handle Daemon Connection Requests via the Mining LLP. **/
	void DaemonHandle::DaemonThread()
	{
		while(!CLIENT)
			Sleep(10);
			
		//printf("[DAEMONS] Initialized Daemon Handle Thread %u\n", ID);
		LLP::Timer TIMER;
		loop
		{
			Sleep(10);
		
			/** Attempt with best efforts to keep the Connection Alive. **/
			if(!CLIENT->Connected() || CLIENT->Errors() || CLIENT->Timeout(60))
			{
				Sleep(1000);
				
				if(!CLIENT->Connect())
					continue;
				
				CLIENT->SetChannel(1);
				TIMER.Start();
				
				//fNewBlock = true;
				Core::fNewRound = false;
				Core::fSubmittingBlock = false;
				Core::fCoinbasePending = true;
				
				fNewBlock = false;
				fCoinbasePending = false;
			}
			
			
			/** Ping The Daemon to Keep Connection Alive. **/
			if(TIMER.ElapsedMilliseconds() > 10000)
			{
				CLIENT->Ping();
				TIMER.Reset();
			}
			
			
			/** Read a Packet until it is Complete. **/
			CLIENT->ReadPacket();
			if(CLIENT->PacketComplete())
			{
					
				/** Handle the New Packet, and Interpret its Data. **/
				LLP::Packet PACKET = CLIENT->NewPacket();
				CLIENT->ResetPacket();
							
							
				/** If a Block was Accepted by the Network, start a new Round. **/
				if(PACKET.HEADER == CLIENT->GOOD)
				{
					printf("[DAEMON] Block %s Accepted by Niro Network on Handle %u\n", Core::hashBlockSubmission.ToString().substr(0, 20).c_str(), ID);
					Core::NewRound();
					
					Core::fSubmittingBlock = false;
				}
					
				else if(PACKET.HEADER == CLIENT->FAIL)
				{
					printf("[DAEMON] Block Rejected by Niro Network on Handle %u\n", ID);
					Core::fSubmittingBlock = false;
				}
				else if(PACKET.HEADER == CLIENT->COINBASE_SET)
				{
					//printf("[DAEMON] Coinbase Transaction Set on Handle %u\n", ID);
					
					fCoinbasePending = false;
				}
				else if(PACKET.HEADER == CLIENT->COINBASE_FAIL)
				{
					printf("[DAEMON] ERROR: Coinbase Transaction Not Set on Handle %u\n", ID);
					
					Core::fNewRound = false;
					Core::fSubmittingBlock = false;
					Core::fCoinbasePending = true;
					
					fNewBlock = false;
					fCoinbasePending = false;
				}
						
				/** Add a Block to the Stack if it is received by the Daemon Connection. **/
				else if(PACKET.HEADER == CLIENT->BLOCK_DATA)
				{
					CBlock::Uptr BLOCK = CLIENT->DeserializeBlock(PACKET.DATA);
					/** If the Block isn't less than 1024-bits request a new one **/
					if(BLOCK->GetHash().high_bits(0x80000000))
					{
						printf("[DAEMON] Block isn't less than 1024-bits. Request new one on Handle %u\n", ID);
						CLIENT->GetBlock();
					}
					else
					{
						if(BLOCK->nHeight == Core::nBestHeight)
						{
							{ LOCK(CONNECTION_MUTEX);
								for(int nIndex = 0; nIndex < CONNECTIONS.size(); nIndex++)
								{
									if(!CONNECTIONS[nIndex])
										continue;
										
									if(CONNECTIONS[nIndex]->nBlocksWaiting > 0)
									{
										CONNECTIONS[nIndex]->AddBlock(std::move(BLOCK));
										
										printf("[DAEMON] Block Received Height = %u on Handle %u Assigned to %u\n", Core::nBestHeight, ID, nIndex);
										break;
									}
								}
							}						
						}
						else
							printf("[DAEMON] Block Obsolete Height = %u, Skipping over on Handle %u\n", BLOCK->nHeight, ID);
					}
				}			
			}			
			
			/** Don't Request Anything if Waiting for New Round to Reset. **/
			if(Core::fCoinbasePending || Core::fSubmittingBlock || Core::fNewRound || fCoinbasePending)
				continue;
			
			
			/** Reset The Daemon if there is a New Block. **/
			if(fNewBlock)
			{
				fNewBlock = false;
				
				NewBlock();
				//printf("[DAEMON] Niro Network: New Block | Reset Daemon Handle %u\n", ID);
				
				/** Set the new Coinbase Transaction. **/
				if(Core::nCurrentRound > 1)
				{
					fCoinbasePending = true;
					CLIENT->SetCoinbase();
					
					continue;
				}
			}
			
			
			{ LOCK(CONNECTION_MUTEX);
				for(int nIndex = 0; nIndex < CONNECTIONS.size(); nIndex++)
				{
					if(!CONNECTIONS[nIndex])
						continue;
					
					/** Submit a block from Pool Connection if there is one available. **/
					{ LOCK(CONNECTIONS[nIndex]->SUBMISSION_MUTEX);
						if(CONNECTIONS[nIndex]->SUBMISSION_BLOCK)
						{
							Core::fSubmittingBlock = true;
							Core::hashBlockSubmission = CONNECTIONS[nIndex]->SUBMISSION_BLOCK->GetHash();
							 
							
							/** Submit the Block to Network if it is Valid. **/
							if(CONNECTIONS[nIndex]->SUBMISSION_BLOCK->nHeight == Core::nBestHeight)
							{
								CLIENT->SubmitBlock(CONNECTIONS[nIndex]->SUBMISSION_BLOCK->hashMerkleRoot, CONNECTIONS[nIndex]->SUBMISSION_BLOCK->nNonce);

								Core::LAST_BLOCK_FOUND_TIMER.Reset();
								Core::LAST_ROUND_BLOCKFINDER = CONNECTIONS[nIndex]->ADDRESS;
								Core::nLastBlockFound = CONNECTIONS[nIndex]->SUBMISSION_BLOCK->nHeight;

								printf("[DAEMON] Submitting Block for Address %s on Handle %u\n", CONNECTIONS[nIndex]->ADDRESS.c_str(), ID);
							}
							else
								printf("[DAEMON] Stale Block Submitted on Handle %u\n", ID);


							/** Reset the Submission Block Pointer. **/
							CONNECTIONS[nIndex]->SUBMISSION_BLOCK = nullptr;
							break;
						}
					}
					
					/** Get Blocks if Requested From Pool Connection. **/
					{ LOCK(CONNECTIONS[nIndex]->BLOCK_MUTEX);
						while(CONNECTIONS[nIndex]->nBlockRequests > 0)
						{
							CONNECTIONS[nIndex]->nBlockRequests--;
							CONNECTIONS[nIndex]->nBlocksWaiting++;
							
							CLIENT->GetBlock();
							
							printf("[DAEMON] Requesting Block from Daemon Handle %u\n", ID);
						}
					}
				}
			}
		}
	}
}
