#include "ddos.h"
#include <iostream>
#include <algorithm>

namespace LLP
{
    DDOS_Score::DDOS_Score(int nTimespan)
    {
		for (int i = 0; i < nTimespan; i++)
			SCORE.push_back(std::make_pair(true, 0));

          TIMER.Start();
          nIterator = 0;
    }

    int DDOS_Score::Score() const
    {
        int nMovingAverage = 0;
        for (auto const score : SCORE)
            nMovingAverage += score.second;

		return nMovingAverage / (int)SCORE.size();
    }

	void DDOS_Score::Flush()
	{
		for(auto score : SCORE)
			score.second = 0;
	}

    DDOS_Score &DDOS_Score::operator++(int)
    {
        int nTime = TIMER.Elapsed();
        if (nTime >= SCORE.size())
        {
            Reset();
            nTime -= (int)SCORE.size();
        }


        for (int i = nIterator; i <= nTime; i++)
        {
            if (!SCORE[i].first)
            {
                SCORE[i].first = true;
                SCORE[i].second = 0;
            }
        }

        SCORE[nTime].second++;
        nIterator = nTime;

        return *this;
    }

	DDOS_Score &DDOS_Score::operator+=(int nScore)
	{		
		int nTime = TIMER.Elapsed();		
		
		/** If the Time has been greater than Moving Average Timespan, Set to Add Score on Time Overlap. **/
		if(nTime >= SCORE.size())
		{
			Reset();
			nTime = nTime % SCORE.size();
		}			
			
		/** Iterate as many seconds as needed, flagging that each has been iterated. **/
		for(int i = nIterator; i <= nTime; i++)
		{
			if(!SCORE[i].first)
			{
				SCORE[i].first  = true;
				SCORE[i].second = 0;
			}
		}		
		
		/** Update the Moving Average Iterator and Score for that Second Instance. **/
		SCORE[nTime].second += nScore;
		nIterator = nTime;		
		
		return *this;
	}


    void DDOS_Score::Reset()
    {
		for (auto score : SCORE)
		score.first = false;

		TIMER.Reset();
		nIterator = 0;
    }

    DDOS_Filter::DDOS_Filter(unsigned int nTimespan, std::string strIPAddress)
        : rSCORE(nTimespan)
        , cSCORE(nTimespan)
		, IPADDRESS{std::move(strIPAddress)}
        , BANTIME(0)
        , TOTALBANS(0) { }

	void DDOS_Filter::Ban(std::string const& strMessage)
	{
		if(Banned())
			return;
		
		TIMER.Start();
		TOTALBANS++;
		
		BANTIME = std::max(TOTALBANS * (rSCORE.Score() + 1) * (cSCORE.Score() + 1), TOTALBANS * 1200u);
		
		std::cout << "XXXXX DDOS Filter Address = " << IPADDRESS << " cScore = " << cSCORE.Score() << " rScore = " << rSCORE.Score() <<
			" Banned for " << BANTIME << " Seconds. Violation: " << strMessage << std::endl;
		
		cSCORE.Flush();
		rSCORE.Flush();
	}

    bool DDOS_Filter::Banned()
    {
        return (TIMER.Elapsed() < BANTIME);
    }
}
