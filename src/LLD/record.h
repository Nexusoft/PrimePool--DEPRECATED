#ifndef COINSHIELD_LLD_ACCOUNT
#define COINSHIELD_LLD_ACCOUNT

#include "database.h"
#include "../coinbase.h"
#include "../hash/uint1024.h"

namespace LLD
{
	/** Container Class for Account Record. Handles the Serialization and De-Serialization. **/
	class Account : public Record<std::string>
	{
	public:
		/** Data Objects the Class Contains. The Objects that are Serialized and De-Serialized. **/
		uint64 nAccountBalance;
		uint64 nRoundShares;
		
		unsigned int nBlocksFound;
		
		/** Optional Constructor for Creating a new Record. **/
		Account(std::string strKey) { cKey = strKey; nAccountBalance = 0; nRoundShares = 0; nBlocksFound = 0; }
		
		/** Required Constructors. One must allow to construct from a Byte Stream, and Have a Blank Constructor **/
		Account() { cKey = ""; nAccountBalance = 0; nRoundShares = 0; nBlocksFound = 0; }
		Account(std::vector<unsigned char> vInputData) { Deserialize(vInputData); }
		
		/** Deserialization Function. Must Deserialize the cKey inherited from Record. **/
		void Deserialize(std::vector<unsigned char> vInputData)
		{
			nAccountBalance = bytes2uint64(vInputData);
			nRoundShares    = bytes2uint64(vInputData, 8);
			nBlocksFound    =   bytes2uint(vInputData, 16);
			cKey            = bytes2string(vInputData, 20);	
		}
		
		/** Serialization Function. Must Include the cKey in Serialization Streams. **/
		std::vector<unsigned char> Serialize()
		{
			std::vector<unsigned char> vBalance = uint2bytes64(nAccountBalance);
			std::vector<unsigned char> vShares  = uint2bytes64(nRoundShares);
			std::vector<unsigned char> vBlocks  = uint2bytes(nBlocksFound);
			std::vector<unsigned char> vKey     = string2bytes(cKey);
			
			std::vector<unsigned char> vData;
			vData.insert(vData.end(), vBalance.begin(), vBalance.end());
			vData.insert(vData.end(), vShares.begin(),   vShares.end());
			vData.insert(vData.end(), vBlocks.begin(),   vBlocks.end());
			vData.insert(vData.end(), vKey.begin(),         vKey.end());
			
			return vData;
		}
		
		void Print()
		{
			printf("Balance %f | Shares %llu | Blocks %u | Address = %s\n", nAccountBalance / 1000000.0, nRoundShares, nBlocksFound, cKey.c_str()); 
		}
	};
	
	
	/** Container Class for Block Record. Handles the Serialization and De-Serialization. 
		Contains the Last Block Coinbase Tx including Payout Data, and Block Hash for the Key. **/
	class Block : public Record<uint1024>
	{
	public:
		/** Data Objects the Class Contains. The Objects that are Serialized and De-Serialized. **/
		uint64 nCoinbaseValue;
		Coinbase cCoinbase;
		Credits  cCredits;
		
		unsigned int nRound;
		
		/** Optional Constructor for Creating a new Record. **/
		Block(uint1024 nKey) { cKey = nKey; nCoinbaseValue = 0; nRound = 0; }
		
		
		/** Required Constructors. One must allow to construct from a Byte Stream, and Have a Blank Constructor **/
		Block() { nCoinbaseValue = 0; cKey = 0; nRound = 0; }
		Block(std::vector<unsigned char> vInputData){ Deserialize(vInputData); }
		
		
		/** De-Serialize this class from a bytes stream. **/
		void Deserialize(std::vector<unsigned char> vInputData)
		{
			/** Deserialize the Coinbase Value. **/
			nCoinbaseValue = bytes2uint64(vInputData);
			
			/** Deserialize the Coinbase Transaction. **/
			unsigned int nBytes = bytes2uint(vInputData, 8);
			std::vector<unsigned char> vCoinbase;
			vCoinbase.insert(vCoinbase.end(), vInputData.begin() + 12, vInputData.begin() + nBytes + 12);
			cCoinbase.Deserialize(vCoinbase, nCoinbaseValue);
			
			/** Deserialize the Round Credits. **/
			std::vector<unsigned char> vCredits;
			vCredits.insert(vCredits.end(), vInputData.begin() + nBytes + 12, vInputData.end() - 132);
			cCredits.Deserialize(vCredits);
			
			/** Deserialize the Round Number. **/
			std::vector<unsigned char> vRound;
			vRound.insert(vRound.end(), vInputData.end() - 132, vInputData.end() - 128);
			nRound = bytes2uint(vRound);
			
			/** Deserialize the Database Key. **/
			std::vector<unsigned char> vKey;
			vKey.insert(vKey.end(), vInputData.end() - 128, vInputData.end());
			cKey.SetBytes(vKey);
		}
		
		/** Database Classes must provide Serialization Function **/
		std::vector<unsigned char> Serialize()
		{
			std::vector<unsigned char> vValue     = uint2bytes64(nCoinbaseValue);
			std::vector<unsigned char> vCoinbase  = cCoinbase.Serialize();
			std::vector<unsigned char> vLength    = uint2bytes(vCoinbase.size());
			std::vector<unsigned char> vCredits   = cCredits.Serialize();
			std::vector<unsigned char> vRound     = uint2bytes(nRound);
			std::vector<unsigned char> vKey       = cKey.GetBytes();
			
			std::vector<unsigned char> vData;
			vData.insert(vData.end(), vValue.begin(),       vValue.end());
			vData.insert(vData.end(), vLength.begin(),     vLength.end());
			vData.insert(vData.end(), vCoinbase.begin(), vCoinbase.end());
			vData.insert(vData.end(), vCredits.begin(),   vCredits.end());
			vData.insert(vData.end(), vRound.begin(),       vRound.end());
			vData.insert(vData.end(), vKey.begin(),           vKey.end());
			
			return vData;
		}
		
		void Print() { printf("Hash = %s\n", cKey.ToString().c_str()); cCoinbase.Print(); cCredits.Print(); }
	};
}

#endif