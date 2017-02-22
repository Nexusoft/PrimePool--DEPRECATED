#include "statscollector.h"
#include "util.h"
#include "core.h"
#include "LLP/types.h"
#include "LLD/record.h"
#include "json/json_spirit_reader_template.h"
#include "json/json_spirit_writer_template.h"
#include "json/json_spirit_utils.h"
#include <boost/format.hpp>
#include <boost/archive/text_oarchive.hpp>
#include <boost/archive/text_iarchive.hpp>
#include <boost/serialization/vector.hpp>
#include <boost/serialization/map.hpp>

#include "mysqlstatspersister.h"

using namespace json_spirit;
using namespace std;

namespace Core
{

void StatsCollector::Init()
{
    STATSPERSISTER = new MySQLStatsPersister();
    STATSPERSISTER->Init();
}

StatsCollector::~StatsCollector()
{
    STATSPERSISTER->Destroy();
}

void StatsCollector::IncConnectionCount( std::string ADDRESS, std::string GUID )
{
    CONNECTIONS_BY_ADDRESS_MUTEX.lock();
    CONNECTIONS_BY_ADDRESS[ADDRESS].ADDRESS = ADDRESS;
    CONNECTIONS_BY_ADDRESS[ADDRESS].MINER_DATA[GUID] = ConnectionMinerData(GUID);
    CONNECTIONS_BY_ADDRESS_MUTEX.unlock();
}

void StatsCollector::DecConnectionCount( std::string ADDRESS, std::string GUID )
{
    CONNECTIONS_BY_ADDRESS_MUTEX.lock();
    CONNECTIONS_BY_ADDRESS[ADDRESS].MINER_DATA.erase(GUID);

    // if this was the last connection for this address then we need to save the current connection data
    // to record the count of 0
    if( CONNECTIONS_BY_ADDRESS[ADDRESS].MINER_DATA.size() == 0)
    {
        const ConnectionData &lConnectionData = CONNECTIONS_BY_ADDRESS[ADDRESS];

        if( STATSPERSISTER != NULL)
            STATSPERSISTER->SaveConnectionData(lConnectionData); 
            
        CONNECTIONS_BY_ADDRESS[ADDRESS].LASTSAVETIMER.Reset();

    }
    CONNECTIONS_BY_ADDRESS_MUTEX.unlock();
}

int StatsCollector::GetConnectionCount( std::string ADDRESS )
{
    int nCount = 0;
    CONNECTIONS_BY_ADDRESS_MUTEX.lock();
    nCount = CONNECTIONS_BY_ADDRESS[ADDRESS].MINER_DATA.size();
    CONNECTIONS_BY_ADDRESS_MUTEX.unlock();

    return nCount;
}

void StatsCollector::UpdateConnectionData(std::string ADDRESS, std::string GUID, double PPS, double WPS)
{
    CONNECTIONS_BY_ADDRESS_MUTEX.lock();
    {
        CONNECTIONS_BY_ADDRESS[ADDRESS].ADDRESS = ADDRESS;
        CONNECTIONS_BY_ADDRESS[ADDRESS].MINER_DATA[GUID].PPS = PPS;
        CONNECTIONS_BY_ADDRESS[ADDRESS].MINER_DATA[GUID].WPS = WPS;
        
        // save to DB every 10 s
        if( CONNECTIONS_BY_ADDRESS[ADDRESS].LASTSAVETIMER.Elapsed() >= Core::CONFIG.nSaveConnectionStatsFrequency )
        {
            const ConnectionData &lConnectionData = CONNECTIONS_BY_ADDRESS[ADDRESS];
            
            if( STATSPERSISTER != NULL)
                STATSPERSISTER->SaveConnectionData(lConnectionData); 
            
            CONNECTIONS_BY_ADDRESS[ADDRESS].LASTSAVETIMER.Reset();
        }
    }
    CONNECTIONS_BY_ADDRESS_MUTEX.unlock();
}

double StatsCollector::GetAccountPPS(std::string ADDRESS)
{
    return CONNECTIONS_BY_ADDRESS[ADDRESS].GetTotalPPS();
}

double StatsCollector::GetAccountWPS(std::string ADDRESS)
{
    return CONNECTIONS_BY_ADDRESS[ADDRESS].GetTotalWPS();
}

void StatsCollector::UpdatePoolData()
{
    POOL_DATA_MUTEX.lock();
    {
        PoolData lCurrentPoolData; // auto populates with current values in constructor
        if( STATSPERSISTER != NULL)
            STATSPERSISTER->UpdatePoolData( lCurrentPoolData);

    }
    POOL_DATA_MUTEX.unlock();
    

}

void StatsCollector::UpdateAccountData(bool bUpdateAll /*= false*/)
{
    uint64 nTotalWeight = Core::TotalWeight();

    std::vector<std::string> vKeys = Core::AccountDB.GetKeys();
    for(int nIndex = 0; nIndex < vKeys.size(); nIndex++)
    {
        std::string strAddress = vKeys[nIndex];

        // don't update data of they dont have any current connections
        int lConnections = GetConnectionCount(strAddress);

        if(lConnections > 0 || bUpdateAll)
        {
            LLD::Account cAccount = Core::AccountDB.GetRecord(strAddress);

            AccountData lAccountData;
            lAccountData.strAccountAddress = strAddress;
            lAccountData.nConnections = lConnections;
            lAccountData.nShares = cAccount.nRoundShares;

            lAccountData.nBalance = cAccount.nAccountBalance;
            lAccountData.nPendingPayout = 0;

            if(Core::cGlobalCoinbase.mapOutputs.count(strAddress))
                    lAccountData.nPendingPayout = Core::cGlobalCoinbase.mapOutputs[strAddress];
            
            if( STATSPERSISTER != NULL)
                STATSPERSISTER->UpdateAccountData( lAccountData);
        }
    }

}

void StatsCollector::ClearAccountRoundShares()
{
    if( STATSPERSISTER != NULL)
        STATSPERSISTER->ClearAccountRoundShares();
}

void StatsCollector::SaveCurrentRound()
{           
    ROUND_HISTORY_MUTEX.lock();
    {
        RoundData lCurrentRound; // constructs with current round data by default
        if( STATSPERSISTER != NULL)
            STATSPERSISTER->AddRoundData(lCurrentRound);
    }    
    ROUND_HISTORY_MUTEX.unlock();
}

void StatsCollector::FlagRoundAsOrphan(unsigned int nRound)
{
    ROUND_HISTORY_MUTEX.lock();
    {
        RoundData lOrphanRound(nRound);
        STATSPERSISTER->FlagRoundAsOrphan(lOrphanRound);
    }    
    ROUND_HISTORY_MUTEX.unlock();


}

void StatsCollector::AddAccountEarnings( std::string strAccountAddress,
                                unsigned int nRound,
                                unsigned int nBlockNumber,
                                uint64 nShares,
                                uint64 nAmountEarned,
                                time_t tTime)
{

    ACCOUNT_EARNINGS_MUTEX.lock();
    {
        AccountEarningTransaction lAccountEarningTransaction;
        lAccountEarningTransaction.strAccountAddress = strAccountAddress;
        lAccountEarningTransaction.nRound = nRound;
        lAccountEarningTransaction.nBlockNumber = nBlockNumber;
        lAccountEarningTransaction.nShares = nShares;
        lAccountEarningTransaction.nAmountEarned = nAmountEarned;
        lAccountEarningTransaction.tTime = tTime;
        
        if( STATSPERSISTER != NULL)
            STATSPERSISTER->AddAccountEarnings(lAccountEarningTransaction);
    }
    ACCOUNT_EARNINGS_MUTEX.unlock(); 
   
}
void StatsCollector::DeleteAccountEarnings( std::string strAccountAddress,
                                unsigned int nRound)
{

    ACCOUNT_EARNINGS_MUTEX.lock();
    {
        AccountEarningTransaction lAccountEarningTransaction;
        lAccountEarningTransaction.strAccountAddress = strAccountAddress;
        lAccountEarningTransaction.nRound = nRound;

        if( STATSPERSISTER != NULL)
            STATSPERSISTER->DeleteAccountEarnings(lAccountEarningTransaction);
    }
    ACCOUNT_EARNINGS_MUTEX.unlock(); 

}

void StatsCollector::AddAccountPayment( std::string strAccountAddress,
                                unsigned int nRound,
                                unsigned int nBlockNumber,
                                uint64 nAmountPaid,
                                time_t tTime)
{

    ACCOUNT_EARNINGS_MUTEX.lock();
    {
        AccountPaymentTransaction lAccountPaymentTransaction;
        lAccountPaymentTransaction.strAccountAddress = strAccountAddress;
        lAccountPaymentTransaction.nRound = nRound;
        lAccountPaymentTransaction.nBlockNumber = nBlockNumber;
        lAccountPaymentTransaction.nAmountPaid = nAmountPaid;
        lAccountPaymentTransaction.tTime = tTime;
        
        if( STATSPERSISTER != NULL)
            STATSPERSISTER->AddAccountPayment(lAccountPaymentTransaction);
    }
    ACCOUNT_EARNINGS_MUTEX.unlock(); 
   
}
void StatsCollector::DeleteAccountPayment( std::string strAccountAddress,
                                unsigned int nRound)
{

    ACCOUNT_EARNINGS_MUTEX.lock();
    {
        AccountPaymentTransaction lAccountPaymentTransaction;
        lAccountPaymentTransaction.strAccountAddress = strAccountAddress;
        lAccountPaymentTransaction.nRound = nRound;

        if( STATSPERSISTER != NULL)
            STATSPERSISTER->DeleteAccountPayment(lAccountPaymentTransaction);
    }
    ACCOUNT_EARNINGS_MUTEX.unlock(); 

}

/*
std::string StatsCollector::GetAccountEarnings()
{
    json_spirit::Object oJSONData;
    json_spirit::Array oAccountEarnings;
    

    for(AccountEarningIterator lAccountEarningIterator = ACCOUNT_EARNINGS.begin();  lAccountEarningIterator != ACCOUNT_EARNINGS.end(); lAccountEarningIterator++)
    {
        json_spirit::Object oAccountEarningData;
        json_spirit::Array oAccountEarningsTransaction;
        oAccountEarningData.push_back( Pair( "address", lAccountEarningIterator->first ) );
        // note we use rbegin /rend here to iterate backwards, so that we get the highest/most recent round first
        for(AccountEarningTransactionIterator lAccountEarningTransactionIterator = lAccountEarningIterator->second.rbegin();  lAccountEarningTransactionIterator != lAccountEarningIterator->second.rend(); lAccountEarningTransactionIterator++)
        {
            json_spirit::Object oEarningData;

            oEarningData.push_back( Pair( "round", (uint64_t)lAccountEarningTransactionIterator->second.nRound ) );
            oEarningData.push_back( Pair( "block", (uint64_t)lAccountEarningTransactionIterator->second.nBlockNumber ) );
            oEarningData.push_back( Pair( "shares", (uint64_t)lAccountEarningTransactionIterator->second.nShares ) );
            oEarningData.push_back( Pair( "total_shares", (uint64_t)lAccountEarningTransactionIterator->second.nTotalShares ) );
            oEarningData.push_back( Pair( "amount_earned", (uint64_t)lAccountEarningTransactionIterator->second.nAmountEarned ) );
            oEarningData.push_back( Pair( "time", lAccountEarningTransactionIterator->second.strTime ) );

            oAccountEarningsTransaction.push_back(oEarningData);
        }

        oAccountEarningData.push_back(Pair( "earnings", oAccountEarningsTransaction)); 

        oAccountEarnings.push_back(oAccountEarningData) ;  

    }

    oJSONData.push_back( Pair( "account_earnings", oAccountEarnings ) );

    return json_spirit::write_string<>(json_spirit::Value(oJSONData), true);

}
*/

/*std::string StatsCollector::GetRoundHistory()
{

    Object oJSONData;
    json_spirit::Array oRoundHistory;

    for(std::vector<RoundData>::reverse_iterator nIterator = ROUND_HISTORY.rbegin(); nIterator != ROUND_HISTORY.rend(); nIterator++)
    {
        json_spirit::Object oRoundHistoryData;
        RoundData lRoundData = *nIterator;

        oRoundHistoryData.push_back( Pair( "round", (uint64_t)lRoundData.nRound ) );
        oRoundHistoryData.push_back( Pair( "block_number", (uint64_t)lRoundData.nBlockNumber ) );
        oRoundHistoryData.push_back( Pair( "block_hash", lRoundData.hashBlock ) );
        oRoundHistoryData.push_back( Pair( "round_reward", (uint64_t)lRoundData.nRoundReward ) );
        oRoundHistoryData.push_back( Pair( "block_finder", lRoundData.strBlockFinder ) );
        oRoundHistoryData.push_back( Pair( "orphan", lRoundData.bOrphan) );
        oRoundHistoryData.push_back( Pair( "block_found_time", lRoundData.strBlockFoundTime ) );

	    oRoundHistory.push_back(oRoundHistoryData );
    }
    oJSONData.push_back( Pair( "round_history", oRoundHistory ) );

    return json_spirit::write_string<>(json_spirit::Value(oJSONData), true);
}*/

/*
std::string StatsCollector::GetPoolStats()
{
    std::string strJSON;

    Object oJSONData;
    oJSONData.push_back( Pair( "block_number", (uint64_t)Core::nBestHeight ) );
    //PS TODO diff not known, need to get from wallet
    oJSONData.push_back( Pair( "difficulty", 0 ) );
    oJSONData.push_back( Pair( "reward", (uint64_t)Core::nRoundReward ) );
    oJSONData.push_back( Pair( "round", (uint64_t)Core::nCurrentRound ) );

    oJSONData.push_back( Pair( "last_block_found", (uint64_t)Core::nLastBlockFound ) );
    oJSONData.push_back( Pair( "last_round_block_finder", (std::string)Core::LAST_ROUND_BLOCKFINDER ) );

    unsigned int lSecondsSinceLastBlock = Core::LAST_BLOCK_FOUND_TIMER.Elapsed();
    unsigned int lMinutes = lSecondsSinceLastBlock > 60 ? lSecondsSinceLastBlock / 60 : 0;
    unsigned int lSeconds = lSecondsSinceLastBlock % 60;

    oJSONData.push_back( Pair( "time_since_block", str(boost::format("%dm %ds") % lMinutes % lSeconds )) );
    
    oJSONData.push_back( Pair( "active_connections", (uint64_t)Core::nConnections ) );

    json_spirit::Array oNextPayments;

    for(std::map<std::string, uint64>::iterator nIterator = Core::cGlobalCoinbase.mapOutputs.begin(); nIterator != Core::cGlobalCoinbase.mapOutputs.end(); nIterator++)
    {
        json_spirit::Object oNextPayment;
        oNextPayment.push_back( Pair( "address", nIterator->first.c_str() ) );
        oNextPayment.push_back( Pair( "amount", (uint64_t) nIterator->second ) );
	    oNextPayments.push_back(oNextPayment );
    }
    oJSONData.push_back( Pair( "next_payments", oNextPayments ) );

    return json_spirit::write_string<>(json_spirit::Value(oJSONData), true);
}
*/

/*
json_spirit::Object StatsCollector::GetAccountDataJSON(std::string strAddress, uint64 nTotalWeight )
{
    json_spirit::Object oAccountData;
    if( Core::AccountDB.HasKey(strAddress))
    {
        LLD::Account cAccount = Core::AccountDB.GetRecord(strAddress);
        unsigned int nBalance = cAccount.nAccountBalance;
        unsigned int nPendingPayout = 0;

        if(Core::cGlobalCoinbase.mapOutputs.count(strAddress))
				nPendingPayout = Core::cGlobalCoinbase.mapOutputs[strAddress];

        uint64 nRoundshares = cAccount.nRoundShares;

        int lConnections = GetConnectionCount(strAddress);
        double lPPS = GetAccountPPS(strAddress);
        double lWPS = GetAccountWPS(strAddress);

        oAccountData.push_back( Pair( "address", strAddress ) );
        oAccountData.push_back( Pair( "connections", (uint64_t) lConnections ) );
        oAccountData.push_back( Pair( "pps", lPPS ) );
        oAccountData.push_back( Pair( "wps", lWPS ) );
        oAccountData.push_back( Pair( "round_shares", (uint64_t) nRoundshares ) );
        oAccountData.push_back( Pair( "total_round_shares", (uint64_t) nTotalWeight ) );
        oAccountData.push_back( Pair( "balance", (uint64_t) nBalance ) );
        oAccountData.push_back( Pair( "pending_payout", (uint64_t) nPendingPayout ) );
    }

    return oAccountData;
}
*/

/*
std::string StatsCollector::GetAccountData()
{
    json_spirit::Object oJSONData;
    json_spirit::Array oAccountStats;
    json_spirit::Object oAccountData;

    uint64 nTotalWeight = Core::TotalWeight();

    std::vector<std::string> vKeys = Core::AccountDB.GetKeys();
    for(int nIndex = 0; nIndex < vKeys.size(); nIndex++)
    {
        std::string strAddress = vKeys[nIndex];
        
        oAccountData = GetAccountDataJSON( strAddress, nTotalWeight); 
        oAccountStats.push_back(oAccountData );
    }

    oJSONData.push_back( Pair( "account_data", oAccountStats ) );

    return json_spirit::write_string<>(json_spirit::Value(oJSONData), true);
}
*/

/** Address, number of connections, round shares, balance**/
/*
std::string StatsCollector::GetSingleAccountData(std::string strAddress)
{
    json_spirit::Object oAccountData = GetAccountDataJSON( strAddress, Core::TotalWeight()); 

    return json_spirit::write_string<>(json_spirit::Value(oAccountData), true);
}
*/
// map of account address to vector of earnings



}