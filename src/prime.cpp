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
		bool TABLE[nSieveSize];
		
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
	
	
	/** Quick Check of Prime.
		Difficulty is represented as so V.X
		V is the whole number, or Cluster Size, X is a proportion
		of Fermat Remainder from last Composite Number [0 - 1] **/
	double VerifyPrimeDifficulty(CBigNum prime, int checks)
	{
		if(!PrimeCheck(prime, checks))
			return 0.0;
			
		CBigNum lastPrime = prime;
		CBigNum next = prime + 2;
		unsigned int clusterSize = 1;
		
		/** Largest prime gap in cluster can be +12
			This was determined by previously found clusters up to 17 primes **/
		for( next ; next <= lastPrime + 12; next += 2)
		{
			if(PrimeCheck(next, checks))
			{
				lastPrime = next;
				++clusterSize;
			}
		}
		
		/** Calulate the rarety of cluster from proportion of fermat remainder of last prime + 2
			Keep fractional remainder in bounds of [0, 1] **/
		double fractionalRemainder = 1000000.0 / GetFractionalDifficulty(next);
		if(fractionalRemainder > 1.0 || fractionalRemainder < 0.0)
			fractionalRemainder = 0.0;
		
		return (clusterSize + fractionalRemainder);
	}
	
	
	/** Basic Search filter to determine if further tests should be done. **/
	bool DivisorCheck(CBigNum bnTest)
	{
		for(int index = 0; index < PRIME_SIEVE.size(); index++)
			if(bnTest % PRIME_SIEVE[index] == 0)
			   return false;
			
				
		return true;
	}


	/** Quick Check to Determine Prime Cluster Difficulty. **/
	double CheckPrimeDifficulty(CBigNum prime)
	{
		if(FermatTest(prime, 2) != 1)
			return 0.0;
			
		CBigNum lastPrime = prime;
		CBigNum next = prime + 2;
		unsigned int clusterSize = 1;
		
		/** Largest prime gap in cluster can be +12
			This was determined by previously found clusters up to 17 primes **/
		for( next ; next <= lastPrime + 12; next += 2)
		{
			if(!DivisorCheck(next))
				continue;
				
			if(FermatTest(next, 2) == 1)
			{
				lastPrime = next;
				++clusterSize;
			}
		}
		
		/** Calulate the rarety of cluster from proportion of fermat remainder of last prime + 2
			Keep fractional remainder in bounds of [0, 1] **/
		double fractionalRemainder = 1000000.0 / GetFractionalDifficulty(next);
		if(fractionalRemainder > 1.0 || fractionalRemainder < 0.0)
			fractionalRemainder = 0.0;
		
		return (clusterSize + fractionalRemainder);
	}
	
	
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
	
	double GmpVerification(CBigNum prime)
	{
		mpz_t zN, zRemainder, zPrime;
		
		mpz_init(zN);
		mpz_init(zRemainder);
		mpz_init(zPrime);
		
		bignum2mpz(&prime, zPrime);
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


	/** Breaks the remainder of last composite in Prime Cluster into an integer. 
		Larger numbers are more rare to find, so a proportion can be determined 
		to give decimal difficulty between whole number increases. **/
	unsigned int GetFractionalDifficulty(CBigNum composite)
	{
		/** Break the remainder of Fermat test to calculate fractional difficulty [Thanks Sunny] **/
		return ((composite - FermatTest(composite, 2) << 24) / composite).getuint();
	}

	
	/** Determines if given number is Prime. Accuracy can be determined by "checks". 
		The default checks the Niro Network uses is 2 **/
	bool PrimeCheck(CBigNum test, int checks)
	{
		/** Check A: Divisor Test **/
		/** Check A: Small Prime Divisor Tests */
		CBigNum primes[4] = { 2, 3, 5, 7 };
		for(int index = 0; index < min(checks, 4); index++)
			if(test % primes[index] == 0)
				return false;
		
		/** Check B: Miller-Rabin Tests */
		if(!Miller_Rabin(test, checks))
			return false;
			
		/** Check C: Fermat Tests */
		for(CBigNum n = 2; n < 2 + checks; n++)
			if(FermatTest(test, n) != 1)
				return false;
		
		return true;
	}

	
	/** Simple Modular Exponential Equation a^(n - 1) % n == 1 or notated in Modular Arithmetic a^(n - 1) = 1 [mod n]. 
		a = Base or 2... 2 + checks, n is the Prime Test. Used after Miller-Rabin and Divisor tests to verify primality. **/
	CBigNum FermatTest(CBigNum n, CBigNum a)
	{
		CAutoBN_CTX pctx;
		CBigNum e = n - 1;
		CBigNum r;
		BN_mod_exp(&r, &a, &e, &n, pctx);
		
		return r;
	}

	
	/** Miller-Rabin Primality Test from the OpenSSL BN Library. **/
	bool Miller_Rabin(CBigNum n, int checks) { return (BN_is_prime(&n, checks, NULL, NULL, NULL) == 1); }

}

