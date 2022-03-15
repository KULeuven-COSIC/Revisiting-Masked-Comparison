/*
 * MIT License
 *
 * Copyright (c) 2021-2022: imec-COSIC KU Leuven, 3001 Leuven, Belgium 
 * Authors: Jan-Pieter D'Anvers <janpieter.danvers@esat.kuleuven.be>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include "randombytes.h"
#include "SecMult.h"

// SPOG19
void secMult(uint32_t nshares, uint32_t q, uint32_t out[nshares], uint32_t A[nshares], uint32_t B[nshares])
{
	
	// produce randomness
	uint32_t iterations = (nshares * (nshares - 1)) / 2;
	uint32_t R[iterations];
	uint32_t Ridx;
	uint32_t r;

	for(uint32_t i = 0; i<iterations; i+=2)
	{
		randomq(&R[i], q);
	}

	for(uint32_t i=0; i<nshares; i++)
	{
		out[i] = (A[i] * B[i]) % q;
	}

	Ridx = 0;
	for(uint32_t i=0; i<nshares; i++)
	{
		for(uint32_t j=i+1; j<nshares; j++)
		{
			r = (R[Ridx] + A[i] * B[j]) % q;
			r += (A[j] * B[i]) % q;
			out[i] = (out[i] - R[Ridx] + q) % q;
			out[j] = (out[j] + r) % q;
			Ridx++;
		}
	}

	#ifdef DEBUG
	uint64_t x_unmasked = 0;
	uint64_t y_unmasked = 0;
	uint64_t z_unmasked = 0;

	for (size_t j = 0; j < nshares; j++)
	{
		x_unmasked = (x_unmasked + A[j]) % q;
		y_unmasked = (y_unmasked + B[j]) % q;
		z_unmasked = (z_unmasked + out[j]) % q;
	}
	// printf("\n%d, %d, %d\n", x_unmasked, y_unmasked, z_unmasked);

	assert(z_unmasked == (x_unmasked * y_unmasked) % q);
	#endif
}

uint32_t uncompress(uint32_t x, uint32_t compressto, uint32_t q)
{
	uint32_t out;
	out = ((x - 1) % (1 << compressto)) + 1; // avoid underflow next line
    out = out * q - (q >> 1); // undo division and rounding
    out = out + (1 << compressto) - 1; // round up
    out = out >> compressto; 
    out = out % q;
	return out;
}

void randomq(uint32_t out[2], uint32_t q)
{
	#ifdef DEBUG
	assert(q==3329);
	#endif

	uint32_t r;
	do
	{
		r = random_uint32();
	} while (r > 387 * q * q);

	r = r % (q * q);
	out[0] = r % q;
	out[1] = r / q;
}
