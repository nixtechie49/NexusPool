#ifndef NEXUS_LLP_TIMER_HPP
#define NEXUS_LLP_TIMER_HPP

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
		Timer() : fStopped(false) {}


		/** Start
		 *
		 *  Capture the start time with a high resolution clock. Sets stopped to false.
		 *
		 **/
		void Start() 
		{
			start_time = std::chrono::high_resolution_clock::now();
			fStopped = false;
		}


		/** Reset
		 *
		 *  Used as another alias for calling Start()
		 *
		 **/
		void Reset()
		{
			Start();
		}


		/** Stop
		 *
		 *  Capture the end time with a high resolution clock. Sets stopped to true.
		 *
		 **/
		void Stop()
		{
			end_time = std::chrono::high_resolution_clock::now();
			fStopped = true;
		}



		/** Elapsed
		 *
		 *  Return the Total Seconds Elapsed Since Timer Started.
		 *
		 *  @return The Total Seconds Elapsed Since Timer Started.
		 *
		 **/
		uint64_t Elapsed()
		{
			if (fStopped)
				return std::chrono::duration_cast<std::chrono::seconds>(end_time - start_time).count();

			return std::chrono::duration_cast<std::chrono::seconds>(std::chrono::high_resolution_clock::now() - start_time).count();
		}


		/** ElapsedMilliseconds
		 *
		 *  Return the Total Milliseconds Elapsed Since Timer Started.
		 *
		 *  @return The Total Milliseconds Elapsed Since Timer Started.
		 *
		 **/
		uint64_t ElapsedMilliseconds()
		{
			if (fStopped)
				return std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time).count();

			return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now() - start_time).count();
		}


		/** ElapsedMicroseconds
		 *
		 *  Return the Total Microseconds Elapsed since Time Started.
		 *
		 *  @return The Total Microseconds Elapsed since Time Started.
		 *
		 **/
		uint64_t ElapsedMicroseconds()
		{
			if (fStopped)
				return std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time).count();

			return std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::high_resolution_clock::now() - start_time).count();
		}
	};
}

#endif
