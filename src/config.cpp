
#include "config.h"
#include "json/json.hpp"

using json = nlohmann::json;

#include <fstream>
#include <iostream>

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
        nDDOS = true;
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
		std::cout << "Configuration: " << std::endl;
		std::cout << "-------------" << std::endl;
		std::cout << "TestNet: " << fTestNet << std::endl;
		std::cout << "Wallet IP: " << strWalletIP << std::endl;
		std::cout << "Wallet Port: " << nWalletPort << std::endl;
		std::cout << "Port: " << nPort << std::endl;

		std::cout << "Daemon Threads: " << nDaemonThreads << std::endl;
		std::cout << "Pool Threads: " << nPoolThreads << std::endl;

		std::cout << "nDDOS: " << nDDOS << std::endl;;
		std::cout << "rScore: " << rScore << std::endl;
		std::cout << "cScore: " << cScore << std::endl;
		std::cout << "Min Share Diff: " << nShare << std::endl;
		std::cout << "Pool Fee: " << nPoolFee << std::endl;

		std::cout << "Stats DB Server IP: " << strStatsDBServerIP << std::endl;
		std::cout << "Stats DB Port: " << nStatsDBServerPort << std::endl;
		std::cout << "Stats DB Username: " << strStatsDBUsername << std::endl;
		std::cout << "Stats DB Password: " << strStatsDBPassword << std::endl;
		std::cout << "Save Connection Stats Frequency: " << nSaveConnectionStatsFrequency << " seconds"  << std::endl;
		std::cout << "Save Connection Stats Series Frequency: " << nSaveConnectionStatsSeriesFrequency << " seconds"  << std::endl;

		std::cout << "-------------" << std::endl;

	}

    bool Config::ReadConfig()
    {
        bool bSuccess = true;

        std::cout << "Reading config file pool.conf" << std::endl;

        std::ifstream lConfigFile("pool.conf");

		json j = json::parse(lConfigFile);
		j.at("testnet").get_to(fTestNet);
		j.at("wallet_ip").get_to(strWalletIP);
		j.at("wallet_port").get_to(nWalletPort);
		j.at("port").get_to(nPort);

		j.at("daemon_threads").get_to(nDaemonThreads);
		j.at("pool_threads").get_to(nPoolThreads);
		j.at("enable_ddos").get_to(nDDOS);
		j.at("ddos_rscore").get_to(rScore);
		j.at("ddos_cscore").get_to(cScore);
		j.at("min_share").get_to(nShare);
		j.at("pool_fee").get_to(nPoolFee);

		j.at("stats_db_server_ip").get_to(strStatsDBServerIP);
		j.at("stats_db_server_port").get_to(nStatsDBServerPort);
		j.at("stats_db_username").get_to(strStatsDBUsername);
		j.at("stats_db_password").get_to(strStatsDBPassword);
		j.at("connection_stats_frequency").get_to(nSaveConnectionStatsFrequency);
		j.at("connection_stats_series_frequency").get_to(nSaveConnectionStatsSeriesFrequency);

        PrintConfig();
        // TODO Need to add exception handling here and set bSuccess appropriately
        return bSuccess;
    }


} // end namespace
