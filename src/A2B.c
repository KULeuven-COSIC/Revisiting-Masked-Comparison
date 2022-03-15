/*
 * MIT License
 *
 * Copyright (c) 2021-2022: imec-COSIC KU Leuven, 3001 Leuven, Belgium
 * Authors: Michiel Van Beirendonck <michiel.vanbeirendonck@esat.kuleuven.be>
 *          Jan-Pieter D'Anvers <janpieter.danvers@esat.kuleuven.be>
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

#include "A2B.h"
#include "SecAdd.h"
#include "randombytes.h"

#ifdef DEBUG
#include "bitmask.h"
#endif

// [https://eprint.iacr.org/2018/381.pdf, Algorithm 8]
static void RefreshXOR(size_t from, size_t to, uint64_t x[to])
{
    for (size_t i = from; i < to; i++)
    {
        x[i] = 0;
    }

    for (size_t i = 0; i < to - 1; i++)
    {
        for (size_t j = i + 1; j < to; j++)
        {
            uint64_t R = random_uint64();
            x[i] ^= R;
            x[j] ^= R;
        }
    }
}

static void RefreshXOR32(size_t from, size_t to, uint32_t x[to])
{
    for (size_t i = from; i < to; i++)
    {
        x[i] = 0;
    }

    for (size_t i = 0; i < to - 1; i++)
    {
        for (size_t j = i + 1; j < to; j++)
        {
            uint32_t R = random_uint32();
            x[i] ^= R;
            x[j] ^= R;
        }
    }
}

static void RefreshXOR_bitsliced(size_t from, size_t to, size_t nbits, uint32_t x[to][nbits])
{
    for (size_t i = from; i < to; i++)
    {
        for (size_t k = 0; k < nbits; k++)
        {
            x[i][k] = 0;
        }
    }

    for (size_t i = 0; i < to - 1; i++)
    {
        for (size_t j = i + 1; j < to; j++)
        {
            for (size_t k = 0; k < nbits; k++)
            {
                uint32_t R = random_uint32();
                x[i][k] ^= R;
                x[j][k] ^= R;
            }
        }
    }
}

// [http://www.crypto-uni.lu/jscoron/publications/secconvorder.pdf, Algorithm 4]
void A2B(size_t nshares, uint64_t B[nshares], const uint64_t A[nshares])
{
    if (nshares == 1)
    {
        B[0] = A[0];
        return;
    }

    uint64_t x[nshares], y[nshares];

    A2B(nshares / 2, &x[0], &A[0]);
    RefreshXOR(nshares / 2, nshares, x);
    A2B(nshares - (nshares / 2), &y[0], &A[nshares / 2]);
    RefreshXOR(nshares - (nshares / 2), nshares, y);
    SecAdd(nshares, B, x, y);

#ifdef DEBUG
    uint64_t A_unmasked = 0;
    uint64_t B_unmasked = 0;

    for (size_t j = 0; j < nshares; j++)
    {
        A_unmasked = A_unmasked + A[j];
        B_unmasked = B_unmasked ^ B[j];
    }

    assert(A_unmasked == B_unmasked);
#endif
}

// [http://www.crypto-uni.lu/jscoron/publications/secconvorder.pdf, Algorithm 4]
void A2B32(size_t nshares, uint32_t B[nshares], const uint32_t A[nshares])
{
    if (nshares == 1)
    {
        B[0] = A[0];
        return;
    }

    uint32_t x[nshares], y[nshares];

    A2B32(nshares / 2, &x[0], &A[0]);
    RefreshXOR32(nshares / 2, nshares, x);
    A2B32(nshares - (nshares / 2), &y[0], &A[nshares / 2]);
    RefreshXOR32(nshares - (nshares / 2), nshares, y);
    SecAdd32(nshares, B, x, y);

#ifdef DEBUG
    uint32_t A_unmasked = 0;
    uint32_t B_unmasked = 0;

    for (size_t j = 0; j < nshares; j++)
    {
        A_unmasked = A_unmasked + A[j];
        B_unmasked = B_unmasked ^ B[j];
    }

    assert(A_unmasked == B_unmasked);
#endif
}

static void A2B_bitsliced_inner(size_t nshares, size_t nbits, uint32_t B_bitsliced[nshares][nbits], const uint32_t A_bitsliced[nshares][nbits])
{
    if (nshares == 1)
    {
        for (size_t i = 0; i < nbits; i++)
        {
            B_bitsliced[0][i] = A_bitsliced[0][i];
        }
        return;
    }

    uint32_t x[nshares][nbits], y[nshares][nbits];

    A2B_bitsliced_inner(nshares / 2, nbits, &x[0], &A_bitsliced[0]);
    RefreshXOR_bitsliced(nshares / 2, nshares, nbits, x);
    A2B_bitsliced_inner(nshares - (nshares / 2), nbits, &y[0], &A_bitsliced[nshares / 2]);
    RefreshXOR_bitsliced(nshares - (nshares / 2), nshares, nbits, y);
    SecAdd_bitsliced(nshares, nbits, B_bitsliced, x, y);
}

static void pack_bitslice(size_t nshares, size_t nbits, uint32_t x_bitsliced[nshares][nbits], const uint32_t x[32][nshares])
{
    for (size_t j = 0; j < nshares; j++)
    {
        for (size_t k = 0; k < nbits; k++)
        {
            x_bitsliced[j][k] = 0;
        }
    }

    for (size_t i = 0; i < 32; i++)
    {
        for (size_t j = 0; j < nshares; j++)
        {
            for (size_t k = 0; k < nbits; k++)
            {
                x_bitsliced[j][k] = x_bitsliced[j][k] | (((x[i][j] >> k) & 1) << i);
            }
        }
    }
}

static void unpack_bitslice(size_t nshares, size_t nbits, uint32_t x[32][nshares], uint32_t x_bitsliced[nshares][nbits])
{
    for (size_t i = 0; i < 32; i++)
    {
        for (size_t j = 0; j < nshares; j++)
        {
            uint32_t tmp = 0;

            for (size_t k = 0; k < nbits; k++)
            {
                tmp |= ((x_bitsliced[j][k] & (1 << i)) >> i) << k;
            }

            x[i][j] = tmp;
        }
    }
}

void A2B_bitsliced(size_t nshares, size_t nbits, uint32_t B[32][nshares], const uint32_t A[32][nshares])
{
    uint32_t A_bitsliced[nshares][nbits];
    uint32_t B_bitsliced[nshares][nbits];

    pack_bitslice(nshares, nbits, A_bitsliced, A);
    A2B_bitsliced_inner(nshares, nbits, B_bitsliced, A_bitsliced);
    unpack_bitslice(nshares, nbits, B, B_bitsliced);

#ifdef DEBUG
    for (size_t i = 0; i < 32; i++)
    {
        uint32_t A_unmasked = 0;
        uint32_t B_unmasked = 0;

        for (size_t j = 0; j < nshares; j++)
        {
            A_unmasked = (A_unmasked + A[i][j]) & bit_mask(nbits);
            B_unmasked = (B_unmasked ^ B[i][j]) & bit_mask(nbits);
        }

        assert(A_unmasked == B_unmasked);
    }
#endif
}

void A2B_keepbitsliced(size_t nshares, uint32_t ncoefsb, uint32_t compressfrom_b, uint32_t compressto_b, uint32_t ncoefsc, uint32_t compressfrom_c, uint32_t compressto_c, uint32_t out[SIMPLECOMPBITS][NSHARES], const uint32_t Bp[ncoefsb][NSHARES], const uint32_t Cp[ncoefsc][NSHARES])
{
    uint32_t B1_bitsliced[nshares][compressfrom_b];
    uint32_t B2_bitsliced[nshares][compressfrom_b];
    uint32_t C1_bitsliced[nshares][compressfrom_c];
    uint32_t C2_bitsliced[nshares][compressfrom_c];

    // convert B
    for (size_t i = 0; i < ncoefsb; i += 32)
    {
        // pack to bitslice, then A2B
        // don't unpack
        pack_bitslice(nshares, compressfrom_b, B1_bitsliced, &Bp[i]);
        A2B_bitsliced_inner(nshares, compressfrom_b, B2_bitsliced, B1_bitsliced);
        for (size_t j = 0; j < nshares; j++)
        {
            for (size_t k = 0; k < compressto_b; k++)
            {
                out[i / 32 * compressto_b + k][j] = B2_bitsliced[j][k + compressfrom_b - compressto_b];
            }
        }
    }

    // convert C
    for (size_t i = 0; i < ncoefsc; i += 32)
    {
        // pack to bitslice, then A2B
        // don't unpack
        pack_bitslice(nshares, compressfrom_c, C1_bitsliced, &Cp[i]);
        A2B_bitsliced_inner(nshares, compressfrom_c, C2_bitsliced, C1_bitsliced);
        for (size_t j = 0; j < nshares; j++)
        {
            for (size_t k = 0; k < compressto_c; k++)
            {
                out[ncoefsb / 32 * compressto_b + i / 32 * compressto_c + k][j] = C2_bitsliced[j][k + compressfrom_c - compressto_c];
            }
        }
    }
}
