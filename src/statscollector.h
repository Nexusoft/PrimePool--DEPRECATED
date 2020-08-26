#ifndef NEXUS_STATS_H
#define NEXUS_STATS_H

#include <string>
#include "json/json_spirit_reader_template.h"
#include "json/json_spirit_writer_template.h"
#include <boost/serialization/list.hpp>
#include "util.h"
#ifdef WIN32
#include <mpir.h>
#else
#include <gmp.h>
#endif
#include "core.h"
#include "statspersister.h"

namespace Core
{
	class PoolData
	{
	public:
		PoolData()
		{
			nRound = Core::nCurrentRound;
			nBlockNumber = Core::nBestHeight;
			nRoundReward = Core::nRoundReward;
			nTotalShares = Core::TotalWeight();
			nConnectionCount = Core::nConnections;
			
		}

		unsigned int nRound;
		unsigned int nBlockNumber;
        uint64 nRoundReward;
		uint64 nTotalShares;
		unsigned int nConnectionCount;
  
	};

	class ConnectionMinerData
	{
		public:
		ConnectionMinerData(){PPS = 0, WPS = 0, GUID="";}
		ConnectionMinerData(std::string lGUID){GUID = lGUID, PPS = 0, WPS = 0;}

		double PPS, WPS;
		std::string GUID;
	};

	class ConnectionData
	{
		public:
		ConnectionData(){ADDRESS="";}
		ConnectionData(std::string lAddress){ADDRESS = lAddress, LASTSAVETIMER.Start();}

		double GetTotalPPS() const 
		{
			double PPS = 0;
			for(std::map<std::string, ConnectionMinerData >::const_iterator iter = MINER_DATA.begin(); iter != MINER_DATA.end(); ++iter)
			{
				PPS += iter->second.PPS;
			}

			return PPS;
		}
		double GetTotalWPS() const 
		{
			double WPS = 0;
			for(std::map<std::string, ConnectionMinerData >::const_iterator iter = MINER_DATA.begin(); iter != MINER_DATA.end(); ++iter)
			{
				WPS += iter->second.WPS;
			}

			return WPS;
		}

		std::string ADDRESS;
		std::map<std::string, ConnectionMinerData > MINER_DATA;

		/** Used to track when this connection data was last persisted**/
		LLP::Timer LASTSAVETIMER;
	};

	class RoundData
	{
	public:
		RoundData(int nRoundToSet)
		{
			nRound = nRoundToSet;
		}

		RoundData()
		{
			nRound = Core::nCurrentRound;
			nBlockNumber = Core::nBestHeight;
			hashBlock = Core::hashBlockSubmission.ToString(); 
			nRoundReward = Core::nRoundReward;
			nTotalShares = Core::TotalWeight();
			strBlockFinder = Core::LAST_ROUND_BLOCKFINDER;
			bOrphan = false;
			tBlockFoundTime = time(0);
		}


		unsigned int nRound;
		unsigned int nBlockNumber;
        std::string hashBlock;
        uint64 nRoundReward;
		uint64 nTotalShares;
		std::string strBlockFinder;
		bool bOrphan;
        time_t tBlockFoundTime;
	};

	class AccountData
	{
	public:

		std::string strAccountAddress;
        int nConnections;
		uint64 nShares;
        uint64_t nBalance ;
        uint64_t nPendingPayout;

	};

	class AccountEarningTransaction
	{
	public:
		AccountEarningTransaction(){}

		std::string strAccountAddress;
		unsigned int nRound;
		unsigned int nBlockNumber;
		uint64 nShares;
		uint64 nAmountEarned;
		time_t tTime;

	};

	class AccountPaymentTransaction
	{	
	public:
		AccountPaymentTransaction(){};

		std::string strAccountAddress;
		unsigned int nRound;
		unsigned int nBlockNumber;
		unsigned int nAmountPaid;
		time_t tTime;

	};

	/** Class used to generate JSON-formatted pool statistics **/
	class StatsCollector
	{
	public:

		StatsCollector(){STATSPERSISTER = NULL; }
		~StatsCollector();
		void Init();


		void IncConnectionCount( std::string ADDRESS, std::string GUID );
		void DecConnectionCount( std::string ADDRESS, std::string GUID );
		int GetConnectionCount( std::string ADDRESS );
		void UpdateConnectionData(std::string ADDRESS, std::string GUID, double PPS, double WPS);

		void UpdatePoolData();
		void UpdateAccountData(bool bUpdateAll = false);
        void ClearAccountRoundShares( );

		void SaveCurrentRound();
		void FlagRoundAsOrphan(unsigned int nRound);


		void AddAccountEarnings( std::string strAccountAddress,
								unsigned int nRound,
								unsigned int nBlockNumber,
								uint64 nShares,
								uint64 nAmountEarned,
								time_t nTime);
		void DeleteAccountEarnings( std::string strAccountAddress,
									unsigned int nRound);

		void AddAccountPayment( std::string strAccountAddress,
								unsigned int nRound,
								unsigned int nBlockNumber,
								uint64 nAmountPaid,
								time_t nTime);
		void DeleteAccountPayment( std::string strAccountAddress,
									unsigned int nRound);
		
	
	protected:

		/** Pointer to the StatsPersister that should be specialised for specific database **/
		StatsPersister* STATSPERSISTER;

		std::map<std::string, ConnectionData> CONNECTIONS_BY_ADDRESS;

		std::mutex CONNECTIONS_BY_ADDRESS_MUTEX;
		std::mutex POOL_DATA_MUTEX;
		std::mutex ROUND_HISTORY_MUTEX;
		std::mutex ACCOUNT_DATA_MUTEX;
		std::mutex ACCOUNT_EARNINGS_MUTEX;

	private:
		
	};

}
#endif
