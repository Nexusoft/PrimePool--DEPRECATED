#include "LLP/pool.h"
#include "LLP/server.h"
#include "LLP/daemon.h"
#include "LLP/webui.h"
#include "LLD/record.h"

#include "core.h"
#include "base58.h"
#include "util.h"
#include <algorithm>

namespace Core
{

	/** Coinbase Transaction for this Block. **/
	Coinbase cGlobalCoinbase;
	std::string POOL_VERSION = "1.0.1";
	

	/** Wallet Connection Variables. **/
	std::map<std::string, double> ROUND_SHARES;
	std::map<std::string, uint64> ACCOUNT_BALANCE;
	
	std::vector<LLP::DaemonHandle*> DAEMON_CONNECTIONS;
	LLP::Server<LLP::PoolConnection>* SERVER;
	LLP::Server<LLP::UiConnection>* WEBSERVER;
	
	LLP::Thread_t MASTER;
	
	
	/** Mutex for Thread Synchronization [std::map is not thread safe]. **/
	boost::mutex              PRIMES_MUTEX;
	
	
	/** Map to hold the Prime Clusters Found. **/
	std::map<uint1024, double> PRIMES_MAP;
	
	
	/** Global Height to flag Pool Threads there is a new Block. **/
	unsigned int nBestHeight = 0;
	
	
	/** The Rewards for the Current Round. **/
	uint64 nRoundReward = 0;
	
	
	unsigned int nDifficultyShares[] = {0, 0, 0, 0, 0};
	
	
	/** Flag to track the new round and when it is setup by the Master Thread. **/
	bool fCoinbasePending = false, fNewRound = false, fSubmittingBlock = false;
	
	
	/** Hash of the Block being Submitted. **/
	uint1024 hashBlockSubmission(0);
	
	
	/** The current Round for the Pool. **/
	unsigned int nCurrentRound = 1;
	
	uint64 nMinimumShare = 45000000;
    
    double lfPoolFee = 0.02;
	
	/** The Database Handles. **/
	LLD::Database<uint1024, LLD::Block> BlockDB("blk.dat");
	LLD::Database<std::string, LLD::Account> AccountDB("acct.dat");
	
