#include "util.h"
#include "core.h"
#include <sys/types.h>
#include <sys/stat.h>

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
        // prepend timestamp
        std::cout << currentDateTime() << "\t";

        va_list arg_ptr;
        va_start(arg_ptr, pszFormat);
		ret = vprintf(pszFormat, arg_ptr);
        va_end(arg_ptr);

        // prepend timestamp
        fprintf(fileout, "%s\t", currentDateTime().c_str() );
        
        va_start(arg_ptr, pszFormat);
		ret = vfprintf(fileout, pszFormat, arg_ptr);
        va_end(arg_ptr);
        
    }
	
	MUTEX.unlock();
	
	return ret;
}

static std::vector<std::string> BANNED_ACCOUNTS; // these need to go somewhere better
static int BANNED_ACCOUNTS_LAST_MOD = 0;
static boost::mutex BANNED_ACCOUNTS_MUTEX;
void LoadBannedAccounts()
{
    BANNED_ACCOUNTS_MUTEX.lock();
    
    struct stat st;
    stat ("banned_accounts.dat", &st);
    int lLastMod = st.st_mtime;
    
    if( BANNED_ACCOUNTS.size() == 0 || lLastMod != BANNED_ACCOUNTS_LAST_MOD)
    {
        std::ifstream lBannedAccountsFile("banned_accounts.dat");
        std::string lLine;
        
        while(std::getline(lBannedAccountsFile, lLine))
        {
            BANNED_ACCOUNTS.push_back(lLine);
        }
        
        BANNED_ACCOUNTS_LAST_MOD = lLastMod;
    }
    
    BANNED_ACCOUNTS_MUTEX.unlock();
}

static std::vector<std::string> BANNED_IP_ADDRESSES;
static int BANNED_IP_ADDRESSES_LAST_MOD = 0;
static boost::mutex BANNED_IP_ADDRESSES_MUTEX;
void LoadBannedIPAddresses()
{
    BANNED_IP_ADDRESSES_MUTEX.lock();
    
    struct stat st;
    stat ("banned_ip_addresses.dat", &st);
    int lLastMod = st.st_mtime;
    
    if( BANNED_IP_ADDRESSES.size() == 0  || lLastMod != BANNED_IP_ADDRESSES_LAST_MOD )
    {
        std::ifstream lBannedIPAddressesFile("banned_ip_addresses.dat");
        std::string lLine;
        
        while(std::getline(lBannedIPAddressesFile, lLine))
        {
            BANNED_IP_ADDRESSES.push_back(lLine);  
        }
        
        BANNED_IP_ADDRESSES_LAST_MOD = lLastMod;
    }
    
    BANNED_IP_ADDRESSES_MUTEX.unlock();
}

void SaveBannedIPAddress(std::string ip_address)
{
    std::ofstream lBannedIPAddressesFile("banned_ip_addresses.dat", std::ios::out | std::ios::app);
    lBannedIPAddressesFile << ip_address + "\n";
    lBannedIPAddressesFile.close();
}

bool IsBannedIPAddress( std::string ip_address)
{
    LoadBannedIPAddresses();
    return std::find(BANNED_IP_ADDRESSES.begin(), BANNED_IP_ADDRESSES.end(), ip_address) != BANNED_IP_ADDRESSES.end() ;    
}

bool IsBannedAccount( std::string account )
{
    LoadBannedAccounts();
    return std::find(BANNED_ACCOUNTS.begin(), BANNED_ACCOUNTS.end(), account) != BANNED_ACCOUNTS.end() ; 
}


static std::map<std::string, int> CONNECTIONS_BY_ADDRESS;
static boost::mutex CONNECTIONS_BY_ADDRESS_MUTEX;
void IncConnectionCount( std::string ADDRESS )
{
    CONNECTIONS_BY_ADDRESS_MUTEX.lock();
    CONNECTIONS_BY_ADDRESS[ADDRESS]++;
    CONNECTIONS_BY_ADDRESS_MUTEX.unlock();
}

void DecConnectionCount( std::string ADDRESS )
{
    CONNECTIONS_BY_ADDRESS_MUTEX.lock();
    CONNECTIONS_BY_ADDRESS[ADDRESS]--;
    CONNECTIONS_BY_ADDRESS_MUTEX.unlock();
}

int GetConnectionCount( std::string ADDRESS )
{
    int nCount = 0;
    CONNECTIONS_BY_ADDRESS_MUTEX.lock();
    nCount = CONNECTIONS_BY_ADDRESS[ADDRESS];
    CONNECTIONS_BY_ADDRESS_MUTEX.unlock();

    return nCount;
}
