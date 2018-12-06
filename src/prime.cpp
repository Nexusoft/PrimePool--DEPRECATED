/*******************************************************************************************
 
			Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++
   
 [Learn, Create, but do not Forge] Viz. http://www.opensource.org/licenses/mit-license.php
  
*******************************************************************************************/

#include "core.h"
#include "LLP/types.h"

using namespace std;

namespace Core
{
	static mpz_t zTwo;

	
	/** Divisor Sieve for Prime Searching. **/
	std::vector<unsigned int> PRIME_SIEVE;
	
	
	/** Sieve of Eratosthenes for Divisor Tests. Used for Searching Primes. **/
	std::vector<unsigned int> Eratosthenes(unsigned int nSieveSize)
	{
		std::vector<bool>TABLE;
		TABLE.resize(nSieveSize);
		
		for(unsigned int nIndex = 0; nIndex < nSieveSize; nIndex++)
			TABLE[nIndex] = false;
			
			
		for(unsigned int nIndex = 2; nIndex < nSieveSize; nIndex++)
			for(unsigned int nComposite = 2; (nComposite * nIndex) < nSieveSize; nComposite++)
				TABLE[nComposite * nIndex] = true;
		
		
		std::vector<unsigned int> PRIMES;
		for(unsigned int nIndex = 2; nIndex < nSieveSize; nIndex++)
			if(!TABLE[nIndex])
				PRIMES.push_back(nIndex);

		
		printf("Sieve of Eratosthenes Generated %i Primes.\n", PRIMES.size());
		
		return PRIMES;
	}
	
	/** Initialize the Divisor Sieve. **/
	void InitializePrimes()
	{
		mpz_init_set_ui(zTwo, 2);
		PRIME_SIEVE = Eratosthenes(80);
	}
	
	
	/** Convert Double to unsigned int Representative. Used for encoding / decoding prime difficulty from nBits. **/
	unsigned int SetBits(double nDiff)
	{
		unsigned int nBits = 10000000;
		nBits *= nDiff;
		
		return nBits;
	}
	
	
	/** Convert unsigned int nBits to Decimal. **/
	double GetDifficulty(unsigned int nBits) { return nBits / 10000000.0; }
	
	bool GmpDivisor(mpz_t zPrime, mpz_t zRemainder)
	{
		for(int nIndex = 0; nIndex < PRIME_SIEVE.size(); nIndex++)
		{
			if(mpz_divisible_ui_p(zPrime, PRIME_SIEVE[nIndex]) != 0)
			   return false;
		}
			
		return true;
	}
	
	
	bool GmpFermat(mpz_t zPrime, mpz_t zN, mpz_t zRemainder)
	{
		mpz_sub_ui(zN, zPrime, 1);
		mpz_powm(zRemainder, zTwo, zN, zPrime);
		if (mpz_cmp_ui(zRemainder, 1) != 0)
			return false;
			
		return true;
	}
	
	
	bool GmpPrimeTest(mpz_t zPrime, mpz_t zN, mpz_t zRemainder)
	{
		if(!GmpDivisor(zPrime, zRemainder))
			return false;
		
		if(!GmpFermat(zPrime, zN, zRemainder))
			return false;
			
		return true;
	}
	
	double GmpVerification(uint1024 prime)
	{
		mpz_t zN, zRemainder, zPrime;
		
		mpz_init(zN);
		mpz_init(zRemainder);
		mpz_init(zPrime);

		mpz_import(zPrime, 32, -1, sizeof(uint32_t), 0, 0, prime.data());
		
		if(!GmpPrimeTest(zPrime, zN, zRemainder))
		{
			mpz_clear(zPrime);
			mpz_clear(zN);
			mpz_clear(zRemainder);
			
			return 0.0;
		}
		
		unsigned int nClusterSize = 1, nPrimeGap = 2, nOffset = 0;
		while(nPrimeGap <= 12)
		{
			mpz_add_ui(zPrime, zPrime, 2);
			nOffset += 2;
			
			if(!GmpPrimeTest(zPrime, zN, zRemainder))
			{
				nPrimeGap += 2;
				
				continue;
			}
			
			nClusterSize++;
			nPrimeGap = 2;
		}
		
		/** Calulate the rarety of cluster from proportion of fermat remainder of last prime + 2
			Keep fractional remainder in bounds of [0, 1] **/
		double dFractionalRemainder = 1000000.0 / GetFractionalDifficulty(prime + nOffset + 2);
		if(dFractionalRemainder > 1.0 || dFractionalRemainder < 0.0)
			dFractionalRemainder = 0.0;
			
		mpz_clear(zPrime);
		mpz_clear(zN);
		mpz_clear(zRemainder);
			
		return nClusterSize + dFractionalRemainder;
	}

	/** Simple Modular Exponential Equation a^(n - 1) % n == 1 or notated in
	Modular Arithmetic a^(n - 1) = 1 [mod n]. **/
	uint1024 FermatTest(uint1024 n)
	{
		uint1024 r;
		mpz_t zR, zE, zN, zA;
		mpz_init(zR);
		mpz_init(zE);
		mpz_init(zN);
		mpz_init_set_ui(zA, 2);

		mpz_import(zN, 32, -1, sizeof(uint32_t), 0, 0, n.data());

		mpz_sub_ui(zE, zN, 1);
		mpz_powm(zR, zA, zE, zN);

		mpz_export(r.data(), 0, -1, sizeof(uint32_t), 0, 0, zR);

		mpz_clear(zR);
		mpz_clear(zE);
		mpz_clear(zN);
		mpz_clear(zA);

		return r;
	}

	/** Breaks the remainder of last composite in Prime Cluster into an integer. 
		Larger numbers are more rare to find, so a proportion can be determined 
		to give decimal difficulty between whole number increases. **/
	unsigned int GetFractionalDifficulty(uint1024 composite)
	{
		/** Break the remainder of Fermat test to calculate fractional difficulty [Thanks Sunny] **/
		mpz_t zA, zB, zC, zN;
		mpz_init(zA);
		mpz_init(zB);
		mpz_init(zC);
		mpz_init(zN);

		mpz_import(zB, 32, -1, sizeof(uint32_t), 0, 0, FermatTest(composite).data());
		mpz_import(zC, 32, -1, sizeof(uint32_t), 0, 0, composite.data());
		mpz_sub(zA, zC, zB);
		mpz_mul_2exp(zA, zA, 24);

		mpz_tdiv_q(zN, zA, zC);

		uint32_t diff = mpz_get_ui(zN);

		mpz_clear(zA);
		mpz_clear(zB);
		mpz_clear(zC);
		mpz_clear(zN);

		return diff;
	}

}

