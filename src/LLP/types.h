#ifndef COINSHIELD_LLP_TYPES_H
#define COINSHIELD_LLP_TYPES_H

#include <boost/date_time/posix_time/posix_time.hpp>

#include <string>
#include <vector>
#include <stdio.h>
#include <boost/bind.hpp>
#include <boost/smart_ptr.hpp>
#include <boost/asio.hpp>
#include <boost/thread/thread.hpp>         

#include "../util.h"
	
namespace LLP
{
	
	/** Event Enumeration. Used to determine each Event from LLP Server. **/
	enum 
	{
		EVENT_HEADER         = 0,
		EVENT_PACKET         = 1,
		EVENT_CONNECT        = 2,
		EVENT_DISCONNECT     = 3,
		EVENT_GENERIC        = 4
	};


	/** Type Definitions for LLP Functions **/
	typedef boost::shared_ptr<boost::asio::ip::tcp::socket>      Socket_t;
	typedef boost::asio::ip::tcp::acceptor                       Listener_t;
	typedef boost::asio::io_service                              Service_t;
	typedef boost::thread                                        Thread_t;
	typedef boost::system::error_code                            Error_t;
	typedef boost::mutex                                         Mutex_t;
	
	
	/** Sleep for a duration in Milliseconds. **/
	inline void Sleep(unsigned int nTime){ boost::this_thread::sleep(boost::posix_time::milliseconds(nTime)); }
	
	
	
	/** Class the tracks the duration of time elapsed in seconds or milliseconds.
		Used for socket timers to determine time outs. **/
	class Timer
	{
	private:
		boost::posix_time::ptime TIMER_START, TIMER_END;
		bool fStopped = false;
	
	public:
		inline void Start() { TIMER_START = boost::posix_time::microsec_clock::local_time(); fStopped = false; }
		inline void Reset() { Start(); }
		inline void Stop() { TIMER_END = boost::posix_time::microsec_clock::local_time(); fStopped = true; }
		
		/** Return the Total Seconds Elapsed Since Timer Started. **/
		unsigned int Elapsed()
		{
			if(fStopped)
				return (TIMER_END - TIMER_START).total_seconds();
				
			return (boost::posix_time::microsec_clock::local_time() - TIMER_START).total_seconds();
		}
		
		/** Return the Total Milliseconds Elapsed Since Timer Started. **/
		unsigned int ElapsedMilliseconds()
		{
			if(fStopped)
				return (TIMER_END - TIMER_START).total_milliseconds();
				
			return (boost::posix_time::microsec_clock::local_time() - TIMER_START).total_milliseconds();
		}
	};
	
	
	/** Class that tracks DDOS attempts on LLP Servers. 
		Uses a Timer to calculate Request Score [rScore] and Connection Score [cScore] as a unit of Score / Second. 
		Pointer stored by Connection class and Server Listener DDOS_MAP. **/
	class DDOS_Score
	{
		std::vector< std::pair<bool, int> > SCORE;
		Timer TIMER;
		int nIterator;
		Mutex_t MUTEX;
		
		
		/** Reset the Timer and the Score Flags to be Overwritten. **/
		void Reset()
		{
			for(int i = 0; i < SCORE.size(); i++)
				SCORE[i].first = false;
				
			TIMER.Reset();
			nIterator = 0;
		}
		
	public:
	
		/** Construct a DDOS Score of Moving Average Timespan. **/
		DDOS_Score(int nTimespan)
		{
			LOCK(MUTEX);
			
			for(int i = 0; i < nTimespan; i++)
				SCORE.push_back(std::make_pair(true, 0));
				
			TIMER.Start();
			nIterator = 0;
		}
		
		
		/** Flush the DDOS Score to 0. **/
		void Flush()
		{
			LOCK(MUTEX);
			
			Reset();
			for(int i = 0; i < SCORE.size(); i++)
				SCORE[i].second = 0;
		}
		
		
		/** Access the DDOS Score from the Moving Average. **/
		int Score()
		{
			LOCK(MUTEX);
			
			int nMovingAverage = 0;
			for(int i = 0; i < SCORE.size(); i++)
				nMovingAverage += SCORE[i].second;
			
			return nMovingAverage / SCORE.size();
		}
		
		
		/** Increase the Score by nScore. Operates on the Moving Average to Increment Score per Second. **/
		DDOS_Score & operator+=(const int& nScore)
		{
			LOCK(MUTEX);
			
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
	};
	
	
	/** Filter to Contain DDOS Scores and Handle DDOS Bans. **/
	class DDOS_Filter
	{
		Timer TIMER;
		unsigned int BANTIME, TOTALBANS;
		
