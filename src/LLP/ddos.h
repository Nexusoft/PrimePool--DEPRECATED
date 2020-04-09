#ifndef NEXUS_LLP_DDOS_H
#define NEXUS_LLP_DDOS_H

#include "timer.h"
#include <vector>
#include <string>

namespace LLP
{
    /** Class that tracks DDOS attempts on LLP Servers.
        Uses a Timer to calculate Request Score [rScore] and Connection Score [cScore] as a unit of Score / Second.
        Pointer stored by Connection class and Server Listener DDOS_MAP. **/
    class DDOS_Score
    {
    public:
        DDOS_Score(int nTimespan);

        int Score() const;

		/** Flush the DDOS Score to 0. **/
		void Flush();

        DDOS_Score &operator++(int);
		DDOS_Score &operator+=(int nScore);

    private:
        std::vector< std::pair<bool, int> > SCORE;
        Timer TIMER;
        int nIterator;

        void Reset();
    };


    class DDOS_Filter
    {
    public:
        DDOS_Score rSCORE, cSCORE;
		std::string IPADDRESS;

        DDOS_Filter(unsigned int nTimespan, std::string strIPAddress);

        void Ban(std::string const& strMessage = "Score Threshold Ban");
        bool Banned();
        
    private:
        Timer TIMER;
        unsigned int BANTIME, TOTALBANS;
    };
}

#endif
