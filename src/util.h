#ifndef NEXUS_UTIL_H
#define NEXUS_UTIL_H

#include <string>
#include <vector>
#include <thread>
#include <stdio.h>
#include <cstdlib>
#include <stdarg.h>
#include <mutex>
#include <fstream>
#include <sstream>

#ifdef WIN32
#include <mpir.h>
#else
#include <gmp.h>
#endif
#include <fstream>
#include <sstream>


#define LOCK(a) std::lock_guard<std::mutex> lock(a)

#define loop                for(;;)
#define printf              ConsoleOutput


typedef long long  int64;
typedef unsigned long long  uint64;

int ConsoleOutput(const char* pszFormat, ...);

#ifndef WIN32
//inline void Sleep(int64 n)
//{
    /*Boost has a year 2038 problem— if the request sleep time is past epoch+2^31 seconds the sleep returns instantly.
      So we clamp our sleeps here to 10 years and hope that boost is fixed by 2028.*/
//    boost::thread::sleep(boost::get_system_time() + boost::posix_time::milliseconds(n>315576000000LL?315576000000LL:n));
//}

inline void Sleep(unsigned int nTime){ boost::this_thread::sleep(boost::posix_time::milliseconds(nTime)); }
#endif

/** Convert a 32 bit Unsigned Integer to Byte Vector using Bitwise Shifts. **/
inline std::vector<unsigned char> uint2bytes(unsigned int UINT)
{
	std::vector<unsigned char> BYTES(4, 0);
	BYTES[0] = UINT >> 24;
	BYTES[1] = UINT >> 16;
	BYTES[2] = UINT >> 8;
	BYTES[3] = UINT;
				
	return BYTES;
}
			
			
/** Convert a byte stream into unsigned integer 32 bit. **/	
inline unsigned int bytes2uint(std::vector<unsigned char> BYTES, int nOffset = 0) 
{
	if(BYTES.size() < nOffset + 4)
		return 0;
	
	return (BYTES[0 + nOffset] << 24) + (BYTES[1 + nOffset] << 16) + (BYTES[2 + nOffset] << 8) + BYTES[3 + nOffset]; 
}		
			
/** Convert a 64 bit Unsigned Integer to Byte Vector using Bitwise Shifts. **/
inline std::vector<unsigned char> uint2bytes64(uint64 UINT)
{
	std::vector<unsigned char> INTS[2];
	INTS[0] = uint2bytes((unsigned int) UINT);
	INTS[1] = uint2bytes((unsigned int) (UINT >> 32));
				
	std::vector<unsigned char> BYTES;
	BYTES.insert(BYTES.end(), INTS[0].begin(), INTS[0].end());
	BYTES.insert(BYTES.end(), INTS[1].begin(), INTS[1].end());
				
	return BYTES;
}

			
/** Convert a byte Vector into unsigned integer 64 bit. **/
inline uint64 bytes2uint64(std::vector<unsigned char> BYTES, int nOffset = 0) { return (bytes2uint(BYTES, nOffset) | ((uint64)bytes2uint(BYTES, nOffset + 4) << 32)); }



/** Convert Standard String into Byte Vector. **/
inline std::vector<unsigned char> string2bytes(std::string STRING)
{
	std::vector<unsigned char> BYTES(STRING.begin(), STRING.end());
	return BYTES;
}


/** Convert Byte Vector into Standard String. **/
inline std::string bytes2string(std::vector<unsigned char> BYTES, int nOffset = 0)
{
	std::string STRING(BYTES.begin() + nOffset, BYTES.end());
	return STRING;
}

/** Convert double into Byte Vector **/
inline std::vector<unsigned char> double2bytes(double DOUBLE)
{
    union {
        double DOUBLE;
        uint64 UINT64;
    } u;
    u.DOUBLE = DOUBLE;

    return uint2bytes64(u.UINT64);
}

/** Convert Byte Vector into double **/
inline double bytes2double(std::vector<unsigned char> BYTES)
{
    uint64 n64 = bytes2uint64(BYTES);
    union {
        double DOUBLE;
        uint64 UINT64;
    } u;
    u.UINT64 = n64;
    return u.DOUBLE;
}

inline const std::string time2datetimestring(time_t nTime) 
{
    struct tm  tstruct;
    char       buf[80];
    tstruct = *gmtime(&nTime);
    strftime(buf, sizeof(buf), "%Y-%m-%d.%X", &tstruct);

    return buf;
}

// Get current date/time, format is YYYY-MM-DD.HH:mm:ss
inline const std::string currentDateTime() 
{
    return time2datetimestring(time(0));
}



template < typename T > std::string to_string( const T& n )
    {
        std::ostringstream stm ;
        stm << n ;
        return stm.str() ;
    }


inline std::string stdprintf(const char *fmt, ...) {
	va_list args;
	va_start(args, fmt);
	char buf[32];
	size_t n = std::vsnprintf(buf, sizeof(buf), fmt, args);
	va_end(args);

	// Static buffer large enough?
	if (n < sizeof(buf)) {
		return {buf, n};
	}

	// Static buffer too small
	std::string s(n + 1, 0);
	va_start(args, fmt);
	std::vsnprintf(const_cast<char*>(s.data()), s.size(), fmt, args);
	va_end(args);

	return s;
}

void LoadBannedAccounts();

void LoadBannedIPAddresses();

void SaveBannedIPAddress(std::string ip_address);

bool IsBannedIPAddress( std::string ip_address);

bool IsBannedAccount( std::string account );

#endif
