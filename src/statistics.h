#ifndef NEXUS_STATS_H
#define NEXUS_STATS_H

#include <string>
#include "json/json_spirit_reader_template.h"
#include "json/json_spirit_writer_template.h"
namespace Core
{

	/** Class used to generate JSON-formatted pool statistics **/
	class Statistics
	{
	public:

		/** The last X blocks found by the pool **/
		static std::string GetBlockHistory(int nNumBlocks);

		/** Global pool stats including 
			block number, 
			difficulty,
			reward, 
			round number, 
			last block found, 
			time since last block,
			number of active connections,
			next payouts (from coinbase) **/
		static std::string GetPoolStats();

		/** Address, number of connections, round shares, balance**/
		static std::string GetAccountData();

		/** Address, number of connections, round shares, balance**/
		static std::string GetSingleAccountData( std::string strAddress );

	private:
		static json_spirit::Object GetAccountDataJSON( std::string strAddress );
	};

}
#endif
