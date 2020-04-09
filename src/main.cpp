#include "core.h"
#include "coinbase.h"
#include "LLP/types.h"
#include "statscollector.h"

#include <iostream>


int main(int argc, char *argv[])
{
	Core::CONFIG.ReadConfig();

	Core::WALLET_IP_ADDRESS = Core::CONFIG.strWalletIP;
	Core::WALLET_PORT = to_string(Core::CONFIG.nWalletPort);

	// before starting the pool we need to initialise the statistics collector class
	Core::STATSCOLLECTOR.Init();

	Core::StartPool(Core::CONFIG.nPort, Core::CONFIG.nDaemonThreads, Core::CONFIG.nPoolThreads, Core::CONFIG.bDDOS, Core::CONFIG.rScore, Core::CONFIG.cScore, Core::CONFIG.nShare, Core::CONFIG.nPoolFee);
	
	loop { LLP::Sleep(10); }

	std::cout << "Exiting" << std::endl;
	
	return 0;
}