	/** DDOS map by Address. **/
	std::map<std::string, LLP::DDOS_Filter*> DDOS_BY_ADDRESS;
	
	
	/** The Address for the Last Round. **/
	std::string LAST_ROUND_BLOCKFINDER = "2S4PPSznWfPVLtPJNpi8Ly46Wft3wGbayGkhaGzKLVcepmrhKTP";
	
	
	/** Used to Sort the Database Account Keys by Balance. **/
	bool BalanceSort(const std::string& firstElem, const std::string& secondElem) { return Core::AccountDB.GetRecord(firstElem).nAccountBalance > Core::AccountDB.GetRecord(secondElem).nAccountBalance; }
	
	
	/** Return List of Accounts Sorted by Balance [Highest Balance First]. **/
	std::vector<std::string> GetSortedAccounts()
	{
		std::vector<std::string> vAccounts = AccountDB.GetKeys();
		std:sort(vAccounts.begin(), vAccounts.end(), BalanceSort);
		
		return vAccounts;
	}
	
	
	/** Entry Function to Start the Daemon Threads and Pool Server. **/
	void StartPool(int nPort, int nMaxDaemons, int nPoolThreads, bool fDDOS, int rScore, int cScore, int nMinShare, int nPoolFee)
	{
        //printf("go");
        printf("\n\n\n\n\n\n\n\n\n\n\n\Niro LLP Prime Pool %s Initializing...\n", POOL_VERSION.c_str());
        printf("[MASTER] Starting Pool with %d Daemon Handles, %d Pool Threads %f Min Share %s | rScore = %d cScore = %d\n", nMaxDaemons, nPoolThreads, nMinShare / 10000000.0, fDDOS ? "With DDOS Protection" : "", rScore, cScore);
		//printf("here");
		nMinimumShare = nMinShare;
        
        lfPoolFee = (double)nPoolFee / 100.0;

		InitializePrimes();
		Sleep(1000);
		
		printf("[MASTER] Initializing Databases.\n");

		std::vector<std::string> vAccounts = GetSortedAccounts();
		for( int nIndex = 0; nIndex < vAccounts.size(); nIndex++)
		{
			NexusAddress cAddress(vAccounts[nIndex]);
			if(!cAddress.IsValid())
			{
				printf("[DATABASE] Erasing Invalid Record: %s\n", vAccounts[nIndex].c_str());
				AccountDB.EraseRecord(vAccounts[nIndex].c_str());
			}
			else
				AccountDB.GetRecord(vAccounts[nIndex]).Print();
				//printf("%s:%f\n", vAccounts[nIndex].c_str(), AccountDB.GetRecord(vAccounts[nIndex]).nAccountBalance / 1000000.0);
		}
		AccountDB.WriteToDisk();

		
		std::vector<uint1024> vBlocks = BlockDB.GetKeys();
		for( int nIndex = 0; nIndex < vBlocks.size(); nIndex++)
		{
			LLD::Block cBlock = BlockDB.GetRecord(vBlocks[nIndex]);
			if(cBlock.nRound > nCurrentRound)
				nCurrentRound = cBlock.nRound;
		}
		
		LLP::Thread_t MASTER(MasterThread);
		LLP::Thread_t ORPHAN(OrphanThread);
        printf("Startin Daemon connections");
		for(int nIndex = 0; nIndex < nMaxDaemons; nIndex++)
			DAEMON_CONNECTIONS.push_back(new LLP::DaemonHandle(nIndex, "127.0.0.1", "9325"));
		printf("...done");
		Sleep(1000);
		SERVER    = new LLP::Server<LLP::PoolConnection>(nPort, nPoolThreads, fDDOS, cScore, rScore, 20);
		//WEBSERVER = new LLP::Server<LLP::UiConnection>  (9555, 5, false, 0, 0, 10);
	}
	
	
	/** Clear the Shares for the Current Round. **/
	void ClearShares()
	{
		std::vector<std::string> vKeys = AccountDB.GetKeys();
		for(int nIndex = 0; nIndex < vKeys.size(); nIndex++)
		{
			LLD::Account cAccount = AccountDB.GetRecord(vKeys[nIndex]);
			cAccount.nRoundShares = 0;
			AccountDB.UpdateRecord(cAccount);
		}
	}
	
	
	/** Get the Total Weight of Shares from the Current Round. **/
	uint64 TotalWeight()
	{	
		uint64 nWeight = 0;
		
		std::vector<std::string> vKeys = AccountDB.GetKeys();
		for(int nIndex = 0; nIndex < vKeys.size(); nIndex++)
			nWeight += AccountDB.GetRecord(vKeys[nIndex]).nRoundShares;
		
		return nWeight;
	}
	
	
	/** Refund the Payouts if the Block was Orphaned. **/
	void RefundPayouts(uint1024 hashBlock)
	{
		/** Get a Record from the Database. **/
		LLD::Block cBlock = BlockDB.GetRecord(hashBlock);
		
		
		/** Set the Flag that Block is an Orphan. **/
		cBlock.nRound = 0;
		
		
		printf("\n---------------------------------------------------\n\n");
		
		
		/** Clear out the Credits for the Round. **/
		for(std::map<std::string, uint64>::iterator nIterator = cBlock.cCredits.mapCredits.begin(); nIterator != cBlock.cCredits.mapCredits.end(); nIterator++)
		{
			LLD::Account cAccount = AccountDB.GetRecord(nIterator->first);
			if(nIterator->second > cAccount.nAccountBalance)
				cAccount.nAccountBalance = 0;
			else
				cAccount.nAccountBalance -= nIterator->second;
			
			printf("[MASTER] Account %s Removed %f Credits\n", nIterator->first.c_str(), nIterator->second / 1000000.0);
			AccountDB.UpdateRecord(cAccount);
		}
		
		
		/** Refund the Payouts for the Round. **/
		for(std::map<std::string, uint64>::iterator nIterator = cBlock.cCoinbase.mapOutputs.begin(); nIterator != cBlock.cCoinbase.mapOutputs.end(); nIterator++)
		{
			LLD::Account cAccount = AccountDB.GetRecord(nIterator->first);
			cAccount.nAccountBalance += nIterator->second;
			
			printf("[MASTER] Account %s Refunded %f NIRO\n", nIterator->first.c_str(), nIterator->second / 1000000.0);
			AccountDB.UpdateRecord(cAccount);
		}
		
		printf("\n---------------------------------------------------\n\n");
		
		
		/** Update the Record into the Database. **/
		BlockDB.UpdateRecord(cBlock);
		BlockDB.WriteToDisk();
	}
	
	
	/** Update the Account Balances from Weights in Current Round. **/
	void UpdateBalances(uint64 nReward)
	{
		printf("\n---------------------------------------------------\n\n");
		
		
		/** Create a New Block Record to Track Payouts. **/
		LLD::Block cBlock(hashBlockSubmission);
		cBlock.cCoinbase = cGlobalCoinbase;
		cBlock.nCoinbaseValue = cGlobalCoinbase.nMaxValue + cGlobalCoinbase.nPoolFee;
		cBlock.nRound = nCurrentRound;
		
		
		/** Make Sure Miners Aren't Credited the Pool Fee [2% for Now]. **/
		nReward -= (nReward * lfPoolFee);
		
		/** The 2% Block Finder Bonus from Previous Round. **/
		if(LAST_ROUND_BLOCKFINDER != "")
		{
			uint64 nCredit = (nReward * 0.02);
			LLD::Account cAccount = AccountDB.GetRecord(LAST_ROUND_BLOCKFINDER);
			cAccount.nAccountBalance += nCredit;
			AccountDB.UpdateRecord(cAccount);
			
			cBlock.cCredits.AddCredit(LAST_ROUND_BLOCKFINDER, nCredit);
            
            printf("[ACCOUNT] Block Finder Bonus to %s of %f NIRO\n", LAST_ROUND_BLOCKFINDER.c_str(), (nCredit) / 1000000.0 );
            
			nReward -= nCredit;
		}
		
		
		/** Calculate the Total Weight of the Last Round. **/
		uint64 nTotalWeight = TotalWeight();
		uint64 nTotalReward = 0;
		
		/** Update the Accounts in the Database Handle. **/
		std::vector<std::string> vKeys = AccountDB.GetKeys();
		for(int nIndex = 0; nIndex < vKeys.size(); nIndex++)
		{
			LLD::Account cAccount = AccountDB.GetRecord(vKeys[nIndex]);
				
			/** Update the Account Balance After Payout was Successful. **/
			if(cGlobalCoinbase.mapOutputs.count(vKeys[nIndex]))
			{
				if(cGlobalCoinbase.mapOutputs[vKeys[nIndex]] > cAccount.nAccountBalance)
					cAccount.nAccountBalance = 0;
				else
					cAccount.nAccountBalance -= cGlobalCoinbase.mapOutputs[vKeys[nIndex]];
					
				AccountDB.UpdateRecord(cAccount);
			}
			
			
			/** Don't Credit Account if there are no Shares for this Round. **/
			if(cAccount.nRoundShares == 0)
				continue;	
					
					
			/** Credit the Account from its Weight. **/
			unsigned int nCredit = (((double)cAccount.nRoundShares / nTotalWeight) * nReward);
            
			cAccount.nAccountBalance += nCredit;
			nTotalReward += nCredit;
			
			
			/** Update the New Credits in the Block Database. **/
			cBlock.cCredits.AddCredit(vKeys[nIndex], nCredit);
			
			
			/** Commit the New Account Record to Database. **/
			AccountDB.UpdateRecord(cAccount);
			
			
			printf("[ACCOUNT] Account: %s | Credit: %f NIRO | Balance: %f NIRO\n", cAccount.cKey.c_str(), nCredit / 1000000.0, cAccount.nAccountBalance / 1000000.0);
		}
		
		/** Any Remainder of the Rewards give to Blockfinder. **/
		if(nReward > nTotalReward)
		{
			LLD::Account cAccount = AccountDB.GetRecord(LAST_ROUND_BLOCKFINDER);
			cAccount.nAccountBalance += (nReward - nTotalReward);
			AccountDB.UpdateRecord(cAccount);
			
			printf("[ACCOUNT] Block Finder Additional Credit to %s of %f NIRO\n", LAST_ROUND_BLOCKFINDER.c_str(), (nReward - nTotalReward) / 1000000.0 );
		}
		
		/** Commit the New Block Record to the Block Database. **/
		BlockDB.UpdateRecord(cBlock);
		BlockDB.WriteToDisk();
		cBlock.cCredits.Print();
		
		/** Commit the Account Changes to Database. **/
		AccountDB.WriteToDisk();
		
		
		printf("[ACCOUNT] Balances Updated. Total Round Weight = %f | Total Round Reward: %f NIRO\n", nTotalWeight / 1000000.0, nReward / 1000000.0);
		printf("\n---------------------------------------------------\n\n");
	}
	

