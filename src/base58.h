/*******************************************************************************************
 
			Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++
   
 [Learn, Create, but do not Forge] Viz. http://www.opensource.org/licenses/mit-license.php
  
*******************************************************************************************/

#ifndef COINSHIELD_BASE58_H
#define COINSHIELD_BASE58_H

#include <string>
#include <vector>

#include "hash/templates.h"

namespace Core
{
	static const char* pszBase58 = "123456789ABCDEFGHJKLMNPQRSTUVWXYZabcdefghijkmnopqrstuvwxyz";

	static const int8_t mapBase58[256] = {
	-1,-1,-1,-1,-1,-1,-1,-1, -1,-1,-1,-1,-1,-1,-1,-1,
	-1,-1,-1,-1,-1,-1,-1,-1, -1,-1,-1,-1,-1,-1,-1,-1,
	-1,-1,-1,-1,-1,-1,-1,-1, -1,-1,-1,-1,-1,-1,-1,-1,
	-1, 0, 1, 2, 3, 4, 5, 6,  7, 8,-1,-1,-1,-1,-1,-1,
	-1, 9,10,11,12,13,14,15, 16,-1,17,18,19,20,21,-1,
	22,23,24,25,26,27,28,29, 30,31,32,-1,-1,-1,-1,-1,
	-1,33,34,35,36,37,38,39, 40,41,42,43,-1,44,45,46,
	47,48,49,50,51,52,53,54, 55,56,57,-1,-1,-1,-1,-1,
	-1,-1,-1,-1,-1,-1,-1,-1, -1,-1,-1,-1,-1,-1,-1,-1,
	-1,-1,-1,-1,-1,-1,-1,-1, -1,-1,-1,-1,-1,-1,-1,-1,
	-1,-1,-1,-1,-1,-1,-1,-1, -1,-1,-1,-1,-1,-1,-1,-1,
	-1,-1,-1,-1,-1,-1,-1,-1, -1,-1,-1,-1,-1,-1,-1,-1,
	-1,-1,-1,-1,-1,-1,-1,-1, -1,-1,-1,-1,-1,-1,-1,-1,
	-1,-1,-1,-1,-1,-1,-1,-1, -1,-1,-1,-1,-1,-1,-1,-1,
	-1,-1,-1,-1,-1,-1,-1,-1, -1,-1,-1,-1,-1,-1,-1,-1,
	-1,-1,-1,-1,-1,-1,-1,-1, -1,-1,-1,-1,-1,-1,-1,-1,
	};

		/**
	 * Tests if the given character is a whitespace character. The whitespace characters
	 * are: space, form-feed ('\f'), newline ('\n'), carriage return ('\r'), horizontal
	 * tab ('\t'), and vertical tab ('\v').
	 *
	 * This function is locale independent. Under the C locale this function gives the
	 * same result as std::isspace.
	 *
	 * @param[in] c     character to test
	 * @return          true if the argument is a whitespace character; otherwise false
	 */
	constexpr inline bool IsSpace(char c) noexcept 
	{
		return c == ' ' || c == '\f' || c == '\n' || c == '\r' || c == '\t' || c == '\v';
	}

	inline std::string EncodeBase58(const unsigned char* pbegin, const unsigned char* pend)
	{
		// Skip & count leading zeroes.
		int zeroes = 0;
		int length = 0;
		while (pbegin != pend && *pbegin == 0) {
			pbegin++;
			zeroes++;
		}
		// Allocate enough space in big-endian base58 representation.
		int size = (pend - pbegin) * 138 / 100 + 1; // log(256) / log(58), rounded up.
		std::vector<unsigned char> b58(size);
		// Process the bytes.
		while (pbegin != pend) {
			int carry = *pbegin;
			int i = 0;
			// Apply "b58 = b58 * 256 + ch".
			for (std::vector<unsigned char>::reverse_iterator it = b58.rbegin(); (carry != 0 || i < length) && (it != b58.rend()); it++, i++) {
				carry += 256 * (*it);
				*it = carry % 58;
				carry /= 58;
			}

			assert(carry == 0);
			length = i;
			pbegin++;
		}
		// Skip leading zeroes in base58 result.
		std::vector<unsigned char>::iterator it = b58.begin() + (size - length);
		while (it != b58.end() && *it == 0)
			it++;
		// Translate the result into a string.
		std::string str;
		str.reserve(zeroes + (b58.end() - it));
		str.assign(zeroes, '1');
		while (it != b58.end())
			str += pszBase58[*(it++)];
		return str;
	}

	//// Encode a byte sequence as a base58-encoded string
	//inline std::string EncodeBase58(const unsigned char* pbegin, const unsigned char* pend)
	//{
	//	CAutoBN_CTX pctx;
	//	CBigNum bn58 = 58;
	//	CBigNum bn0 = 0;

	//	// Convert big endian data to little endian
	//	// Extra zero at the end make sure bignum will interpret as a positive number
	//	std::vector<unsigned char> vchTmp(pend-pbegin+1, 0);
	//	reverse_copy(pbegin, pend, vchTmp.begin());

