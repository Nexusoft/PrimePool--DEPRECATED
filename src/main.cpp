#include "core.h"
#include "coinbase.h"
#include "LLP/types.h"


int main(int argc, char *argv[])
{
	
	int nDaemonThreads = boost::lexical_cast<int>(argv[1]);
	int nPoolThreads   = boost::lexical_cast<int>(argv[2]);
	int ddos           = boost::lexical_cast<int>(argv[3]);
	int rScore         = boost::lexical_cast<int>(argv[4]);
	int cScore         = boost::lexical_cast<int>(argv[5]);
	int nShare         = boost::lexical_cast<int>(argv[6]);
	
	Core::StartPool(nDaemonThreads, nPoolThreads, (bool)ddos, rScore, cScore, nShare);
	
	loop { Sleep(10); }
	
	return 0;
}
