#ifndef COINSHIELD_LLP_SERVER_H
#define COINSHIELD_LLP_SERVER_H

#include "types.h"

namespace LLP
{

	Mutex_t     DDOS_MUTEX;


	/** Base Template Thread Class for Server base. Used for Core LLP Packet Functionality. 
		Not to be inherited, only for use by the LLP Server Base Class. **/
	template <class ProtocolType> class DataThread
	{
		/** Data Thread. **/
		Thread_t DATA_THREAD;
		
	public:
	
		/** Service that is used to handle Connections on this Thread. **/
		Service_t IO_SERVICE;
		
		/** Variables to track Connection / Request Count. **/
		bool fDDOS; unsigned int nConnections, ID, REQUESTS, TIMEOUT, DDOS_rSCORE, DDOS_cSCORE;
		
		/** Vector to store Connections. **/
		std::vector< ProtocolType* > CONNECTIONS;
		
		/** Returns the index of a component of the CONNECTIONS vector that has been flagged Disconnected **/
		int FindSlot()
		{
			int nSize = CONNECTIONS.size();
			for(int index = 0; index < nSize; index++)
				if(!CONNECTIONS[index])
					return index;
						
			return nSize;
		}

		/** Adds a new connection to current Data Thread **/
		void AddConnection(Socket_t SOCKET, DDOS_Filter* DDOS)
		{
			int nSlot = FindSlot();
			if(nSlot == CONNECTIONS.size())
				CONNECTIONS.push_back(NULL);
				
			if(fDDOS)
				DDOS -> cSCORE += 1;
			
			CONNECTIONS[nSlot] = new ProtocolType(SOCKET, DDOS, fDDOS);
			
			CONNECTIONS[nSlot]->Event(EVENT_CONNECT);
			CONNECTIONS[nSlot]->CONNECTED = true;
			
			++nConnections;
		}
		
		/** Removes given connection from current Data Thread. 
			Happens with a timeout / error, graceful close, or disconnect command. **/
		void RemoveConnection(int index)
		{
			CONNECTIONS[index]->Event(EVENT_DISCONNECT);
			CONNECTIONS[index]->Disconnect();
			
			delete CONNECTIONS[index];
					
			CONNECTIONS[index] = NULL;
			-- nConnections;
		}
		
		/** Thread that handles all the Reading / Writing of Data from Sockets. 
			Creates a Packet QUEUE on this connection to be processed by an LLP Messaging Thread. **/
		void Thread()
		{
			for(;;)
			{
				/** Keep data threads at 1000 FPS Maximum. **/
				Sleep(1);
				
				/** Check all connections for data and packets. **/
				int nSize = CONNECTIONS.size();
				for(int nIndex = 0; nIndex < nSize; nIndex++)
				{
					try
					{
						
						/** Skip over Inactive Connections. **/
						if(!CONNECTIONS[nIndex])
							continue;
		
							
						/** Remove Connection if it has Timed out or had any Errors. **/
						if(CONNECTIONS[nIndex]->Timeout(TIMEOUT) || CONNECTIONS[nIndex]->Errors())
						{
							RemoveConnection(nIndex);
							
							continue;
						}
						
						
						/** Skip over Connection if not Connected. **/
						if(!CONNECTIONS[nIndex]->Connected())
							continue;
						
						
						/** Handle any DDOS Filters. **/
						if(fDDOS)
						{
							/** Ban a node if it has too many Requests per Second. **/
							if(CONNECTIONS[nIndex]->DDOS->rSCORE.Score() > DDOS_rSCORE || CONNECTIONS[nIndex]->DDOS->cSCORE.Score() > DDOS_cSCORE)
							   CONNECTIONS[nIndex]->DDOS->Ban();
							
							/** Remove a connection if it was banned by DDOS Protection. **/
							if(CONNECTIONS[nIndex]->DDOS->Banned())
							{
								RemoveConnection(nIndex);
								
								continue;
							}
						}
						
						
						/** Generic event for Connection. **/
						CONNECTIONS[nIndex]->Event(EVENT_GENERIC);
						
						/** Work on Reading a Packet. **/
						CONNECTIONS[nIndex]->ReadPacket();
						
						/** If a Packet was received successfully, increment request count [and DDOS count if enabled]. **/
						if(CONNECTIONS[nIndex]->PacketComplete())
						{
							
							/** Packet Process return value of False will flag Data Thread to Disconnect. **/
							if(!CONNECTIONS[nIndex] -> ProcessPacket())
							{
								RemoveConnection(nIndex);
								
								continue;
							}
							
							CONNECTIONS[nIndex] -> ResetPacket();
							REQUESTS++;
							
							if(fDDOS)
								CONNECTIONS[nIndex]->DDOS->rSCORE += 1;
							
						}
					}
					catch(std::exception& e)
					{
						printf("error: %s\n", e.what());
					}
				}
			}
		}
		
		DataThread<ProtocolType>(unsigned int id, bool isDDOS, unsigned int rScore, unsigned int cScore, unsigned int nTimeout) : 
			ID(id), fDDOS(isDDOS), DDOS_rSCORE(rScore), DDOS_cSCORE(cScore), TIMEOUT(nTimeout), REQUESTS(0), CONNECTIONS(0), nConnections(0), DATA_THREAD(boost::bind(&DataThread::Thread, this)){ }
	};

	
	
