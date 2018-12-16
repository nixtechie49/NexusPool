#ifndef NEXUSPOOL_PRIME_HPP
#define NEXUSPOOL_PRIME_HPP

#include "hash/uint1024.h"
#include "hash/templates.h"
#include "utils.hpp"
#ifdef _WIN32_WINNT
#include <mpir/mpir.h>
#else
#include <gmp.h>
#endif

#include <vector>

namespace nexuspool {

	static mpz_t zTwo;


	/** Divisor Sieve for Prime Searching. **/
	std::vector<uint32_t> PRIME_SIEVE;


	/** Sieve of Eratosthenes for Divisor Tests. Used for Searching Primes. **/
	inline std::vector<uint32_t> eratos_thenes(uint32_t sieve_size)
	{
		std::vector<bool>TABLE;
		TABLE.resize(sieve_size);

		for (uint32_t nIndex = 0; nIndex < sieve_size; nIndex++)
			TABLE[nIndex] = false;


		for (uint32_t nIndex = 2; nIndex < sieve_size; nIndex++)
			for (uint32_t nComposite = 2; (nComposite * nIndex) < sieve_size; nComposite++)
				TABLE[nComposite * nIndex] = true;


		std::vector<uint32_t> PRIMES;
		for (uint32_t nIndex = 2; nIndex < sieve_size; nIndex++)
		{
			if (!TABLE[nIndex])
				PRIMES.push_back(nIndex);
		}

	//	printf("Sieve of Eratosthenes Generated %i Primes.\n", PRIMES.size());

		return PRIMES;
	}

	/** Initialize the Divisor Sieve. **/
	inline void InitializePrimes()
	{
		mpz_init_set_ui(zTwo, 2);
		PRIME_SIEVE = eratos_thenes(80);
	}


	/** Convert Double to unsigned int Representative. Used for encoding / decoding prime difficulty from nBits. **/
	inline uint32_t set_bits(double diff)
	{
		uint32_t bits = 10000000;
		bits *= diff;

		return bits;
	}


	/** Convert unsigned int nBits to Decimal. **/
	inline double get_difficulty(uint32_t bits) { return bits / 10000000.0; }

	inline bool gmp_divisor(mpz_t prime)
	{
		for (size_t nIndex = 0; nIndex < PRIME_SIEVE.size(); nIndex++)
		{
			if (mpz_divisible_ui_p(prime, PRIME_SIEVE[nIndex]) != 0)
				return false;
		}

		return true;
	}


	inline bool gmp_fermat(mpz_t prime, mpz_t zN, mpz_t remainder)
	{
		mpz_sub_ui(zN, prime, 1);
		mpz_powm(remainder, zTwo, zN, prime);
		if (mpz_cmp_ui(remainder, 1) != 0)
			return false;

		return true;
	}


	inline bool gmp_prime_test(mpz_t prime, mpz_t zN, mpz_t remainder)
	{
		if (!gmp_divisor(prime))
			return false;

		if (!gmp_fermat(prime, zN, remainder))
			return false;

		return true;
	}

	/** Simple Modular Exponential Equation a^(n - 1) % n == 1 or notated in
Modular Arithmetic a^(n - 1) = 1 [mod n]. **/
	inline uint1024 fermat_test(uint1024 n)
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
	inline uint32_t get_fractional_difficulty(uint1024 composite)
	{
		/** Break the remainder of Fermat test to calculate fractional difficulty [Thanks Sunny] **/
		mpz_t zA, zB, zC, zN;
		mpz_init(zA);
		mpz_init(zB);
		mpz_init(zC);
		mpz_init(zN);

		mpz_import(zB, 32, -1, sizeof(uint32_t), 0, 0, fermat_test(composite).data());
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

	inline double gmp_verification(uint1024 prime)
	{
		mpz_t zN, zRemainder, zPrime;

		mpz_init(zN);
		mpz_init(zRemainder);
		mpz_init(zPrime);

		mpz_import(zPrime, 32, -1, sizeof(uint32_t), 0, 0, prime.data());

		if (!gmp_prime_test(zPrime, zN, zRemainder))
		{
			mpz_clear(zPrime);
			mpz_clear(zN);
			mpz_clear(zRemainder);

			return 0.0;
		}

		uint32_t nClusterSize = 1, nPrimeGap = 2, nOffset = 0;
		while (nPrimeGap <= 12)
		{
			mpz_add_ui(zPrime, zPrime, 2);
			nOffset += 2;

			if (!gmp_prime_test(zPrime, zN, zRemainder))
			{
				nPrimeGap += 2;

				continue;
			}

			nClusterSize++;
			nPrimeGap = 2;
		}

		/** Calulate the rarety of cluster from proportion of fermat remainder of last prime + 2
			Keep fractional remainder in bounds of [0, 1] **/
		double dFractionalRemainder = 1000000.0 / get_fractional_difficulty(prime + nOffset + 2);
		if (dFractionalRemainder > 1.0 || dFractionalRemainder < 0.0)
			dFractionalRemainder = 0.0;

		mpz_clear(zPrime);
		mpz_clear(zN);
		mpz_clear(zRemainder);

		return nClusterSize + dFractionalRemainder;
	}
}

#endif