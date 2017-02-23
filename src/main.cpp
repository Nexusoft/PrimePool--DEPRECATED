#include "core.h"
#include "coinbase.h"
#include "LLP/types.h"
#include "statscollector.h"

int main(int argc, char *argv[])
{
	Core::CONFIG.ReadConfig();

	/*
	std::string strIP  = argv[1];
	int nPort          = boost::lexical_cast<int>(argv[2]);
	int nDaemonThreads = boost::lexical_cast<int>(argv[3]);
	int nPoolThreads   = boost::lexical_cast<int>(argv[4]);
	int ddos           = boost::lexical_cast<int>(argv[5]);
	int rScore         = boost::lexical_cast<int>(argv[6]);
	int cScore         = boost::lexical_cast<int>(argv[7]);
	int nShare         = boost::lexical_cast<int>(argv[8]);
    int nPoolFee       = boost::lexical_cast<int>(argv[9]);
	*/

	Core::WALLET_IP_ADDRESS = Core::CONFIG.strWalletIP;

	// before starting the pool we need to initialise the statistics collector class
	Core::STATSCOLLECTOR.Init();

	Core::StartPool(Core::CONFIG.nPort, Core::CONFIG.nDaemonThreads, Core::CONFIG.nPoolThreads, Core::CONFIG.bDDOS, Core::CONFIG.rScore, Core::CONFIG.cScore, Core::CONFIG.nShare, Core::CONFIG.nPoolFee);
	
	loop { Sleep(10); }

	printf("Exiting");
	
	return 0;
}
