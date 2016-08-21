#include "core.h"
#include "coinbase.h"
#include "LLP/types.h"


int main(int argc, char *argv[])
{
	int nPort          = boost::lexical_cast<int>(argv[1]);
	int nDaemonThreads = boost::lexical_cast<int>(argv[2]);
	int nPoolThreads   = boost::lexical_cast<int>(argv[3]);
	int ddos           = boost::lexical_cast<int>(argv[4]);
	int rScore         = boost::lexical_cast<int>(argv[5]);
	int cScore         = boost::lexical_cast<int>(argv[6]);
	int nShare         = boost::lexical_cast<int>(argv[7]);
    int nPoolFee       = boost::lexical_cast<int>(argv[8]);
	
	Core::StartPool(nPort, nDaemonThreads, nPoolThreads, (bool)ddos, rScore, cScore, nShare, nPoolFee);
	
	loop { Sleep(10); }
	
	return 0;
}
