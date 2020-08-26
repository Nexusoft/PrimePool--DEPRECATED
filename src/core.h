#ifndef COINSHIELD_LLP_CORE_H
#define COINSHIELD_LLP_CORE_H

#include "hash/uint1024.h"
#include "hash/templates.h"
#include "util.h"
#ifdef WIN32
#include <mpir.h>
#else
#include <gmp.h>
#endif
#include "config.h"

#include <memory>
#include <mutex>
#include <map>

class Coinbase;


namespace LLD
{
	class Account; 
	class Block;
	template<typename KeyType, class RecordType> class Database;
}

namespace LLP { class DaemonHandle; class DaemonConnection; class Timer; class DDOS_Filter; }
namespace Core
{	
	class StatsCollector;

	/** Global Best Height Tracker. Keeps track of Current Block. **/
	extern unsigned int nBestHeight;
	
	/** Mock Class for Building Block Hash. **/
	class CBlock
	{
	public:

		/** Begin of Header.   BEGIN(nVersion) **/
		unsigned int  nVersion;
		uint1024 hashPrevBlock;
		uint512 hashMerkleRoot;
		unsigned int  nChannel;
		unsigned int   nHeight;
		unsigned int     nBits;
		uint64          nNonce;
		/** End of Header.     END(nNonce). 
			All the components to build an SK1024 Block Hash. **/
		
		
		CBlock()
		{
			nVersion        = 0;
			hashPrevBlock   = 0;
			hashMerkleRoot  = 0;
			nChannel        = 0;
			nHeight         = 0;
			nBits           = 0;
			nNonce          = 0;
		}
		
		inline uint1024 GetHash() const { return SK1024(BEGIN(nVersion), END(nBits)); }
		inline uint1024 GetPrime() const { return GetHash() + nNonce; }
	};

	/** Pool Configuration **/
	extern Config CONFIG; 

	/** Statistics Collector **/
	extern Core::StatsCollector STATSCOLLECTOR;
	
	/** The IP of the wallet server **/
	extern std::string WALLET_IP_ADDRESS;
	extern std::string WALLET_PORT;
	
	/** Global Declaration of the Coinbase Transaction. **/
	extern Coinbase cGlobalCoinbase;
	
	
	/** Flag to let other threads know when the Master is Setting up another Round. **/
	extern bool fCoinbasePending, fNewRound, fSubmittingBlock;
	
	
	/** Hash of the Block being Submitted. **/
	extern uint1024 hashBlockSubmission;
	
	
	/** The current Round for the Pool. **/
	extern unsigned int nCurrentRound;

	/** The current found reward**/
	extern uint64 nRoundReward;
	
	/** The Address of the Last Blockfinder. **/
	extern std::string LAST_ROUND_BLOCKFINDER;
	
	/** Last block information **/
	extern unsigned int nLastBlockFound;
	extern LLP::Timer LAST_BLOCK_FOUND_TIMER;
	
	/** Map to store DDOS scores by Address. **/
	extern std::map<std::string, LLP::DDOS_Filter*> DDOS_BY_ADDRESS;
	
	
	
	/** --------- PRIME.CPP ----------- **/
	void InitializePrimes();
	
	uint1024 FermatTest(uint1024 n);
	unsigned int SetBits(double nDiff);
	unsigned int GetFractionalDifficulty(uint1024 composite);
	
	double GetDifficulty(unsigned int nBits);
	double GmpVerification(uint1024 prime);
	
	
	/** --------- CORE.CPP ----------- **/
	extern std::map<uint1024, double> PRIMES_MAP;
	extern std::mutex              PRIMES_MUTEX;
	
	extern LLP::Timer nMeterTimer;
	extern unsigned int nShares;
	extern unsigned int nAccounts;
	extern unsigned int nConnections;
	extern unsigned int nDifficultyShares[];
	
	extern uint64 nMinimumShare;
	
	
	/** Databases to be used in Memory. Committed to Disk Every Second to reduce Hard Disk Load by Master Thread. **/
	extern LLD::Database<uint1024, LLD::Block> BlockDB;
	extern LLD::Database<std::string, LLD::Account> AccountDB;
	
	
	/** Start the Daemon Handlers and LLP Pool Server. **/
	void StartPool(int nPort, int nMaxDaemons, int nPoolThreads, bool fDDOS, int rScore, int cScore, int nMinShare, int nPoolFee);
	
	
	/** Clear the Global Shares Object. **/
	void ClearShares();
	
	
	/** Determine the Weight of All Accounts. **/
	uint64 TotalWeight();
	
	
	/** Refund the Payouts on Orphaned Block. **/
	void RefundPayouts(uint1024 hashBlock);
	
	
	/** Update Account Balances with Given Block Reward. **/
	void UpdateBalances(uint64 nReward);
	
	
	/** Find the Least Loaded Daemon Connection. **/
	LLP::DaemonHandle* FindDaemon();
	
	
	/** Reset the Daemon Connections for a New Block. **/
	void ResetDaemons();
	
	
	/** Reset Daemon Handlers for a New Round. **/
	void NewRound();
	
	
	/** Thread to Track the Blocks Found. **/
	void OrphanThread();
	
	/** Thread to Track the Niro Network. **/
	void MasterThread();
}

#endif
