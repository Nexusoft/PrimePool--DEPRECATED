#include "statistics.h"
#include "util.h"
#include "core.h"
#include "LLP/types.h"
#include "LLD/record.h"
#include "json/json_spirit_reader_template.h"
#include "json/json_spirit_writer_template.h"
#include "json/json_spirit_utils.h"
#include <boost/format.hpp>

using namespace json_spirit;
using namespace std;

namespace Core
{

/** The last X blocks found by the pool **/
std::string Statistics::GetBlockHistory(int nNumBlocks)
{
    std::string strJSON;

    return strJSON;
}

/** Global pool stats including 
    block number, 
    difficulty,
    reward, 
    round number, 
    last block found,
    last_round_block_finder, 
    time since last block,
    number of active connections,
    next payouts (from coinbase) **/
std::string Statistics::GetPoolStats()
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
    json_spirit::Object oNextPayment;

    for(std::map<std::string, uint64>::iterator nIterator = Core::cGlobalCoinbase.mapOutputs.begin(); nIterator != Core::cGlobalCoinbase.mapOutputs.end(); nIterator++)
    {
        oNextPayment.push_back( Pair( "address", nIterator->first.c_str() ) );
        oNextPayment.push_back( Pair( "amount", (uint64_t) nIterator->second ) );
	    oNextPayments.push_back(oNextPayment );
    }
    oJSONData.push_back( Pair( "next_payments", oNextPayments ) );

    return json_spirit::write_string<>(json_spirit::Value(oJSONData), true);
}


json_spirit::Object Statistics::GetAccountDataJSON(std::string strAddress )
{
    json_spirit::Object oAccountData;
    if( Core::AccountDB.HasKey(strAddress))
    {
        LLD::Account cAccount = Core::AccountDB.GetRecord(strAddress);
        unsigned int nBalance = cAccount.nAccountBalance;
        unsigned int nPendingPayout = 0;

        if(Core::cGlobalCoinbase.mapOutputs.count(strAddress))
				nPendingPayout = Core::cGlobalCoinbase.mapOutputs[strAddress];

        unsigned int nRoundshares = cAccount.nRoundShares;

        int lConnections = GetConnectionCount(strAddress);

        oAccountData.push_back( Pair( "address", strAddress ) );
        oAccountData.push_back( Pair( "connections", (uint64_t) nConnections ) );
        oAccountData.push_back( Pair( "round_shares", (uint64_t) nRoundshares ) );
        oAccountData.push_back( Pair( "balance", (uint64_t) nBalance ) );
        oAccountData.push_back( Pair( "pending_payout", (uint64_t) nPendingPayout ) );
    }

    return oAccountData;
}

std::string Statistics::GetAccountData()
{
    json_spirit::Object oJSONData;
    json_spirit::Array oAccountStats;
    json_spirit::Object oAccountData;

    std::vector<std::string> vKeys = Core::AccountDB.GetKeys();
    for(int nIndex = 0; nIndex < vKeys.size(); nIndex++)
    {
        std::string strAddress = vKeys[nIndex];
        
        oAccountData = GetAccountDataJSON( strAddress); 
        oAccountStats.push_back(oAccountData );
    }

    oJSONData.push_back( Pair( "account_data", oAccountStats ) );

    return json_spirit::write_string<>(json_spirit::Value(oJSONData), true);
}

/** Address, number of connections, round shares, balance**/
std::string Statistics::GetSingleAccountData(std::string strAddress)
{
    json_spirit::Object oAccountData = GetAccountDataJSON( strAddress); 

    return json_spirit::write_string<>(json_spirit::Value(oAccountData), true);
}











}