	/** Reset the Daemon Threads **/
	void ResetDaemons()
	{
		for(int index = 0; index < DAEMON_CONNECTIONS.size(); index++)
			DAEMON_CONNECTIONS[index]->fNewBlock = true;
	}
	
	
	/** Declare a new Round for the Pool. **/
	void NewRound()
	{
		fNewRound = true;
		fCoinbasePending = true;
		
		LLP::Timer TIMER;
		TIMER.Start();
		
		UpdateBalances(nRoundReward);
		ClearShares();
		nCurrentRound++;
		
		{ LOCK(PRIMES_MUTEX);
		  PRIMES_MAP.clear();
		}
		
		fNewRound = false;
		
		printf("[MASTER] New Round, Reset Handlers in %u ms\n", TIMER.ElapsedMilliseconds());
	}
	
	
	/** Determine the thread with the least amount of active connections.
		This keeps the load balanced across all server threads. **/
	LLP::DaemonHandle* FindDaemon()
	{
		int nIndex = 0, nConnections = DAEMON_CONNECTIONS[0]->nTotalConnections;
		for(int index = 1; index < DAEMON_CONNECTIONS.size(); index++)
		{
			if(DAEMON_CONNECTIONS[index]->nTotalConnections < nConnections)
			{
				nIndex = index;
				nConnections = DAEMON_CONNECTIONS[index]->nTotalConnections;
			}
		}
			
		return DAEMON_CONNECTIONS[nIndex];
	}
	
