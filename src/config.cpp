
#include "config.h"
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>

#include <fstream>

namespace pt = boost::property_tree;

namespace Core
{
    Config::Config()
    {
        strWalletIP = "127.0.0.1";
        nWalletPort = 9325;
        nPort = 9549;
        fTestNet = false;

        nDaemonThreads = 10;
        nPoolThreads = 20;
        bDDOS = true;
        rScore = 20;
        cScore = 2;
        nShare = 40000000;
        nPoolFee = 1;

        strStatsDBServerIP = "127.0.0.1";
        nStatsDBServerPort = 3306;
        strStatsDBUsername = "mysql";
        strStatsDBPassword = "mysql";
        nSaveConnectionStatsFrequency = 10;
        nSaveConnectionStatsSeriesFrequency = 300;

    }

    void Config::PrintConfig()
	{
		printf("Configuration: \n");
		printf("-------------\n");
        printf("TestNet: %i \n", fTestNet);
		printf("Wallet IP: %s \n", strWalletIP.c_str());
        printf("Wallet Port: %i \n", nWalletPort);
		printf("Port: %i \n", nPort);

        printf("Daemon Threads: %i \n", nDaemonThreads);
		printf("Pool Threads: %i \n", nPoolThreads);

        printf("bDDOS: %i \n", bDDOS);
        printf("rScore: %i \n", rScore);
        printf("cScore: %i \n", cScore);
        printf("Min Share Diff: %i \n", nShare);
        printf("Pool Fee: %i \n", nPoolFee);

        printf("Stats DB Server IP: %s \n", strStatsDBServerIP.c_str());
		printf("Stats DB Port: %i \n", nStatsDBServerPort);
        printf("Stats DB Username: %s \n", strStatsDBUsername.c_str());
        printf("Stats DB Password: %s \n", strStatsDBPassword.c_str());
        printf("Save Connection Stats Frequency: %i seconds \n", nSaveConnectionStatsFrequency);
        printf("Save Connection Stats Series Frequency: %i seconds \n", nSaveConnectionStatsSeriesFrequency);

        printf("-------------\n");

	}

    bool Config::ReadConfig()
    {
        bool bSuccess = true;

        printf("Reading config file pool.conf\n");

        std::ifstream lConfigFile("pool.conf");

        pt::ptree root;
        pt::read_json("pool.conf", root);

        fTestNet = root.get<bool>("testnet");
        strWalletIP = root.get<std::string>("wallet_ip");
        nWalletPort = root.get<int>("wallet_port");
        nPort = root.get<int>("port");

        nDaemonThreads = root.get<int>("daemon_threads");
        nPoolThreads = root.get<int>("pool_threads");
        bDDOS = root.get<bool>("enable_ddos");
        rScore = root.get<int>("ddos_rscore");
        cScore = root.get<int>("ddos_cscore");
        nShare = root.get<int>("min_share");
        nPoolFee = root.get<int>("pool_fee");

        strStatsDBServerIP = root.get<std::string>("stats_db_server_ip");
        nStatsDBServerPort = root.get<int>("stats_db_server_port");
        strStatsDBUsername = root.get<std::string>("stats_db_username");
        strStatsDBPassword = root.get<std::string>("stats_db_password");
        nSaveConnectionStatsFrequency = root.get<int>("connection_stats_frequency");
        nSaveConnectionStatsSeriesFrequency = root.get<int>("connection_stats_series_frequency");

        PrintConfig();
        // TODO Need to add exception handling here and set bSuccess appropriately
        return bSuccess;
    }


} // end namespace
