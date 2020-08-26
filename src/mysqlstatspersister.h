#ifndef NEXUS_MYSQL_STATS_PERSISTER_H
#define NEXUS_MYSQL_STATS_PERSISTER_H

#include "statspersister.h"
#include "statscollector.h"
#include <mysql/mysql.h>

namespace Core
{
    class MySQLStatsPersister : public StatsPersister
    {
    public:

        MySQLStatsPersister();
        ~MySQLStatsPersister();

        /** Initialises the connection to the exteral persistence store**/
        virtual void Init();
        virtual void Destroy();

        void RunQuery(std::string query);

        //pool data
        virtual void UpdatePoolData( const PoolData& lPoolData);
        
        // round data
        virtual void AddRoundData( const RoundData& lRoundData);
        virtual void FlagRoundAsOrphan(const RoundData& lRoundData);

        // account data
        /** Should Insert or Update the account data **/
        virtual void UpdateAccountData( const AccountData& lAccountData);
        /** Should set the round shares for all accounts to 0**/
        virtual void ClearAccountRoundShares( );
        virtual void SaveConnectionData(const ConnectionData &lConnectionData);

        // account earnings
        /** This maybe called multiple times for a given round (in the case of additional bonuses) **/ 
        virtual void AddAccountEarnings( const AccountEarningTransaction& lAccountEarningTransaction);
        virtual void DeleteAccountEarnings( const AccountEarningTransaction& lAccountEarningTransaction);

        // account payments
        virtual void AddAccountPayment( const AccountPaymentTransaction& lAccountPaymentTransaction);
        virtual void DeleteAccountPayment( const AccountPaymentTransaction& lAccountPaymentTransaction);

    protected:

        virtual void UpdateSchema();

        /** Connection to MySQL **/
        MYSQL *pMYSQLConnection;

        std::mutex QUERY_MUTEX;
 
    };
}

#endif