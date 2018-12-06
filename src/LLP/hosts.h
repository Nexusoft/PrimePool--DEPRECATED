/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2018] ++

            (c) Copyright The Nexus Developers 2014 - 2018

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#ifndef NEXUS_LLP_INCLUDE_HOSTS_H
#define NEXUS_LLP_INCLUDE_HOSTS_H

#include <string>
#include <vector>

namespace LLP
{

    class CAddress;
    class CNetAddr;
    class CService;

    /* The DNS Lookup Routine to find the Nodes that are set as DNS seeds. */
    std::vector<CAddress> DNS_Lookup(std::vector<std::string> DNS_Seed);


    /* Standard Wrapper Function to Interact with cstdlib DNS functions. */
    bool LookupHost(const char *pszName, std::vector<CNetAddr>& vIP, uint32_t nMaxSolutions = 0, bool fAllowLookup = true);
    bool LookupHostNumeric(const char *pszName, std::vector<CNetAddr>& vIP, uint32_t nMaxSolutions = 0);
    bool Lookup(const char *pszName, CService& addr, int portDefault = 0, bool fAllowLookup = true);
    bool Lookup(const char *pszName, std::vector<CService>& vAddr, int portDefault = 0, bool fAllowLookup = true, uint32_t nMaxSolutions = 0);
    bool LookupNumeric(const char *pszName, CService& addr, int portDefault = 0);
}

#endif