	/** Thread to Determine what Block and Rewards the Niro Network is on. **/
	void OrphanThread()
	{
		printf("[MASTER] Initialized Orphan Thread\n");
		LLP::Timer TIMER;
		TIMER.Start();
		
		LLP::DaemonConnection* CLIENT = new LLP::DaemonConnection("127.0.0.1", "9325");
		loop
		{
			Sleep(1);
			
			/** Attempt with best efforts to keep the Connection Alive. **/
			if(!CLIENT->Connected() || CLIENT->Errors() || CLIENT->Timeout(30))
			{
				Sleep(1000);
				
				TIMER.Reset();
				if(!CLIENT->Connect())
					continue;
					
				fSubmittingBlock = false;
				fNewRound = false;
				
			}

			/** Read a Packet until it is Complete. **/
			CLIENT->ReadPacket();
			if(CLIENT->PacketComplete())
			{
				LLP::Packet PACKET = CLIENT->NewPacket();
				CLIENT->ResetPacket();
				
				if(PACKET.HEADER == CLIENT->ORPHAN_BLOCK)
				{
					uint1024 hashOrphan;
					hashOrphan.SetBytes(PACKET.DATA);
					
					printf("[MASTER] Block %s - Round %u Has Been Orphaned\n", hashOrphan.ToString().substr(0, 20).c_str(), BlockDB.GetRecord(hashOrphan).nRound);
					Core::RefundPayouts(hashOrphan);
					
					fCoinbasePending = true;
				}
				
				else if(PACKET.HEADER == CLIENT->GOOD_BLOCK)
				{
					uint1024 hashBlock;
					hashBlock.SetBytes(PACKET.DATA);
						
					printf("[MASTER] Block %s - Round: %u Is in Main Chain\n", hashBlock.ToString().substr(0, 20).c_str(), BlockDB.GetRecord(hashBlock).nRound);
				}
			
			}
			
			/** Don't make any new requests until Master Thread is Set. **/
			if(fNewRound || fSubmittingBlock || fCoinbasePending)
				continue;
			
			/** Check last 5 blocks every 20 seconds. **/
			if(TIMER.ElapsedMilliseconds() > 20000)
			{
				std::vector<uint1024> vKeys = BlockDB.GetKeys();
				for(int nIndex = 0; nIndex < vKeys.size(); nIndex++)
					if(BlockDB.GetRecord(vKeys[nIndex]).nRound >= nCurrentRound)
						CLIENT->CheckBlock(vKeys[nIndex]);
					
				TIMER.Reset();
			}	
		}
	}
	