	public:
		DDOS_Score rSCORE, cSCORE;
		DDOS_Filter(unsigned int nTimespan) : rSCORE(nTimespan), cSCORE(nTimespan), BANTIME(0), TOTALBANS(0) { }
		Mutex_t MUTEX;
		
		/** Ban a Connection, and Flush its Scores. **/
		void Ban()
		{
			LOCK(MUTEX);
			
			if(Banned())
				return;
			
			TIMER.Start();
			TOTALBANS++;
			
			BANTIME = std::max(TOTALBANS * (rSCORE.Score() + 1) * (cSCORE.Score() + 1), TOTALBANS * 1200u);
			
			printf("XXXXX DDOS Filter cScore = %i rScore = %i Banned for %u Seconds.\n", cSCORE.Score(), rSCORE.Score(), BANTIME);
			
			cSCORE.Flush();
			rSCORE.Flush();
		}
		
		/** Check if Connection is Still Banned. **/
		bool Banned() { return (TIMER.Elapsed() < BANTIME); }
	};
	
	

	/** Class to handle sending and receiving of LLP Packets. **/
	class Packet
	{
	public:
		Packet() { SetNull(); }
	
		/** Components of an LLP Packet.
			BYTE 0       : Header
			BYTE 1 - 5   : Length
			BYTE 6 - End : Data      **/
		unsigned char    HEADER;
		unsigned int     LENGTH;
		std::vector<unsigned char> DATA;
		
		
		/** Set the Packet Null Flags. **/
		inline void SetNull()
		{
			HEADER   = 255;
			LENGTH   = 0;
			
			DATA.clear();
		}
		
		
		/** Packet Null Flag. Header = 255. **/
		bool IsNull() { return (HEADER == 255); }
		
		
		/** Determine if a packet is fully read. **/
		bool Complete() { return (Header() && DATA.size() == LENGTH); }
		
		
		/** Determine if header is fully read **/
		bool Header() { return IsNull() ? false : (HEADER < 128 && LENGTH > 0) || (HEADER >= 128 && HEADER < 255 && LENGTH == 0); }
		
		
		/** Sets the size of the packet from Byte Vector. **/
		void SetLength(std::vector<unsigned char> BYTES) { LENGTH = (BYTES[0] << 24) + (BYTES[1] << 16) + (BYTES[2] << 8) + (BYTES[3] ); }
		
		
		/** Serializes class into a Byte Vector. Used to write Packet to Sockets. **/
		std::vector<unsigned char> GetBytes()
		{
			std::vector<unsigned char> BYTES(1, HEADER);
			
			/** Handle for Data Packets. **/
			if(HEADER < 128)
			{
				BYTES.push_back((LENGTH >> 24)); BYTES.push_back((LENGTH >> 16));
				BYTES.push_back((LENGTH >> 8));  BYTES.push_back(LENGTH);
				
				BYTES.insert(BYTES.end(),  DATA.begin(), DATA.end());
			}
			
			return BYTES;
		}
	};
	
	

	/** Base Template class to handle outgoing / incoming LLP data for both Client and Server. **/
	class Connection
	{
	protected:
		
		/** Basic Connection Variables. **/
		Timer         TIMER;
		Error_t       ERROR_HANDLE;
		Socket_t      SOCKET;
		
		
		/** 
			Virtual Event Function to be Overridden allowing Custom Read Events. 
			Each event fired on Header Complete, and each time data is read to fill packet.
			Useful to check Header length to maximum size of packet type for DDOS protection, 
			sending a keep-alive ping while downloading large files, etc.
			
			LENGTH == 0: General Events
			LENGTH  > 0 && PACKET: Read nSize Bytes into Data Packet
		**/
		virtual void Event(unsigned char EVENT, unsigned int LENGTH = 0){ }
		
