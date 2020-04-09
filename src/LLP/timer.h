#ifndef NEXUS_LLP_TIMER_H
#define NEXUS_LLP_TIMER_H

#include <chrono>
#include <stdint.h>

namespace LLP
{

      /** Timer
       *
       *  Class the tracks the duration of time elapsed in seconds or milliseconds.
       *  Used for socket timers to determine time outs.
       *
       **/
      class Timer
      {
      private:
          std::chrono::high_resolution_clock::time_point start_time;
          std::chrono::high_resolution_clock::time_point end_time;
          bool fStopped;

      public:

          /** Timer
           *
           *  Contructs timer class with stopped set to false.
           *
           **/
          Timer();


          /** Start
           *
           *  Capture the start time with a high resolution clock. Sets stopped to false.
           *
           **/
          void Start();


          /** Reset
           *
           *  Used as another alias for calling Start()
           *
           **/
          void Reset();


          /** Stop
           *
           *  Capture the end time with a high resolution clock. Sets stopped to true.
           *
           **/
          void Stop();


          /** Elapsed
           *
           *  Return the Total Seconds Elapsed Since Timer Started.
           *
           *  @return The Total Seconds Elapsed Since Timer Started.
           *
           **/
          uint32_t Elapsed();


          /** ElapsedMilliseconds
           *
           *  Return the Total Milliseconds Elapsed Since Timer Started.
           *
           *  @return The Total Milliseconds Elapsed Since Timer Started.
           *
           **/
          uint32_t ElapsedMilliseconds();


          /** ElapsedMicroseconds
           *
           *  Return the Total Microseconds Elapsed since Time Started.
           *
           *  @return The Total Microseconds Elapsed since Time Started.
           *
           **/
          uint64_t ElapsedMicroseconds();
      };
}

#endif
