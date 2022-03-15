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

#include "BooleanEqualityTest.h"
#include "SecAnd.h"
#include "A2B.h"

uint32_t BooleanEqualityTest(uint64_t E[NSHARES])
{
    uint64_t B[NSHARES];
    uint32_t Bh[NSHARES], Bl[NSHARES];
    uint32_t out[NSHARES];
    uint32_t out_unmasked = 0;

    A2B(NSHARES, B, E);

    // Boolean equality circuit: B = 0 -> ~B[63] AND ~B[62] AND ... ~B[0];

    // ~B
    B[0] ^= 0xffffffffffffffff;

    for (size_t i = 0; i < NSHARES; i++)
    {
        Bl[i] = B[i];
        Bh[i] = B[i] >> 32;
    }

    SecAND32(NSHARES, out, Bh, Bl);

    for(size_t j = 16; j > 0; j >>= 1)
    {
        for (size_t i = 0; i < NSHARES; i++)
        {
            Bl[i] = (out[i]) & ((1 << j) - 1);
            Bh[i] = (out[i] >> j) & ((1 << j) - 1);
        }

        SecAND32(NSHARES, out, Bh, Bl);
    }

    for (size_t i = 0; i < NSHARES; i++)
    {
        out_unmasked ^= out[i];
    }

    return out_unmasked;
}

uint32_t BooleanEqualityTest_GF(struct uint96_t B)
{
    // uint64_t B[NSHARES];
    uint32_t Bh[NSHARES], Bl[NSHARES];
    uint32_t tmp[NSHARES];
    uint32_t out[NSHARES];
    uint32_t out_unmasked = 0;

    // A2B(NSHARES, B, E);

    // Boolean equality circuit: B = 0 -> ~B[63] AND ~B[62] AND ... ~B[0];

    // ~B
    B.LSB[0] ^= 0xffffffffffffffff;
    B.MSB[0] ^= 0xffffffff;

    for (size_t i = 0; i < NSHARES; i++)
    {
        Bl[i] = B.LSB[i];
        Bh[i] = B.LSB[i] >> 32;
    }

    SecAND32(NSHARES, tmp, Bh, Bl);
    SecAND32(NSHARES, out, tmp, B.MSB);


    for(size_t j = 16; j > 0; j >>= 1)
    {
        for (size_t i = 0; i < NSHARES; i++)
        {
            Bl[i] = (out[i]) & ((1 << j) - 1);
            Bh[i] = (out[i] >> j) & ((1 << j) - 1);
        }

        SecAND32(NSHARES, out, Bh, Bl);
    }

    for (size_t i = 0; i < NSHARES; i++)
    {
        out_unmasked ^= out[i];
    }

    return out_unmasked;
}

uint32_t BooleanEqualityTest_Simple(uint32_t B[SIMPLECOMPBITS][NSHARES], uint32_t len)
{
    // uint64_t B[NSHARES];
    uint32_t Bh[NSHARES], Bl[NSHARES];
    uint32_t out[NSHARES];
    uint32_t out_unmasked = 0;

    // ~B
    for (size_t i = 0; i < len; i++)
    {
        B[i][0] ^= 0xffffffffffffffff;
    }

    // AND all different registers for 0 to len
    SecAND32(NSHARES, out, B[0], B[1]);

    for (size_t i = 2; i < len; i++)
    {
        for (size_t j = 0; j < NSHARES; j++)
        {
            Bh[j] = out[j];
        }
        SecAND32(NSHARES, out, Bh, B[i]);
    }
    
    // do within register AND's
    for(size_t j = 16; j > 0; j >>= 1)
    {
        for (size_t i = 0; i < NSHARES; i++)
        {
            Bl[i] = (out[i]) & ((1 << j) - 1);
            Bh[i] = (out[i] >> j) & ((1 << j) - 1);
        }

        SecAND32(NSHARES, out, Bh, Bl);
    }

    for (size_t i = 0; i <  NSHARES; i++)
    {
        out_unmasked ^= out[i];
    }

    return out_unmasked;
}

uint32_t BooleanEqualityTest_Simple_NBS(uint32_t B[][NSHARES], uint32_t len)
{
    // uint64_t B[NSHARES];
    uint32_t tmp[NSHARES];
    uint32_t out[NSHARES];
    uint32_t out_unmasked = 0;

    // ~B
    for (size_t i = 0; i < len; i++)
    {
        B[i][0] ^= 0xffffffffffffffff;
    }

    out[0] = 1;
    for (size_t j = 1; j < NSHARES; j++)
    {
        out[j] = 0;
    }

    for (size_t i = 0; i < NCOEFFS_B; i++)
    {
        for (size_t j = 0; j < COMPRESSTO_B; j++)
        {
            for (size_t k = 0; k < NSHARES; k++)
            {
                tmp[k] = B[i][k] >> j;
            }
            SecAND32(NSHARES, out, out, tmp);
        }
    }

    for (size_t i = NCOEFFS_B; i < NCOEFFS_B + NCOEFFS_C; i++)
    {
        for (size_t j = 0; j < COMPRESSTO_C; j++)
        {
            for (size_t k = 0; k < NSHARES; k++)
            {
                tmp[k] = B[i][k] >> j;
            }
            SecAND32(NSHARES, out, out, tmp);
        }
    }

    for (size_t i = 0; i <  NSHARES; i++)
    {
        out_unmasked ^= out[i];
    }

    out_unmasked = out_unmasked & 1;

    return out_unmasked;
}