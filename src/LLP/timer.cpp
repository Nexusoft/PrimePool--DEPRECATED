#include "timer.h"
namespace LLP
{
    /*  Contructs timer class with stopped set to false. */
    Timer::Timer() : fStopped(false) {}

    /*  Capture the start time with a high resolution clock. Sets stopped to false. */
    void Timer::Start()
    {
        start_time = std::chrono::high_resolution_clock::now();
        fStopped = false;
    }

    /*  Used as another alias for calling Start() */
    void Timer::Reset()
    {
        Start();
    }

    /* Capture the end time with a high resolution clock. Sets stopped to true. */
    void Timer::Stop()
    {
        end_time = std::chrono::high_resolution_clock::now();
        fStopped = true;
    }

    /*  Return the Total Seconds Elapsed Since Timer Started. */
    uint32_t Timer::Elapsed()
    {
        if(fStopped)
            return std::chrono::duration_cast<std::chrono::seconds>(end_time - start_time).count();

        return std::chrono::duration_cast<std::chrono::seconds>(std::chrono::high_resolution_clock::now() - start_time).count();
    }

    /*  Return the Total Milliseconds Elapsed Since Timer Started. */
    uint32_t Timer::ElapsedMilliseconds()
    {
        if(fStopped)
            return std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time).count();

        return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now() - start_time).count();
    }

    /*  Return the Total Microseconds Elapsed since Time Started. */
    uint64_t Timer::ElapsedMicroseconds()
    {
        if(fStopped)
            return std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time).count();

        return std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::high_resolution_clock::now() - start_time).count();
    }
}