	/** Thread to Determine what Block and Rewards the Coinshield Network is on. **/
	void MasterThread()
	{
		printf("[MASTER] Initialized Master Thread\n");
		LLP::Timer TIMER;
		TIMER.Start();
		
		LLP::Timer METER_TIMER;
		METER_TIMER.Start();
		
		
		LLP::DaemonConnection* CLIENT = new LLP::DaemonConnection("127.0.0.1", "9325");
		loop
		{
			Sleep(1);
			
			/** Let thread idle while account balances are updated. **/
			if(fNewRound || fSubmittingBlock)
				continue;
				
			/** Attempt with best efforts to keep the Connection Alive. **/
			if(!CLIENT->Connected() || CLIENT->Errors() || CLIENT->Timeout(30))
			{
				Sleep(1000);
				
				if(!CLIENT->Connect())
					continue;

				CLIENT->SetChannel(1);
				
				CLIENT->GetHeight();
				CLIENT->GetRound();
				
				TIMER.Reset();
				METER_TIMER.Reset();
				
				fNewRound = false;
				fSubmittingBlock = false;
				fCoinbasePending = true;
				
				printf("[MASTER] Connect to Daemon Server...\n");
			}
			
			
			if(METER_TIMER.ElapsedMilliseconds() > 10000)
			{
				
				printf("[METERS] 4 ch: x %f | 5 ch: x %f | 6 ch: x %f | 7 ch: x %f\n", (double)nDifficultyShares[0] / nDifficultyShares[1],
				(double)nDifficultyShares[1] / nDifficultyShares[2], (double)nDifficultyShares[2] / nDifficultyShares[3], (double)nDifficultyShares[3] / nDifficultyShares[4]);
				
				METER_TIMER.Reset();
			}
			
			
			if(TIMER.ElapsedMilliseconds() > 2000)
			{
				CLIENT->GetHeight();
				CLIENT->GetRound();
					
				if(fCoinbasePending)
				{
					//printf("[MASTER] Getting Reward.\n");
					CLIENT->GetReward();
				}
						
				//printf("[MASTER] Checking Round.\n");
				
				TIMER.Reset();
			}
			
				
			/** Read a Packet until it is Complete. **/
			CLIENT->ReadPacket();
			if(!CLIENT->PacketComplete())
				continue;
					
					
			/** Handle the New Packet, and Interpret its Data. **/
			LLP::Packet PACKET = CLIENT->NewPacket();
			CLIENT->ResetPacket();
					
			if(fCoinbasePending)
				printf("[MASTER} There is a New Packet...\n");
						
			/** Update the Global Round Height if there is a new Block. **/
			if(PACKET.HEADER == CLIENT->BLOCK_HEIGHT) 
			{ 
				unsigned int nHeight = bytes2uint(PACKET.DATA);
				//printf("[MASTER] Height Received %u\n", nHeight);
				
				if(nHeight > nBestHeight)
				{
					printf("[MASTER] Coinshield Network: New Block [Height] %u\n", nHeight);
					fCoinbasePending = true;

				}
				
				nBestHeight = nHeight;
			}
			
			
			/** Response from Mining LLP that there is a new block. **/
			else if(PACKET.HEADER == CLIENT->NEW_ROUND)
			{
				printf("[MASTER] Coinshield Network: New Block [Round] %u\n", nBestHeight);
				fCoinbasePending = true;
			}
			
			/** After the Coinbase Transaction is Set on Master Thread, Check the Blocks for Orphans. **/
			else if(PACKET.HEADER == CLIENT->COINBASE_SET)
			{
				printf("[MASTER] Coinbase Transaction Set on Master\n");
				fCoinbasePending = false;
				ResetDaemons();
				
					
				/** Commit Shares to Database on New Block. **/
				AccountDB.WriteToDisk();				
			}
			
			/** If the Coinbase Tx Fails to Update, get data again and attempt to rebuild it. **/
			else if(PACKET.HEADER == CLIENT->COINBASE_FAIL)
			{
				printf("[MASTER] ERROR: Coinbase Transaction Not Set. Retrying...\n");
				
				fCoinbasePending = true;
			}
				
			/** Update the Current Round Rewards. **/
			else if(PACKET.HEADER == CLIENT->BLOCK_REWARD)
			{
				nRoundReward = bytes2uint64(PACKET.DATA);
				printf("[MASTER] Received Reward of %f\n", nRoundReward / 1000000.0);
				
				if(nCurrentRound > 1 && fCoinbasePending)
				{
					
					cGlobalCoinbase.nPoolFee = (nRoundReward * lfPoolFee);
					cGlobalCoinbase.Reset(nRoundReward - cGlobalCoinbase.nPoolFee);
					
					
					std::vector<std::string> vAccounts = GetSortedAccounts();
					for(int nIndex = 0; nIndex < vAccounts.size(); nIndex++)
					{
						/** Read the Account Record from the Database. **/
						LLD::Account cAccount = AccountDB.GetRecord(vAccounts[nIndex]);
						
						/** Only add a Payout if Above the 0 NIRO Threshold. **/
						if(cAccount.nAccountBalance > 0)
						{
							int nValue = cGlobalCoinbase.AddTransaction(vAccounts[nIndex], cAccount.nAccountBalance);
							if(nValue < 0)
							{
								cGlobalCoinbase.AddTransaction(vAccounts[nIndex], cAccount.nAccountBalance + nValue);
										
								break;
							}
						}
					}
						
					/** Add NIRO Block Finder Bonus [On top of Larger Weight] on Rounds where Coinbase is Incomplete. **/
					if(!cGlobalCoinbase.IsComplete())
					{
						int nBonus = cGlobalCoinbase.GetRemainder();
						cGlobalCoinbase.AddTransaction(LAST_ROUND_BLOCKFINDER, nBonus);
					}
						
					cGlobalCoinbase.Print();
					CLIENT->SetCoinbase();
				}
				else
                {
					fCoinbasePending = false;
                    ResetDaemons();
                }
				printf("[DAEMON] Round %u Block %u Rewards %f NIRO\n", nCurrentRound, nBestHeight, nRoundReward / 1000000.0);
			}
		}
	}
}