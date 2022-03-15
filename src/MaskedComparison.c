/*
 * MIT License
 *
 * Copyright (c) 2021-2022: imec-COSIC KU Leuven, 3001 Leuven, Belgium 
 * Author:  Michiel Van Beirendonck <michiel.vanbeirendonck@esat.kuleuven.be>
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

#include "MaskedComparison.h"
#include "ReduceComparisons.h"
#include "BooleanEqualityTest.h"
#include "randombytes.h"
#include "bitmask.h"
#include "A2B.h"
#include "B2A.h"
#include "SecAnd.h"
#include "SecMult.h"
#include "hal.h"
#include <string.h>

#ifdef KYBER
static void shared_compress(size_t ncoeffs, size_t compressto, uint32_t B[ncoeffs][NSHARES])
{
    for (size_t i = 0; i < ncoeffs; i++)
    {
    #ifdef DEBUG

        uint32_t B_unmasked = 0, BA_unmasked = 0;

        for (size_t j = 0; j < NSHARES; j++)
        {
            B_unmasked = (B_unmasked + B[i][j]) % Q;
        }

        B_unmasked = (((B_unmasked << compressto) + Q/2) / Q) & bit_mask(compressto);

    #endif

        for (size_t j = 0; j < NSHARES; j++)
        {
            uint64_t tmp = (uint64_t)B[i][j] << (compressto + KYBER_FRAC_BITS);

            if (j == 0)
            {
                tmp += (Q << KYBER_FRAC_BITS)/2;
            }

            B[i][j] = (tmp / Q); // ! make sure (uint64_t/constant) is constant-time on your platform
        }

    #ifdef DEBUG

        for (size_t j = 0; j < NSHARES; j++)
        {
            BA_unmasked =  (BA_unmasked + B[i][j]) & bit_mask(compressto + KYBER_FRAC_BITS);
        }

        BA_unmasked >>= KYBER_FRAC_BITS;

        assert(B_unmasked== BA_unmasked);

    #endif
    }
}
#endif

uint64_t MaskedComparison_Arith(const uint32_t B[NCOEFFS_B][NSHARES], const uint32_t C[NCOEFFS_C][NSHARES],
                          const uint32_t public_B[NCOEFFS_B], const uint32_t public_C[NCOEFFS_C])
{
    uint32_t B_compressed[NCOEFFS_B][NSHARES];
    uint32_t C_compressed[NCOEFFS_C][NSHARES];
    uint64_t BC_reshared[NCOEFFS_B + NCOEFFS_C][NSHARES];
    uint64_t E[NSHARES];
    uint32_t Bp[NCOEFFS_B][NSHARES], Cp[NCOEFFS_C][NSHARES];

    PROFILE_STEP_INIT();

    ////////////////////////////////////////////////////////////
    ///                Step 0 : Preprocessing                ///
    ////////////////////////////////////////////////////////////

    PROFILE_STEP_START();

    memcpy(Bp, B, NCOEFFS_B * NSHARES * sizeof(uint32_t));
    memcpy(Cp, C, NCOEFFS_C * NSHARES * sizeof(uint32_t));

    #if defined(KYBER)
        shared_compress(NCOEFFS_B, COMPRESSTO_B, Bp);
        shared_compress(NCOEFFS_C, COMPRESSTO_C, Cp);
    #endif

    for (size_t i = 0; i < NCOEFFS_B; i++)
    {
        Bp[i][0] = (Bp[i][0] - (public_B[i] << (COMPRESSFROM_B - COMPRESSTO_B))) & bit_mask(COMPRESSFROM_B);
    }

    for (size_t i = 0; i < NCOEFFS_C; i++)
    {
        Cp[i][0] = (Cp[i][0] - (public_C[i] << (COMPRESSFROM_C - COMPRESSTO_C))) & bit_mask(COMPRESSFROM_C);
    }

    PROFILE_STEP_STOP(0);

    ////////////////////////////////////////////////////////////
    ///                    Step 1 : A2B                      ///
    ////////////////////////////////////////////////////////////

    PROFILE_STEP_START();

    for (size_t i = 0; i < NCOEFFS_B; i += 32)
    {
        A2B_bitsliced(NSHARES, COMPRESSFROM_B, B_compressed + i, Bp + i);
    }

    for (size_t i = 0; i < NCOEFFS_B; i++)
    {
        for (size_t j = 0; j < NSHARES; j++)
        {
            B_compressed[i][j] = (B_compressed[i][j] >> (COMPRESSFROM_B - COMPRESSTO_B)) & bit_mask(COMPRESSTO_B);
        }
    }

    for (size_t i = 0; i < NCOEFFS_C; i += 32)
    {
        A2B_bitsliced(NSHARES, COMPRESSFROM_C, C_compressed + i, Cp + i);
    }

    for (size_t i = 0; i < NCOEFFS_C; i++)
    {
        for (size_t j = 0; j < NSHARES; j++)
        {
            C_compressed[i][j] = (C_compressed[i][j] >> (COMPRESSFROM_C - COMPRESSTO_C)) & bit_mask(COMPRESSTO_C);
        }
    }

    PROFILE_STEP_STOP(1);

    ////////////////////////////////////////////////////////////
    ///                    Step 2 : B2A                      ///
    ////////////////////////////////////////////////////////////

    PROFILE_STEP_START();

    for (size_t i = 0; i < NCOEFFS_B; i++)
    {
        B2A(BC_reshared[i], B_compressed[i]);
    }

    for (size_t i = 0; i < NCOEFFS_C; i++)
    {
        B2A(BC_reshared[NCOEFFS_B + i], C_compressed[i]);
    }

    PROFILE_STEP_STOP(2);

    ////////////////////////////////////////////////////////////
    ///              Step 3 : ReduceComparisons              ///
    ////////////////////////////////////////////////////////////

    PROFILE_STEP_START();

    ReduceComparisons(E, BC_reshared);

    PROFILE_STEP_STOP(3);

    ////////////////////////////////////////////////////////////
    ///            Step 4 : BooleanEqualityTest              ///
    ////////////////////////////////////////////////////////////

    PROFILE_STEP_START();

    uint64_t result = BooleanEqualityTest(E);

    PROFILE_STEP_STOP(4);

    return result;
}

uint64_t MaskedComparison_Simple(const uint32_t B[NCOEFFS_B][NSHARES], const uint32_t C[NCOEFFS_C][NSHARES],
                          const uint32_t public_B[NCOEFFS_B], const uint32_t public_C[NCOEFFS_C])
{
    uint32_t Bp[NCOEFFS_B][NSHARES], Cp[NCOEFFS_C][NSHARES];

    PROFILE_STEP_INIT();

    ////////////////////////////////////////////////////////////
    ///                Step 0 : Preprocessing                ///
    ////////////////////////////////////////////////////////////

    PROFILE_STEP_START();

    memcpy(Bp, B, NCOEFFS_B * NSHARES * sizeof(uint32_t));
    memcpy(Cp, C, NCOEFFS_C * NSHARES * sizeof(uint32_t));

    #if defined(KYBER)
        shared_compress(NCOEFFS_B, COMPRESSTO_B, Bp);
        shared_compress(NCOEFFS_C, COMPRESSTO_C, Cp);
    #endif

    for (size_t i = 0; i < NCOEFFS_B; i++)
    {
        Bp[i][0] = (Bp[i][0] - (public_B[i] << (COMPRESSFROM_B - COMPRESSTO_B))) & bit_mask(COMPRESSFROM_B);
    }

    for (size_t i = 0; i < NCOEFFS_C; i++)
    {
        Cp[i][0] = (Cp[i][0] - (public_C[i] << (COMPRESSFROM_C - COMPRESSTO_C))) & bit_mask(COMPRESSFROM_C);
    }

    PROFILE_STEP_STOP(0);

    ////////////////////////////////////////////////////////////
    ///                    Step 1 : A2B                      ///
    ////////////////////////////////////////////////////////////

    uint32_t BC_Bitsliced[ SIMPLECOMPBITS ][NSHARES];

    PROFILE_STEP_START();

    A2B_keepbitsliced(NSHARES, NCOEFFS_B, COMPRESSFROM_B, COMPRESSTO_B, NCOEFFS_C, COMPRESSFROM_C, COMPRESSTO_C, BC_Bitsliced, Bp, Cp);

    PROFILE_STEP_STOP(1);

    ////////////////////////////////////////////////////////////
    ///            Step 4 : BooleanEqualityTest              ///
    ////////////////////////////////////////////////////////////

    PROFILE_STEP_START();

    uint64_t result = BooleanEqualityTest_Simple(BC_Bitsliced, SIMPLECOMPBITS);

    PROFILE_STEP_STOP(4);

    return result;
}

uint64_t MaskedComparison_Simple_NBS(const uint32_t B[NCOEFFS_B][NSHARES], const uint32_t C[NCOEFFS_C][NSHARES],
                          const uint32_t public_B[NCOEFFS_B], const uint32_t public_C[NCOEFFS_C])
{
    uint32_t Bp[NCOEFFS_B][NSHARES], Cp[NCOEFFS_C][NSHARES];

    PROFILE_STEP_INIT();

    ////////////////////////////////////////////////////////////
    ///                Step 0 : Preprocessing                ///
    ////////////////////////////////////////////////////////////

    PROFILE_STEP_START();

    memcpy(Bp, B, NCOEFFS_B * NSHARES * sizeof(uint32_t));
    memcpy(Cp, C, NCOEFFS_C * NSHARES * sizeof(uint32_t));

    #if defined(KYBER)
        shared_compress(NCOEFFS_B, COMPRESSTO_B, Bp);
        shared_compress(NCOEFFS_C, COMPRESSTO_C, Cp);
    #endif

    for (size_t i = 0; i < NCOEFFS_B; i++)
    {
        Bp[i][0] = (Bp[i][0] - (public_B[i] << (COMPRESSFROM_B - COMPRESSTO_B))) & bit_mask(COMPRESSFROM_B);
    }

    for (size_t i = 0; i < NCOEFFS_C; i++)
    {
        Cp[i][0] = (Cp[i][0] - (public_C[i] << (COMPRESSFROM_C - COMPRESSTO_C))) & bit_mask(COMPRESSFROM_C);
    }

    PROFILE_STEP_STOP(0);

    ////////////////////////////////////////////////////////////
    ///                    Step 1 : A2B                      ///
    ////////////////////////////////////////////////////////////

    uint32_t BC[NCOEFFS_B + NCOEFFS_C][NSHARES];

    PROFILE_STEP_START();

    for (size_t i = 0; i < NCOEFFS_B; i++)
    {
        A2B32(NSHARES, BC[i], Bp[i]);
        for (size_t j = 0; j < NSHARES; j++)
        {
            BC[i][j] = (BC[i][j] >> (COMPRESSFROM_B - COMPRESSTO_B)) & bit_mask(COMPRESSTO_B);
        }
    }

    for (size_t i = 0; i < NCOEFFS_C; i++)
    {
        A2B32(NSHARES, BC[NCOEFFS_B + i], Cp[i]);
        for (size_t j = 0; j < NSHARES; j++)
        {
            BC[NCOEFFS_B + i][j] = (BC[NCOEFFS_B + i][j] >> (COMPRESSFROM_C - COMPRESSTO_C)) & bit_mask(COMPRESSTO_C);
        }
    }

    PROFILE_STEP_STOP(1);

    ////////////////////////////////////////////////////////////
    ///            Step 4 : BooleanEqualityTest              ///
    ////////////////////////////////////////////////////////////

    PROFILE_STEP_START();

    uint64_t result = BooleanEqualityTest_Simple_NBS(BC, NCOEFFS_B + NCOEFFS_C);

    PROFILE_STEP_STOP(4);

    return result;
}

uint64_t MaskedComparison_Simple_NBSO(const uint32_t B[NCOEFFS_B][NSHARES], const uint32_t C[NCOEFFS_C][NSHARES],
                          const uint32_t public_B[NCOEFFS_B], const uint32_t public_C[NCOEFFS_C])
{
    uint32_t Bp[NCOEFFS_B][NSHARES], Cp[NCOEFFS_C][NSHARES];

    PROFILE_STEP_INIT();

    ////////////////////////////////////////////////////////////
    ///                Step 0 : Preprocessing                ///
    ////////////////////////////////////////////////////////////

    PROFILE_STEP_START();

    memcpy(Bp, B, NCOEFFS_B * NSHARES * sizeof(uint32_t));
    memcpy(Cp, C, NCOEFFS_C * NSHARES * sizeof(uint32_t));

    #if defined(KYBER)
        shared_compress(NCOEFFS_B, COMPRESSTO_B, Bp);
        shared_compress(NCOEFFS_C, COMPRESSTO_C, Cp);
    #endif

    for (size_t i = 0; i < NCOEFFS_B; i++)
    {
        Bp[i][0] = (Bp[i][0] - (public_B[i] << (COMPRESSFROM_B - COMPRESSTO_B))) & bit_mask(COMPRESSFROM_B);
    }

    for (size_t i = 0; i < NCOEFFS_C; i++)
    {
        Cp[i][0] = (Cp[i][0] - (public_C[i] << (COMPRESSFROM_C - COMPRESSTO_C))) & bit_mask(COMPRESSFROM_C);
    }

    PROFILE_STEP_STOP(0);

    ////////////////////////////////////////////////////////////
    ///                    Step 1 : A2B                      ///
    ////////////////////////////////////////////////////////////

    uint32_t BC[NCOEFFS_B + NCOEFFS_C][NSHARES];

    PROFILE_STEP_START();

    for (size_t i = 0; i < NCOEFFS_B; i += 32)
    {
        A2B_bitsliced(NSHARES, COMPRESSFROM_B, BC + i, Bp + i);
    }

    for (size_t i = 0; i < NCOEFFS_B; i++)
    {
        for (size_t j = 0; j < NSHARES; j++)
        {
            BC[i][j] = (BC[i][j] >> (COMPRESSFROM_B - COMPRESSTO_B)) & bit_mask(COMPRESSTO_B);
        }
    }

    for (size_t i = 0; i < NCOEFFS_C; i += 32)
    {
        A2B_bitsliced(NSHARES, COMPRESSFROM_C, BC + (NCOEFFS_B + i), Cp + i);
    }

    for (size_t i = 0; i < NCOEFFS_C; i++)
    {
        for (size_t j = 0; j < NSHARES; j++)
        {
            BC[NCOEFFS_B + i][j] = (BC[NCOEFFS_B + i][j] >> (COMPRESSFROM_C - COMPRESSTO_C)) & bit_mask(COMPRESSTO_C);
        }
    }

    PROFILE_STEP_STOP(1);


    ////////////////////////////////////////////////////////////
    ///            Step 4 : BooleanEqualityTest              ///
    ////////////////////////////////////////////////////////////

    PROFILE_STEP_START();

    uint64_t result = BooleanEqualityTest_Simple(BC, NCOEFFS_B + NCOEFFS_C);

    PROFILE_STEP_STOP(4);

    return result;
}

uint64_t MaskedComparison_GF(const uint32_t B[NCOEFFS_B][NSHARES], const uint32_t C[NCOEFFS_C][NSHARES],
                          const uint32_t public_B[NCOEFFS_B], const uint32_t public_C[NCOEFFS_C])
{
    struct uint96_t E;
    uint32_t Bp[NCOEFFS_B][NSHARES], Cp[NCOEFFS_C][NSHARES];

    PROFILE_STEP_INIT();

    ////////////////////////////////////////////////////////////
    ///                Step 0 : Preprocessing                ///
    ////////////////////////////////////////////////////////////

    PROFILE_STEP_START();

    memcpy(Bp, B, NCOEFFS_B * NSHARES * sizeof(uint32_t));
    memcpy(Cp, C, NCOEFFS_C * NSHARES * sizeof(uint32_t));

    #if defined(KYBER)
        shared_compress(NCOEFFS_B, COMPRESSTO_B, Bp);
        shared_compress(NCOEFFS_C, COMPRESSTO_C, Cp);
    #endif

    for (size_t i = 0; i < NCOEFFS_B; i++)
    {
        Bp[i][0] = (Bp[i][0] - (public_B[i] << (COMPRESSFROM_B - COMPRESSTO_B))) & bit_mask(COMPRESSFROM_B);
    }

    for (size_t i = 0; i < NCOEFFS_C; i++)
    {
        Cp[i][0] = (Cp[i][0] - (public_C[i] << (COMPRESSFROM_C - COMPRESSTO_C))) & bit_mask(COMPRESSFROM_C);
    }

    PROFILE_STEP_STOP(0);

    ////////////////////////////////////////////////////////////
    ///                    Step 1 : A2B                      ///
    ////////////////////////////////////////////////////////////

    uint32_t BC_Bitsliced[ SIMPLECOMPBITS ][NSHARES];

    PROFILE_STEP_START();

    A2B_keepbitsliced(NSHARES, NCOEFFS_B, COMPRESSFROM_B, COMPRESSTO_B, NCOEFFS_C, COMPRESSFROM_C, COMPRESSTO_C, BC_Bitsliced, Bp, Cp);

    PROFILE_STEP_STOP(1);

    ////////////////////////////////////////////////////////////
    ///              Step 3 : ReduceComparisons              ///
    ////////////////////////////////////////////////////////////

    PROFILE_STEP_START();

    ReduceComparisons_GF(&E, BC_Bitsliced);

    PROFILE_STEP_STOP(3);

    ////////////////////////////////////////////////////////////
    ///            Step 4 : BooleanEqualityTest              ///
    ////////////////////////////////////////////////////////////

    PROFILE_STEP_START();

    uint64_t result = BooleanEqualityTest_GF(E);

    PROFILE_STEP_STOP(4);

    return result;
}

#ifdef KYBER
uint64_t MaskedComparison_HybridSimple(const uint32_t B[NCOEFFS_B][NSHARES], const uint32_t C[NCOEFFS_C][NSHARES],
                          const uint32_t public_B[NCOEFFS_B], const uint32_t public_C[NCOEFFS_C])
{
    uint32_t Bp[NCOEFFS_B][NSHARES], Cp[NCOEFFS_C][NSHARES];

    PROFILE_STEP_INIT();

    ////////////////////////////////////////////////////////////
    ///                Step 0 : Preprocessing                ///
    ////////////////////////////////////////////////////////////

    PROFILE_STEP_START();

    memcpy(Bp, B, NCOEFFS_B * NSHARES * sizeof(uint32_t));
    memcpy(Cp, C, NCOEFFS_C * NSHARES * sizeof(uint32_t));

    shared_compress(NCOEFFS_C, COMPRESSTO_C, Cp);

    for (size_t i = 0; i < NCOEFFS_C; i++)
    {
        Cp[i][0] = (Cp[i][0] - (public_C[i] << (COMPRESSFROM_C - COMPRESSTO_C))) & bit_mask(COMPRESSFROM_C);
    }

    // PROFILE_STEP_STOP(0);

    ////////////////////////////////////////////////////////////
    ///                    Step 1 : Compress B               ///
    ////////////////////////////////////////////////////////////

    // PROFILE_STEP_START();

    uint32_t E[32][NSHARES];
    memset(E, 0, 32 * NSHARES * sizeof(uint32_t));

    for (int i = 0; i < NCOEFFS_B; i++)
    {
        // decompress
        uint32_t uncL, uncH;
        uncL = uncompress(public_B[i], COMPRESSTO_B, Q);
        uncH = uncompress(public_B[i]+1, COMPRESSTO_B, Q);
        if(uncH<uncL)
        {
            uncH += Q;
        }
        
        // perform for each uncompressed value
        uint32_t tmp_in[NSHARES], tmp_out[NSHARES];
        uint32_t *swap, *p_in, *p_out;
        p_in = tmp_in;
        p_out = tmp_out;

        Bp[i][0] = (Bp[i][0] - uncL + Q) % Q;

        memcpy(tmp_in, Bp[i], NSHARES * sizeof(uint32_t));
        for (uint32_t unc=uncL+1; unc < uncH; unc++)
        {
            Bp[i][0] = (Bp[i][0] - 1 + Q) % Q;
            secMult(NSHARES, Q, p_out, p_in, Bp[i]);

            swap = p_in; p_in = p_out; p_out = swap;
        }
        memcpy(Bp[i], p_in, NSHARES * sizeof(uint32_t));

        // compression
        uint32_t R[2];
        for (int j = 0; j < LB; j+=2)
        {
            randomq(R, Q);
            for (int k = 0; k < NSHARES; k++)
            {
                E[j][k] += R[0] * Bp[i][k];
                E[j + 1][k] += R[1] * Bp[i][k];
            }
        }
    }

    for (int j = 0; j < LB; j++)
    {
        for (int k = 0; k < NSHARES; k++)
        {
            uint64_t tmp = (uint64_t)E[j][k] << COMPRESSFROM_B_HYBRID;
            E[j][k] = (tmp / Q);
        }
        E[j][0] += (1 << KYBER_FRAC_BITS) - 1;
    }

    PROFILE_STEP_STOP(1);

    ////////////////////////////////////////////////////////////
    ///                    Step 2 : A2B                      ///
    ////////////////////////////////////////////////////////////

    uint32_t BC_Bitsliced[ SIMPLECOMPBITS_HYBRID ][NSHARES];
    
    PROFILE_STEP_START();

    A2B_keepbitsliced(NSHARES, 32, COMPRESSFROM_B_HYBRID, COMPRESSTO_B_HYBRID, NCOEFFS_C, COMPRESSFROM_C, COMPRESSTO_C, BC_Bitsliced, E, Cp);

    PROFILE_STEP_STOP(2);

    ////////////////////////////////////////////////////////////
    ///            Step 4 : BooleanEqualityTest              ///
    ////////////////////////////////////////////////////////////

    PROFILE_STEP_START();

    uint64_t result = BooleanEqualityTest_Simple(BC_Bitsliced, SIMPLECOMPBITS_HYBRID);

    PROFILE_STEP_STOP(4);

    return result;
}
#endif