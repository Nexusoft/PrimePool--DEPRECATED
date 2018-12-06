#include "core.h"
#include "coinbase.h"
#include "LLP/types.h"
#include "statscollector.h"

#include <iostream>


#ifndef OPENSSL_HACK
#define OPENSSL_HACK
FILE _iob[] = { *stdin, *stdout, *stderr };
extern "C" FILE * __cdecl __iob_func(void) { return _iob; }
#endif

int main(int argc, char *argv[])
{
	Core::CONFIG.ReadConfig();

	Core::WALLET_IP_ADDRESS = Core::CONFIG.strWalletIP;

	// before starting the pool we need to initialise the statistics collector class
	Core::STATSCOLLECTOR.Init();

	Core::StartPool(Core::CONFIG.nPort, Core::CONFIG.nDaemonThreads, Core::CONFIG.nPoolThreads, Core::CONFIG.bDDOS, Core::CONFIG.rScore, Core::CONFIG.cScore, Core::CONFIG.nShare, Core::CONFIG.nPoolFee);
	
	loop { LLP::Sleep(10); }

	std::cout << "Exiting" << std::endl;
	
	return 0;
}
