/*******************************************************************************************
 
			Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++
   
 [Learn, Create, but do not Forge] Viz. http://www.opensource.org/licenses/mit-license.php
  
*******************************************************************************************/

#ifndef COINSHIELD_BASE58_H
#define COINSHIELD_BASE58_H

#include <string>
#include <vector>
#include "bignum.h"

namespace Core
{
	static const char* pszBase58 = "123456789ABCDEFGHJKLMNPQRSTUVWXYZabcdefghijkmnopqrstuvwxyz";

	// Encode a byte sequence as a base58-encoded string
	inline std::string EncodeBase58(const unsigned char* pbegin, const unsigned char* pend)
	{
		CAutoBN_CTX pctx;
		CBigNum bn58 = 58;
		CBigNum bn0 = 0;

		// Convert big endian data to little endian
		// Extra zero at the end make sure bignum will interpret as a positive number
		std::vector<unsigned char> vchTmp(pend-pbegin+1, 0);
		reverse_copy(pbegin, pend, vchTmp.begin());

		// Convert little endian data to bignum
		CBigNum bn;
		bn.setvch(vchTmp);

		// Convert bignum to std::string
		std::string str;
		// Expected size increase from base58 conversion is approximately 137%
		// use 138% to be safe
		str.reserve((pend - pbegin) * 138 / 100 + 1);
		CBigNum dv;
		CBigNum rem;
		while (bn > bn0)
		{
			if (!BN_div(&dv, &rem, &bn, &bn58, pctx))
				throw bignum_error("EncodeBase58 : BN_div failed");
			bn = dv;
			unsigned int c = rem.getulong();
			str += pszBase58[c];
		}

		// Leading zeroes encoded as base58 zeros
		for (const unsigned char* p = pbegin; p < pend && *p == 0; p++)
			str += pszBase58[0];

		// Convert little endian std::string to big endian
		reverse(str.begin(), str.end());
		return str;
	}

	// Encode a byte vector as a base58-encoded string
	inline std::string EncodeBase58(const std::vector<unsigned char>& vch)
	{
		return EncodeBase58(&vch[0], &vch[0] + vch.size());
	}

	// Decode a base58-encoded string psz into byte vector vchRet
	// returns true if decoding is succesful
	inline bool DecodeBase58(const char* psz, std::vector<unsigned char>& vchRet)
	{
		CAutoBN_CTX pctx;
		vchRet.clear();
		CBigNum bn58 = 58;
		CBigNum bn = 0;
		CBigNum bnChar;
		while (isspace(*psz))
			psz++;

		// Convert big endian string to bignum
		for (const char* p = psz; *p; p++)
		{
			const char* p1 = strchr(pszBase58, *p);
			if (p1 == NULL)
			{
				while (isspace(*p))
					p++;
				if (*p != '\0')
					return false;
				break;
			}
			bnChar.setulong(p1 - pszBase58);
			if (!BN_mul(&bn, &bn, &bn58, pctx))
				throw bignum_error("DecodeBase58 : BN_mul failed");
			bn += bnChar;
		}

		// Get bignum as little endian data
		std::vector<unsigned char> vchTmp = bn.getvch();

		// Trim off sign byte if present
		if (vchTmp.size() >= 2 && vchTmp.end()[-1] == 0 && vchTmp.end()[-2] >= 0x80)
			vchTmp.erase(vchTmp.end()-1);

		// Restore leading zeros
		int nLeadingZeros = 0;
		for (const char* p = psz; *p == pszBase58[0]; p++)
			nLeadingZeros++;
		vchRet.assign(nLeadingZeros + vchTmp.size(), 0);

		// Convert little endian data to big endian
		reverse_copy(vchTmp.begin(), vchTmp.end(), vchRet.end() - vchTmp.size());
		return true;
	}

	// Decode a base58-encoded string str into byte vector vchRet
	// returns true if decoding is succesful
	inline bool DecodeBase58(const std::string& str, std::vector<unsigned char>& vchRet)
	{
		return DecodeBase58(str.c_str(), vchRet);
	}




	// Encode a byte vector to a base58-encoded string, including checksum
	inline std::string EncodeBase58Check(const std::vector<unsigned char>& vchIn)
	{
		// add 4-byte hash check to the end
		std::vector<unsigned char> vch(vchIn);
		uint256 hash = SK256(vch.begin(), vch.end());
		vch.insert(vch.end(), (unsigned char*)&hash, (unsigned char*)&hash + 4);
		return EncodeBase58(vch);
	}

	// Decode a base58-encoded string psz that includes a checksum, into byte vector vchRet
	// returns true if decoding is succesful
	inline bool DecodeBase58Check(const char* psz, std::vector<unsigned char>& vchRet)
	{
		if (!DecodeBase58(psz, vchRet))
			return false;
		if (vchRet.size() < 4)
		{
			vchRet.clear();
			return false;
		}
		uint256 hash = SK256(vchRet.begin(), vchRet.end()-4);
		if (memcmp(&hash, &vchRet.end()[-4], 4) != 0)
		{
			vchRet.clear();
			return false;
		}
		vchRet.resize(vchRet.size()-4);
		return true;
	}

	// Decode a base58-encoded string str that includes a checksum, into byte vector vchRet
	// returns true if decoding is succesful
	inline bool DecodeBase58Check(const std::string& str, std::vector<unsigned char>& vchRet)
	{
		return DecodeBase58Check(str.c_str(), vchRet);
	}





