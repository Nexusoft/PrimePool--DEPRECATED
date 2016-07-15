#include "util.h"
#include "core.h"

static boost::mutex MUTEX;
static FILE* fileout = NULL;


int ConsoleOutput(const char* pszFormat, ...)
{
	MUTEX.lock();
	int ret = 0;
	
	if (!fileout)
    {
        boost::filesystem::path pathDebug = "debug.log";
        fileout = fopen(pathDebug.string().c_str(), "a");
        if (fileout) setbuf(fileout, NULL); // unbuffered
    }
	
    if (fileout)
    {
        va_list arg_ptr;
        va_start(arg_ptr, pszFormat);
		ret = vprintf(pszFormat, arg_ptr);
        va_end(arg_ptr);
        
        va_start(arg_ptr, pszFormat);
		ret = vfprintf(fileout, pszFormat, arg_ptr);
        va_end(arg_ptr);
        
    }
	
	MUTEX.unlock();
	
	return ret;
}

std::vector<std::string> LoadBannedAccounts()
{
    //PS This should be loading once and then caching the list, reloading if the filetime has changed
    std::vector<std::string> lBannedAccounts;
    std::ifstream lBannedAccountsFile("banned_accounts.dat");
    std::string lLine;
    
    while(std::getline(lBannedAccountsFile, lLine))
    {
        lBannedAccounts.push_back(lLine);
        
    }
    
    return lBannedAccounts;
}

std::vector<std::string> LoadBannedIPAddresses()
{
    //PS This should be loading once and then caching the list, reloading if the filetime has changed
    std::vector<std::string> lBannedIPAddresses;
    std::ifstream lBannedIPAddressesFile("banned_ip_addresses.dat");
    std::string lLine;
    
    while(std::getline(lBannedIPAddressesFile, lLine))
    {
        lBannedIPAddresses.push_back(lLine);  
    }
    
    return lBannedIPAddresses;
}

void SaveBannedIPAddress(std::string ip_address)
{
    std::ofstream lBannedIPAddressesFile("banned_ip_addresses.dat", std::ios::out | std::ios::app);
    lBannedIPAddressesFile << ip_address + "\n";
    lBannedIPAddressesFile.close();
}