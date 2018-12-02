#include "statscollector.h"
#include "util.h"
#include "core.h"
#include "LLP/types.h"
#include "LLD/record.h"
#include <boost/format.hpp>
#include <boost/archive/text_oarchive.hpp>
#include <boost/archive/text_iarchive.hpp>
#include <boost/serialization/vector.hpp>
#include <boost/serialization/map.hpp>

#include "mysqlstatspersister.h"

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

} // end namespace Core