#ifndef NEXUS_LLD_DATABASE
#define NEXUS_LLD_DATABASE

#include <fstream>
#include <string>
#include <mutex>
#include <map>
#include "../util.h"

namespace LLD
{

	/** Main Class to be inherited Creating Customized Database Handles. Contains only the Key. **/
	template<typename KeyType> 
	class Record 
	{
		public: KeyType cKey; 
	};	

	/** Base Template Class for the Database Handle. Processes main Lower Level Disk Communications. **/
	template<typename KeyType, class RecordType> class Database
	{
		/** Mutex for Thread Synchronization. **/
		std::mutex DATABASE_MUTEX;
		
		std::string strFilename;
		typename std::map<KeyType, RecordType> mapRecords;
		
	public:
		Database(std::string fileout) { strFilename = fileout; ReadIntoMemory(); }
		
		/** Return the Keys to the Records Held in the Database. **/
		std::vector<KeyType> GetKeys()
		{
			LOCK(DATABASE_MUTEX);
			
			std::vector<KeyType> vKeys;
			for(typename std::map<KeyType, RecordType>::iterator nIterator = mapRecords.begin(); nIterator != mapRecords.end(); nIterator++)
				vKeys.push_back(nIterator->first);
				
			return vKeys;
		}
		
		
		/** Return Whether a Key Exists in the Database. **/
		bool HasKey(KeyType cKey)
		{
			LOCK(DATABASE_MUTEX);
			
			return mapRecords.count(cKey);
		}
		
		
		/** Erase a Record from the Database. **/
		void EraseRecord(KeyType cKey)
		{
			LOCK(DATABASE_MUTEX);
			
			mapRecords.erase(cKey);
		}
		
		
		/** Write the Database Memory Map to the Disk. **/
		void WriteToDisk()
		{
			LOCK(DATABASE_MUTEX);
			
			std::ofstream fOutgoing(strFilename.c_str(), std::ofstream::out | std::ofstream::binary | std::ofstream::trunc);
			for(typename std::map<KeyType, RecordType>::iterator nIterator = mapRecords.begin(); nIterator != mapRecords.end(); nIterator++)
			{
				std::vector<unsigned char> vData = nIterator->second.Serialize();
				std::vector<unsigned char> vSize = uint2bytes(vData.size());
				fOutgoing.write((char*) &vSize[0], 4);
				fOutgoing.write((char*) &vData[0], vData.size());
			}
			fOutgoing.close();
		}
		
		
		/** Read into the Memory Map the Database Binary Data. **/
		void ReadIntoMemory()
		{
			LOCK(DATABASE_MUTEX);
			
			/** Clear the Database Memory Map if Updating from Disk. **/
			if(!mapRecords.empty())
				mapRecords.clear();

			/** Open the Stream to Read from the File Stream. **/
			std::ifstream fIncoming(strFilename.c_str(), std::ifstream::in | std::ifstream::binary);
			if(!fIncoming)
			{
				printf("Failed To Read Database File.\n");
				return;
			}
			
			/** Get the Size of the Database File. **/
			fIncoming.seekg (0, fIncoming.end);
			unsigned int nLength = fIncoming.tellg();
			fIncoming.seekg (0, fIncoming.beg);
			
			
			/** Read every Record until the End of the File. **/
			while(fIncoming.tellg() < nLength)
			{
				/** Read the Record Size. **/
				std::vector<unsigned char> vSize(4, 0);
				fIncoming.read((char*) &vSize[0], 4);
				unsigned int nSize = bytes2uint(vSize);
				
				/** Read the Binary Data. **/
				std::vector<unsigned char> vData(nSize, 0);
				fIncoming.read((char*)&vData[0], nSize);

				/** Construct the Record from the Binary Data. **/
				RecordType cRecord(vData);
				mapRecords[cRecord.cKey] = cRecord;

			}
			
			fIncoming.close();
		}
		
		
		/** Get a Record from the Database with Given Key. **/
		RecordType GetRecord(KeyType cKey)
		{
			LOCK(DATABASE_MUTEX);
			
			return mapRecords[cKey];
		}
		
		
		/** Add / Update A Record in the Database **/
		void UpdateRecord(RecordType cRecord)
		{
			LOCK(DATABASE_MUTEX);

			mapRecords[cRecord.cKey] = cRecord;
		}
		
	};
}

#endif