	//	// Convert little endian data to bignum
	//	CBigNum bn;
	//	bn.setvch(vchTmp);

	//	// Convert bignum to std::string
	//	std::string str;
	//	// Expected size increase from base58 conversion is approximately 137%
	//	// use 138% to be safe
	//	str.reserve((pend - pbegin) * 138 / 100 + 1);
	//	CBigNum dv;
	//	CBigNum rem;
	//	while (bn > bn0)
	//	{
	//		if (!BN_div(&dv, &rem, &bn, &bn58, pctx))
	//			throw bignum_error("EncodeBase58 : BN_div failed");
	//		bn = dv;
	//		unsigned int c = rem.getulong();
	//		str += pszBase58[c];
	//	}

	//	// Leading zeroes encoded as base58 zeros
	//	for (const unsigned char* p = pbegin; p < pend && *p == 0; p++)
	//		str += pszBase58[0];

	//	// Convert little endian std::string to big endian
	//	reverse(str.begin(), str.end());
	//	return str;
	//}

	// Encode a byte vector as a base58-encoded string
	inline std::string EncodeBase58(const std::vector<unsigned char>& vch)
	{
		return EncodeBase58(&vch[0], &vch[0] + vch.size());
	}

	inline bool DecodeBase58(const char* psz, std::vector<unsigned char>& vch)
	{
		// Skip leading spaces.
		while (*psz && IsSpace(*psz))
			psz++;
		// Skip and count leading '1's.
		int zeroes = 0;
		int length = 0;
		while (*psz == '1') {
			zeroes++;
			psz++;
		}
		// Allocate enough space in big-endian base256 representation.
		int size = strlen(psz) * 733 / 1000 + 1; // log(58) / log(256), rounded up.
		std::vector<unsigned char> b256(size);
		// Process the characters.
		static_assert(sizeof(mapBase58) / sizeof(mapBase58[0]) == 256, "mapBase58.size() should be 256"); // guarantee not out of range
		while (*psz && !IsSpace(*psz)) {
			// Decode base58 character
			int carry = mapBase58[(uint8_t)*psz];
			if (carry == -1)  // Invalid b58 character
				return false;
			int i = 0;
			for (std::vector<unsigned char>::reverse_iterator it = b256.rbegin(); (carry != 0 || i < length) && (it != b256.rend()); ++it, ++i) {
				carry += 58 * (*it);
				*it = carry % 256;
				carry /= 256;
			}
			assert(carry == 0);
			length = i;
			psz++;
		}
		// Skip trailing spaces.
		while (IsSpace(*psz))
			psz++;
		if (*psz != 0)
			return false;
		// Skip leading zeroes in b256.
		std::vector<unsigned char>::iterator it = b256.begin() + (size - length);
		while (it != b256.end() && *it == 0)
			it++;
		// Copy result into output vector.
		vch.reserve(zeroes + (b256.end() - it));
		vch.assign(zeroes, 0x00);
		while (it != b256.end())
			vch.push_back(*(it++));
		return true;
	}

	//// Decode a base58-encoded string psz into byte vector vchRet
	//// returns true if decoding is succesful
	//inline bool DecodeBase58(const char* psz, std::vector<unsigned char>& vchRet)
	//{
	//	CAutoBN_CTX pctx;
	//	vchRet.clear();
	//	CBigNum bn58 = 58;
	//	CBigNum bn = 0;
	//	CBigNum bnChar;
	//	while (isspace(*psz))
	//		psz++;

	//	// Convert big endian string to bignum
	//	for (const char* p = psz; *p; p++)
	//	{
	//		const char* p1 = strchr(pszBase58, *p);
	//		if (p1 == NULL)
	//		{
	//			while (isspace(*p))
	//				p++;
	//			if (*p != '\0')
	//				return false;
	//			break;
	//		}
	//		bnChar.setulong(p1 - pszBase58);
	//		if (!BN_mul(&bn, &bn, &bn58, pctx))
	//			throw bignum_error("DecodeBase58 : BN_mul failed");
	//		bn += bnChar;
	//	}

	//	// Get bignum as little endian data
	//	std::vector<unsigned char> vchTmp = bn.getvch();

	//	// Trim off sign byte if present
	//	if (vchTmp.size() >= 2 && vchTmp.end()[-1] == 0 && vchTmp.end()[-2] >= 0x80)
	//		vchTmp.erase(vchTmp.end()-1);

	//	// Restore leading zeros
	//	int nLeadingZeros = 0;
	//	for (const char* p = psz; *p == pszBase58[0]; p++)
	//		nLeadingZeros++;
	//	vchRet.assign(nLeadingZeros + vchTmp.size(), 0);

	//	// Convert little endian data to big endian
	//	reverse_copy(vchTmp.begin(), vchTmp.end(), vchRet.end() - vchTmp.size());
	//	return true;
	//}

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

