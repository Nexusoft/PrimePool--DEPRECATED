#include "mysqlstatspersister.h"


namespace Core
{

MySQLStatsPersister::MySQLStatsPersister()
    : StatsPersister()
{
    pMYSQLConnection = NULL;

}

MySQLStatsPersister::~MySQLStatsPersister()
{
    if( pMYSQLConnection != NULL)
        mysql_close (pMYSQLConnection); 

}


/** Initialises the connection to the exteral persistence store**/
void MySQLStatsPersister::Init() 
{
    StatsPersister::Init();

    // create connection to mysql
    pMYSQLConnection=mysql_init(NULL);
    if (!pMYSQLConnection)
    {
        throw std::runtime_error("MySQL Initialization failed\n");
    }

    // set auto reconnect
    my_bool reconnect = 1;
    mysql_options(pMYSQLConnection, MYSQL_OPT_RECONNECT, &reconnect);
    
    if (mysql_real_connect(pMYSQLConnection, 
        Core::CONFIG.strStatsDBServerIP.c_str(), 
        Core::CONFIG.strStatsDBUsername.c_str(), 
        Core::CONFIG.strStatsDBPassword.c_str(), 
        NULL, // don't use nxspool database by default as it might not yet exist
        Core::CONFIG.nStatsDBServerPort,
        NULL,0))
    {
        printf("MySQL connection Succeeded\n");
    }
    else
    {
        printf( "MySQL Error: %s\n", mysql_error(pMYSQLConnection));
        throw std::runtime_error("MySQL connection failed.  Check server IP, port, and credentials\n");
    }

    // check to see if the schema needs creating or updating (first run)
    UpdateSchema();

    // change to correct DB
    RunQuery("USE nxspool;");
}

void MySQLStatsPersister::Destroy()
{
    StatsPersister::Destroy();

}

void MySQLStatsPersister::RunQuery(std::string query)
{
   try
   {
       QUERY_MUTEX.lock();
   
        MYSQL_RES *res_set;

        //mysql_query (pMYSQLConnection, "USE nxspool;");
        mysql_query (pMYSQLConnection, query.c_str());
        res_set = mysql_store_result(pMYSQLConnection);
        
        if(res_set )
            mysql_free_result( res_set );
        else if( *mysql_error(pMYSQLConnection) )
        {
            printf("MySQL Error: %s\n", mysql_error(pMYSQLConnection));
            printf("Query:  %s\n",query.c_str());
        }

        QUERY_MUTEX.unlock();
   }
   catch(...)
   {
       QUERY_MUTEX.unlock();
       throw;
   }

}

/** Checks to see if the schema exists or requires updating **/ 
void MySQLStatsPersister::UpdateSchema()
{
    MYSQL_RES *res_set;
    
    // crete DB if not exists
    RunQuery("CREATE DATABASE IF NOT EXISTS nxspool;");
    RunQuery("USE nxspool;");
    
    RunQuery("CREATE TABLE IF NOT EXISTS pool_data\
    (\
        id INT NOT NULL DEFAULT 0, \
        round_number INT NOT NULL,\
        block_number INT DEFAULT NULL,\
        round_reward INT DEFAULT NULL,\
        total_shares INT DEFAULT NULL,\
        connection_count INT DEFAULT NULL,\
	    PRIMARY KEY (id)\
    ) ENGINE=INNODB;");
    
    RunQuery("CREATE TABLE IF NOT EXISTS round_history\
    (\
        round_number INT NOT NULL,\
        block_number INT DEFAULT NULL,\
        block_hash VARCHAR(1024) NOT NULL,\
        round_reward INT DEFAULT NULL,\
        total_shares INT DEFAULT NULL,\
        block_finder VARCHAR(255) NOT NULL,\
        orphan INT DEFAULT NULL,\
        block_found_time DATETIME NOT NULL,\
        PRIMARY KEY (round_number)\
    ) ENGINE=INNODB;");
    
    RunQuery("CREATE TABLE IF NOT EXISTS connection_history\
    (\
        account_address VARCHAR(255) NOT NULL,\
        series_time DATETIME NOT NULL,\
        last_save_time DATETIME NOT NULL,\
        connection_count INT DEFAULT NULL,\
        pps DOUBLE DEFAULT NULL,\
        wps DOUBLE DEFAULT NULL,\
        PRIMARY KEY (account_address, series_time)\
    ) ENGINE=INNODB;");
    
    RunQuery("CREATE TABLE IF NOT EXISTS account_data\
    (\
        account_address VARCHAR(255) NOT NULL,\
        last_save_time DATETIME NOT NULL,\
        connection_count INT DEFAULT NULL,\
        round_shares INT DEFAULT NULL,\
        balance INT DEFAULT NULL,\
        pending_payout INT DEFAULT NULL,\
        PRIMARY KEY (account_address)\
    ) ENGINE=INNODB;");
   
    RunQuery("CREATE TABLE IF NOT EXISTS earnings_history\
    (\
        account_address VARCHAR(255) NOT NULL,\
        round_number INT NOT NULL ,\
        block_number INT DEFAULT NULL,\
        round_shares INT DEFAULT NULL,\
        amount_earned INT DEFAULT NULL,\
        datetime DATETIME NOT NULL,\
        KEY (account_address),\
        KEY (account_address, round_number)\
    ) ENGINE=INNODB;");
    
    RunQuery("CREATE TABLE IF NOT EXISTS payment_history\
    (\
        account_address VARCHAR(255) NOT NULL,\
        round_number INT NOT NULL ,\
        block_number INT DEFAULT NULL,\
        amount_paid INT DEFAULT NULL,\
        datetime DATETIME NOT NULL,\
        KEY (account_address),\
        KEY (account_address, block_number)\
    ) ENGINE=INNODB;");
   


}

void MySQLStatsPersister::UpdatePoolData( const PoolData& lPoolData)
{    
    std::string query = stdprintf("REPLACE INTO nxspool.pool_data VALUES (0, %i, %i, %i, %i, %i);", 
                                    lPoolData.nRound,
                                    lPoolData.nBlockNumber,
                                    lPoolData.nRoundReward,
                                    lPoolData.nTotalShares,
                                    lPoolData.nConnectionCount
                                    );
                            
    
    RunQuery( query );

}

void MySQLStatsPersister::SaveConnectionData(const ConnectionData &lConnectionData)
{
    std::string strSeriesDateTimeString = time2datetimestring(GetCurrentSeriesDateTime());
    std::string strNow = currentDateTime();

    std::string query = stdprintf("REPLACE INTO nxspool.connection_history VALUES ('%s', '%s', '%s', %i, %.4f, %.4f);", 
                                    lConnectionData.ADDRESS.c_str(),
                                    strSeriesDateTimeString.c_str(),
                                    strNow.c_str(),
                                    lConnectionData.MINER_DATA.size(),
                                    lConnectionData.GetTotalPPS(),
                                    lConnectionData.GetTotalWPS()
                                    );
                            
    
    RunQuery( query );

}

// round data
void MySQLStatsPersister::AddRoundData( const RoundData& lRoundData)
{
    std::string query = stdprintf("INSERT INTO nxspool.round_history VALUES (%i, %i, '%s',%i, %i, '%s', %i, '%s');", 
                                lRoundData.nRound,
                                lRoundData.nBlockNumber,
                                lRoundData.hashBlock.c_str(),
                                lRoundData.nRoundReward,
                                lRoundData.nTotalShares,
                                lRoundData.strBlockFinder.c_str(),
                                lRoundData.bOrphan,
                                time2datetimestring(lRoundData.tBlockFoundTime).c_str()
                                );
                            
    
    RunQuery( query );

}

void MySQLStatsPersister::FlagRoundAsOrphan(const RoundData& lRoundData)
{
    std::string query = stdprintf("UPDATE nxspool.round_history SET orphan = 1 WHERE round_number = %i;", lRoundData.nRound );
    RunQuery( query );
}

// account data
/** Should Insert or Update the account data **/
void MySQLStatsPersister::UpdateAccountData( const AccountData& lAccountData)
{
    std::string query = stdprintf("INSERT INTO nxspool.account_data (account_address, last_save_time, connection_count, round_shares, balance, pending_payout) \
                                    VALUES ('%s', '%s', %i, %i, %i, %i) \
                                    ON DUPLICATE KEY UPDATE \
                                    last_save_time = VALUES(last_save_time), \
                                    connection_count = VALUES(connection_count), \
                                    round_shares = VALUES(round_shares), \
                                    balance = VALUES(balance), \
                                    pending_payout = VALUES(pending_payout);", 
                                lAccountData.strAccountAddress.c_str(),
                                currentDateTime().c_str(),
                                lAccountData.nConnections,
                                lAccountData.nShares,
                                lAccountData.nBalance,
                                lAccountData.nPendingPayout
                                );
                            
    
    RunQuery( query );

}

void MySQLStatsPersister::ClearAccountRoundShares()
{
    std::string query = "UPDATE nxspool.account_data SET round_shares = 0 ;" ;
    RunQuery( query );
}


// account earnings
/** This maybe called multiple times for a given round (in the case of additional bonuses) **/ 
void MySQLStatsPersister::AddAccountEarnings( const AccountEarningTransaction& lAccountEarningTransaction )
{
    std::string query = stdprintf("INSERT INTO nxspool.earnings_history VALUES ('%s', %i, %i, %i, %i, '%s');", 
                                lAccountEarningTransaction.strAccountAddress.c_str(),
                                lAccountEarningTransaction.nRound,
                                lAccountEarningTransaction.nBlockNumber,
                                lAccountEarningTransaction.nShares,
                                lAccountEarningTransaction.nAmountEarned,
                                time2datetimestring(lAccountEarningTransaction.tTime).c_str()
                                );
                            
    
    RunQuery( query );

}

void MySQLStatsPersister::DeleteAccountEarnings( const AccountEarningTransaction& lAccountEarningTransaction)
{
    std::string query = stdprintf("DELETE FROM nxspool.earnings_history WHERE account_address='%s' AND round_number= %i;", 
                                lAccountEarningTransaction.strAccountAddress.c_str(),
                                lAccountEarningTransaction.nRound
                                );
                            
    RunQuery( query );

}

// account payments
void MySQLStatsPersister::AddAccountPayment( const AccountPaymentTransaction& lAccountPaymentTransaction)
{
    std::string query = stdprintf("INSERT INTO nxspool.payment_history VALUES ('%s', %i, %i, %i, '%s');", 
                                lAccountPaymentTransaction.strAccountAddress.c_str(),
                                lAccountPaymentTransaction.nRound,
                                lAccountPaymentTransaction.nBlockNumber,
                                lAccountPaymentTransaction.nAmountPaid,
                                time2datetimestring(lAccountPaymentTransaction.tTime).c_str()
                                );
                            
    
    RunQuery( query );

}

void MySQLStatsPersister::DeleteAccountPayment( const AccountPaymentTransaction& lAccountPaymentTransaction)
{
    std::string query = stdprintf("DELETE FROM nxspool.payment_history WHERE account_address='%s' AND round_number= %i;", 
                                lAccountPaymentTransaction.strAccountAddress.c_str(),
                                lAccountPaymentTransaction.nRound
                                );
                            
    
    RunQuery( query );
}


} // end namespace Core