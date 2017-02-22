#ifndef NEXUS_STATS_PERSISTER_H
#define NEXUS_STATS_PERSISTER_H

#include <string>
#include "LLP/types.h"
#include "config.h"
#include "core.h"

namespace Core
{
    /** Forward declare types used here **/
    class ConnectionData;
    class PoolData;
    class RoundData;
    class AccountData;
    class AccountEarningTransaction;
    class AccountPaymentTransaction;

    /** Abstract class that defines the common interface that all stats database connections should use**/
    class StatsPersister
    {
    public:

        StatsPersister(){}
        ~StatsPersister(){Destroy();}

        /** Initialises the connection to the exteral persistence store**/
        virtual void Init() {SERIESTIMER.Start(); tCurrentSeriesDateTime = time(0);;};
        virtual void Destroy(){};

        virtual void SaveConnectionData(const ConnectionData &lConnectionData) = 0;
        time_t GetCurrentSeriesDateTime()
        {
            // roll on the connection data series every 5 mins
            if( SERIESTIMER.Elapsed() >= Core::CONFIG.nSaveConnectionStatsSeriesFrequency)
            {
                tCurrentSeriesDateTime = time(0);
                SERIESTIMER.Reset();
            }

            return tCurrentSeriesDateTime;
        }
        
        //pool data
        virtual void UpdatePoolData( const PoolData& lPoolData) = 0;

        // round data
        virtual void AddRoundData( const RoundData& lRoundData) = 0;
        virtual void FlagRoundAsOrphan(const RoundData& lRoundData) = 0;

        // account data
        /** Should Insert or Update the account data **/
        virtual void UpdateAccountData( const AccountData& lAccountData) = 0;
        /** Should set the round shares for all accounts to 0**/
        virtual void ClearAccountRoundShares( ) = 0;

        // account earnings
        /** This maybe called multiple times for a given round (in the case of additional bonuses) **/ 
        virtual void AddAccountEarnings( const AccountEarningTransaction& lAccountEarningTransaction) = 0;
        virtual void DeleteAccountEarnings( const AccountEarningTransaction& lAccountEarningTransaction) = 0;

        // account payments
        virtual void AddAccountPayment( const AccountPaymentTransaction& lAccountPaymentTransaction) = 0;
        virtual void DeleteAccountPayment( const AccountPaymentTransaction& lAccountPaymentTransaction) = 0;

    protected:
        /** Used to define when the time series data should move on (every 5 mins) **/
		LLP::Timer SERIESTIMER;
        
        time_t      tCurrentSeriesDateTime;

    };
}

#endif