	/** Base Class to create a Custom LLP Server. Protocol Type class must inherit Connection,
		and provide a ProcessPacket method. Optional Events by providing GenericEvent method. **/
	template <class ProtocolType> class Server
	{
		/** The DDOS variables. Tracks the Requests and Connections per Second
			from each connected address. **/
		std::map<unsigned int,   DDOS_Filter*> DDOS_MAP;
		bool fDDOS;
		
	public:
		int PORT, MAX_THREADS;
		
		/** The data type to keep track of current running threads. **/
		std::vector< DataThread<ProtocolType>* > DATA_THREADS;
		
		
		Server<ProtocolType>(int nPort, int nMaxThreads, bool isDDOS, int cScore, int rScore, int nTimeout) : 
			fDDOS(isDDOS), LISTENER(SERVICE), PORT(nPort), MAX_THREADS(nMaxThreads), METER_THREAD(boost::bind(&Server::MeterThread, this)), LISTEN_THREAD(boost::bind(&Server::ListeningThread, this))
		{
			for(int index = 0; index < MAX_THREADS; index++)
				DATA_THREADS.push_back(new DataThread<ProtocolType>(index, fDDOS, rScore, cScore, nTimeout));
		}
		
	private:
	
		/** Basic Socket Handle Variables. **/
		Service_t   SERVICE;
		Listener_t  LISTENER;
		Error_t     ERROR_HANDLE;
		Thread_t    LISTEN_THREAD;
		Thread_t    METER_THREAD;
		
	
		/** Determine the thread with the least amount of active connections. 
			This keeps the load balanced across all server threads. **/
		int FindThread()
		{
			int nIndex = 0, nConnections = DATA_THREADS[0]->nConnections;
			for(int index = 1; index < MAX_THREADS; index++)
			{
				if(DATA_THREADS[index]->nConnections < nConnections)
				{
					nIndex = index;
					nConnections = DATA_THREADS[index]->nConnections;
				}
			}
			
			return nIndex;
		}
		
		/** LLP Meter Thread. Tracks the Requests / Second. **/
		void MeterThread()
		{
			Timer TIMER;
			TIMER.Start();
			
			for(;;)
			{	
				Sleep(10000);
				
				Core::nConnections = 0;
				for(int nIndex = 0; nIndex < MAX_THREADS; nIndex++)
					Core::nConnections += DATA_THREADS[nIndex]->nConnections;
					
				double RPS = (double) TotalRequests() / TIMER.Elapsed();
				printf("[METERS] LLP Running at %f Requests per Second with %u Connections.\n", RPS, Core::nConnections);

				TIMER.Reset();
				ClearRequests();
			}
		}
		
		/** Main Listening Thread of LLP Server. Handles new Connections and DDOS associated with Connection if enabled. **/
		void ListeningThread()
		{
			/** Don't listen until all data threads are created. **/
			while(DATA_THREADS.size() < MAX_THREADS)
				Sleep(1000);
				
			/** Basic Socket Options for Boost ASIO. Allow aborted connections, don't allow lingering. **/
			boost::asio::socket_base::enable_connection_aborted    CONNECTION_ABORT(true);
			boost::asio::socket_base::linger                       CONNECTION_LINGER(false, 0);
			boost::asio::ip::tcp::acceptor::reuse_address          CONNECTION_REUSE(true);
			boost::asio::ip::tcp::endpoint 						   ENDPOINT(boost::asio::ip::tcp::v4(), PORT);
			
			/** Open the listener with maximum of 1000 queued Connections. **/
			LISTENER.open(ENDPOINT.protocol());
			LISTENER.set_option(CONNECTION_ABORT);
			LISTENER.set_option(CONNECTION_REUSE);
			LISTENER.set_option(CONNECTION_LINGER);
			LISTENER.bind(ENDPOINT);
			LISTENER.listen(1000, ERROR_HANDLE);
			
			//printf("LLP Server Listening on Port %u\n", PORT);
			for(;;)
			{
				/** Limit listener to allow maximum of 200 new connections per second. **/
				Sleep(5);
				
				try
				{
					/** Accept a new connection, then process DDOS. **/
					int nThread = FindThread();
					Socket_t SOCKET(new boost::asio::ip::tcp::socket(DATA_THREADS[nThread]->IO_SERVICE));
					LISTENER.accept(*SOCKET);
					
					/** Initialize DDOS Protection for Incoming IP Address. **/
					std::vector<unsigned char> BYTES(4, 0);
                    std::string IPADDRESS = SOCKET->remote_endpoint().address().to_string();
					sscanf(IPADDRESS.c_str(), "%u.%u.%u.%u", &BYTES[0], &BYTES[1], &BYTES[2], &BYTES[3]);
					
					unsigned int ADDRESS = (BYTES[0] << 24) + (BYTES[1] << 16) + (BYTES[2] << 8) + BYTES[3];
					
					{ //LOCK(DDOS_MUTEX);
						if(!DDOS_MAP.count(ADDRESS))
							DDOS_MAP[ADDRESS] = new DDOS_Filter(30, IPADDRESS);
                            
                        // ban if on the banned IP list
                        if( fDDOS && IsBannedIPAddress( IPADDRESS ))
							DDOS_MAP[ADDRESS]->Ban("Banned IP Address");                            
                            
						/** DDOS Operations: Only executed when DDOS is enabled. **/
						if(fDDOS && DDOS_MAP[ADDRESS]->Banned())
						{
							SOCKET -> shutdown(boost::asio::ip::tcp::socket::shutdown_both, ERROR_HANDLE);
							SOCKET -> close();
								
							continue;
						}
					
					
						/** Add new connection if passed all DDOS checks. **/
						DATA_THREADS[nThread]->AddConnection(SOCKET, DDOS_MAP[ADDRESS]);
					}
				}
				catch(std::exception& e)
				{
					printf("error: %s\n", e.what());
				}
			}
		}
		
		/** Used for Meter. Adds up the total amount of requests from each Data Thread. **/
		int TotalRequests()
		{
			int nTotalRequests = 0;
			for(int nThread = 0; nThread < MAX_THREADS; nThread++)
				nTotalRequests += DATA_THREADS[nThread]->REQUESTS;
					
			return nTotalRequests;
		}
			
		/** Used for Meter. Resets the REQUESTS variable to 0 in each Data Thread. **/
		void ClearRequests()
		{
			for(int nThread = 0; nThread < MAX_THREADS; nThread++)
				DATA_THREADS[nThread]->REQUESTS = 0;
		}
	};

}

#endif