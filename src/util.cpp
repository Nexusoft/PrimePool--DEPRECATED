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
        ret = vfprintf(fileout, pszFormat, arg_ptr);
        va_end(arg_ptr);
    }
	
	MUTEX.unlock();
	
	return ret;
}