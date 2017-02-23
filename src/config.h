#ifndef NEXUS_POOL_CONFIG_H
#define NEXUS_POOL_CONFIG_H
#include <string>

namespace Core
{
    class Config
    {
    public:
        Config();

        bool ReadConfig();
        void PrintConfig();

        std::string  strWalletIP;
        int          nPort;
        int          nDaemonThreads;
        int          nPoolThreads;
        bool         bDDOS;
        int          rScore;
        int          cScore;
        int          nShare;
        int          nPoolFee;

        std::string  strStatsDBServerIP;
        int          nStatsDBServerPort;
        std::string  strStatsDBUsername;
        std::string  strStatsDBPassword;
        // number of second between saves for each account
        int          nSaveConnectionStatsFrequency; 
        // number of seconds between each time series for connection stats.
        // this defaults to 300 seconds, which means that even though we capture the realtime stats
        // every nSaveConnectionStatsFrequency seconds, we capture the historical stats every 300 
        int          nSaveConnectionStatsSeriesFrequency; //

    };

}


#endif