		/** Virtual Process Function. To be overridden with your own custom packet processing. **/
		virtual bool ProcessPacket(){ }
	public:
	
	
		/** Incoming Packet Being Built. **/
		Packet        INCOMING;
		
		
		/** DDOS Score if a Incoming Server Connection. **/
		DDOS_Filter*   DDOS;
		
		
		/** Connected Flag. **/
		bool CONNECTED;
		
		
		/** Flag to Determine if DDOS is Enabled. **/
		bool fDDOS;
		
		
		/** Connection Constructors **/
		Connection() : SOCKET(), DDOS(NULL), INCOMING(), CONNECTED(false), fDDOS(false) { INCOMING.SetNull(); }
		Connection( Socket_t SOCKET_IN, DDOS_Filter* DDOS_IN, bool isDDOS ) : SOCKET(SOCKET_IN), fDDOS(isDDOS), DDOS(DDOS_IN), INCOMING(), CONNECTED(false) { TIMER.Start(); }
		
		
		/** Checks for any flags in the Error Handle. **/
		bool Errors(){ return (ERROR_HANDLE == boost::asio::error::eof || ERROR_HANDLE); }
				
				
		/** Determines if nTime seconds have elapsed since last Read / Write. **/
		bool Timeout(unsigned int nTime){ return (TIMER.Elapsed() >= nTime); }
		
		
		/** Determines if Connected or Not. **/
		bool Connected(){ return CONNECTED; }
		
		
		/** Handles two types of packets, requests which are of header >= 128, and data which are of header < 128. **/
		bool PacketComplete(){ return INCOMING.Complete(); }
		
		
		/** Used to reset the packet to Null after it has been processed. This then flags the Connection to read another packet. **/
		void ResetPacket(){ INCOMING.SetNull(); }
		
		
		/** Write a single packet to the TCP stream. **/
		void WritePacket(Packet PACKET) { Write(PACKET.GetBytes()); }
		
		
		/** Non-Blocking Packet reader to build a packet from TCP Connection.
			This keeps thread from spending too much time for each Connection. **/
		void ReadPacket()
		{
				
			/** Handle Reading Packet Type Header. **/
			if(SOCKET->available() > 0 && INCOMING.IsNull())
			{
				std::vector<unsigned char> HEADER(1, 255);
				if(Read(HEADER, 1) == 1)
					INCOMING.HEADER = HEADER[0];
					
			}
				
			if(!INCOMING.IsNull() && !INCOMING.Complete())
			{
				/** Handle Reading Packet Length Header. **/
				if(SOCKET->available() >= 4 && INCOMING.LENGTH == 0)
				{
					std::vector<unsigned char> BYTES(4, 0);
					if(Read(BYTES, 4) == 4)
					{
						INCOMING.SetLength(BYTES);
						Event(EVENT_HEADER);
					}
				}
					
				/** Handle Reading Packet Data. **/
				unsigned int nAvailable = SOCKET->available();
				if(nAvailable > 0 && INCOMING.LENGTH > 0 && INCOMING.DATA.size() < INCOMING.LENGTH)
				{
					std::vector<unsigned char> DATA( std::min(nAvailable, (unsigned int)(INCOMING.LENGTH - INCOMING.DATA.size())), 0);
					unsigned int nRead = Read(DATA, DATA.size());
					
					if(nRead == DATA.size())
					{
						INCOMING.DATA.insert(INCOMING.DATA.end(), DATA.begin(), DATA.end());
						Event(EVENT_PACKET, nRead);
					}
				}
			}
		}
		
		
		/** Disconnect Socket. Cleans up memory usage to prevent "memory runs" from poor memory management. **/
		void Disconnect()
		{
			if(!CONNECTED)
				return;
				
			try
			{
				SOCKET -> shutdown(boost::asio::ip::tcp::socket::shutdown_both, ERROR_HANDLE);
				SOCKET -> close();
			}
			catch(...){}
			
			CONNECTED = false;
		}
		
		
	private:
		
		/** Lower level network communications: Read. Interacts with OS sockets. **/
		size_t Read(std::vector<unsigned char> &DATA, size_t nBytes) { if(Errors()) return 0; TIMER.Reset(); return  boost::asio::read(*SOCKET, boost::asio::buffer(DATA, nBytes), ERROR_HANDLE); }
							
				
				
		/** Lower level network communications: Write. Interacts with OS sockets. **/
		void Write(std::vector<unsigned char> DATA) { if(Errors()) return; TIMER.Reset(); boost::asio::write(*SOCKET, boost::asio::buffer(DATA, DATA.size()), ERROR_HANDLE); }

	};

	

}

#endif
	
	