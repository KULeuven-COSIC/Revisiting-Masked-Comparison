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

#include "ReduceComparisons.h"
#include "randombytes.h"
#include "params.h"

void ReduceComparisons(uint64_t E[NSHARES], const uint64_t D[NCOEFFS_B + NCOEFFS_C][NSHARES])
{
    for (size_t j = 0; j < NSHARES; j++)
    {
        E[j] = 0;
    }

    for (size_t i = 0; i < (NCOEFFS_B + NCOEFFS_C); i++)
    {
        uint64_t R = random_uint64();

#ifdef DEBUG
        uint64_t D_unmasked = 0;
#endif

        for (size_t j = 0; j < NSHARES; j++)
        {
            E[j] += R * D[i][j];

#ifdef DEBUG
            D_unmasked += D[i][j];
#endif
        }
    }
}

void ReduceComparisons_GF(struct uint96_t *E, uint32_t BC_Bitsliced[SIMPLECOMPBITS][NSHARES])
{
    uint32_t biti;
    uint64_t tmp;

    for (size_t i = 0; i < NSHARES; i++)
    {
        for (size_t j = 0; j < NSHARES; j++)
        {
            E->MSB[i] = 0;
            E->LSB[i] = 0;
        }
    }

    for (size_t i = 0; i < SIMPLECOMPBITS; i++)
    {
        uint64_t R = random_uint64();

        for (size_t j = 0; j < NSHARES; j++)
        {
            for (size_t k = 0; k < 32; k++)
            {
                biti = (BC_Bitsliced[i][j] >> k) & 1;
                tmp = R * biti;
                E->LSB[j] ^= tmp << k;
                E->MSB[j] ^= tmp >> (64 - k);
            }
        }
    }
}