	/** Base class for all base58-encoded data */
	class CBase58Data
	{
	protected:
		// the version byte
		unsigned char nVersion;

		// the actually encoded data
		std::vector<unsigned char> vchData;

		CBase58Data()
		{
			nVersion = 0;
			vchData.clear();
		}

		~CBase58Data()
		{
			// zero the memory, as it may contain sensitive data
			if (!vchData.empty())
				memset(&vchData[0], 0, vchData.size());
		}

		void SetData(int nVersionIn, const void* pdata, size_t nSize)
		{
			nVersion = nVersionIn;
			vchData.resize(nSize);
			if (!vchData.empty())
				memcpy(&vchData[0], pdata, nSize);
		}

		void SetData(int nVersionIn, const unsigned char *pbegin, const unsigned char *pend)
		{
			SetData(nVersionIn, (void*)pbegin, pend - pbegin);
		}

	public:
		bool SetString(const char* psz)
		{
			std::vector<unsigned char> vchTemp;
			DecodeBase58Check(psz, vchTemp);
			if (vchTemp.empty())
			{
				vchData.clear();
				nVersion = 0;
				return false;
			}
			nVersion = vchTemp[0];
			vchData.resize(vchTemp.size() - 1);
			if (!vchData.empty())
				memcpy(&vchData[0], &vchTemp[1], vchData.size());
			memset(&vchTemp[0], 0, vchTemp.size());
			return true;
		}

		bool SetString(const std::string& str)
		{
			return SetString(str.c_str());
		}

		std::string ToString() const
		{
			std::vector<unsigned char> vch(1, nVersion);
			vch.insert(vch.end(), vchData.begin(), vchData.end());
			return EncodeBase58Check(vch);
		}

		int CompareTo(const CBase58Data& b58) const
		{
			if (nVersion < b58.nVersion) return -1;
			if (nVersion > b58.nVersion) return  1;
			if (vchData < b58.vchData)   return -1;
			if (vchData > b58.vchData)   return  1;
			return 0;
		}

		bool operator==(const CBase58Data& b58) const { return CompareTo(b58) == 0; }
		bool operator<=(const CBase58Data& b58) const { return CompareTo(b58) <= 0; }
		bool operator>=(const CBase58Data& b58) const { return CompareTo(b58) >= 0; }
		bool operator< (const CBase58Data& b58) const { return CompareTo(b58) <  0; }
		bool operator> (const CBase58Data& b58) const { return CompareTo(b58) >  0; }
	};

	/** base58-encoded bitcoin addresses.
	 * Public-key-hash-addresses have version 55 (or 111 testnet).
	 * The data vector contains RIPEMD160(SHA256(pubkey)), where pubkey is the serialized public key.
	 * Script-hash-addresses have version 117 (or 196 testnet).
	 * The data vector contains RIPEMD160(SHA256(cscript)), where cscript is the serialized redemption script.
	 */
	class NexusAddress : public CBase58Data
	{
	public:
		enum
		{
			PUBKEY_ADDRESS = 42,
			SCRIPT_ADDRESS = 104,
			PUBKEY_ADDRESS_TEST = 111,
            SCRIPT_ADDRESS_TEST = 196,
		};

		bool SetHash256(const uint256& hash256)
		{
			SetData(Core::CONFIG.fTestNet ? PUBKEY_ADDRESS_TEST : PUBKEY_ADDRESS, &hash256, 32);
			return true;
		}

		void SetPubKey(const std::vector<unsigned char>& vchPubKey)
		{
			SetHash256(SK256(vchPubKey));
		}

		bool SetScriptHash256(const uint256& hash256)
		{
			SetData(Core::CONFIG.fTestNet ? SCRIPT_ADDRESS_TEST : SCRIPT_ADDRESS, &hash256, 32);
			return true;
		}

		bool IsValid() const
		{
			unsigned int nExpectedSize = 32;
            bool fExpectTestNet = false;
            switch(nVersion)
            {
                case PUBKEY_ADDRESS:
                    nExpectedSize = 32; // Hash of public key
                    fExpectTestNet = false;
                    break;
                case SCRIPT_ADDRESS:
                    nExpectedSize = 32; // Hash of CScript
                    fExpectTestNet = false;
                    break;

                case PUBKEY_ADDRESS_TEST:
                    nExpectedSize = 32;
                    fExpectTestNet = true;
                    break;
                case SCRIPT_ADDRESS_TEST:
                    nExpectedSize = 32;
                    fExpectTestNet = true;
                    break;

                default:
                    return false;
            }
            return fExpectTestNet == Core::CONFIG.fTestNet && vchData.size() == nExpectedSize;
		}
		bool IsScript() const
		{
			if (!IsValid())
				return false;
				
			return nVersion == SCRIPT_ADDRESS;
		}

		NexusAddress()
		{
		}

		NexusAddress(uint256 hash256In)
		{
			SetHash256(hash256In);
		}

		NexusAddress(const std::vector<unsigned char>& vchPubKey)
		{
			SetPubKey(vchPubKey);
		}

		NexusAddress(const std::string& strAddress)
		{
			SetString(strAddress);
		}

		NexusAddress(const char* pszAddress)
		{
			SetString(pszAddress);
		}

		uint256 GetHash256() const
		{
			assert(vchData.size() == 32);
			uint256 hash256;
			memcpy(&hash256, &vchData[0], 32);
			return hash256;
		}
	};
}

#endif

