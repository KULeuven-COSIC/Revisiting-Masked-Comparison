#ifndef PARAMS_H
#define PARAMS_H

// #define L 2 
#define L 3 
// #define L 4 

#define N 256
#define NCOEFFS_B (L * N)
#define NCOEFFS_C N

#if defined(SABER)

    #define Q 8192
    #define P 1024

    #if L == 2
        #define SABER_ET 3
    #elif L == 3
        #define SABER_ET 4
    #elif L == 4
        #define SABER_ET 6
    #endif

    #define SABER_EQ 13
    #define SABER_EP 10

    #define COMPRESSFROM_B SABER_EQ
    #define COMPRESSFROM_C SABER_EP

    #define COMPRESSTO_B SABER_EP
    #define COMPRESSTO_C SABER_ET

    // needed for compilation, not actually used
    #define KYBER_DU 10
    #define KYBER_DV 10
    #define COMPRESSTO_B_HYBRID 13
    #define COMPRESSFROM_B_HYBRID 13
    #define LB 12 // needs to be even
    #define SIMPLECOMPBITS_HYBRID 13
    #define KYBER_FRAC_BITS 0

#elif defined(KYBER)

    #define Q 3329
    #define P 3329

    #if NSHARES == 2
        #define KYBER_FRAC_BITS 13
    #elif NSHARES == 3
        #define KYBER_FRAC_BITS 14
    #elif NSHARES == 4
        #define KYBER_FRAC_BITS 15
    #elif NSHARES == 5
        #define KYBER_FRAC_BITS 16
    #elif NSHARES == 6
        #define KYBER_FRAC_BITS 17
    #endif

    #if L == 2 
        #define KYBER_DU 10
        #define KYBER_DV 4
    #elif L == 3
        #define KYBER_DU 10
        #define KYBER_DV 4
    #elif L == 4
        #define KYBER_DU 11
        #define KYBER_DV 5
    #endif

    #define COMPRESSFROM_B (KYBER_DU + KYBER_FRAC_BITS)
    #define COMPRESSFROM_C (KYBER_DV + KYBER_FRAC_BITS)

    #define COMPRESSTO_B KYBER_DU
    #define COMPRESSTO_C KYBER_DV

    #define COMPRESSTO_B_HYBRID 13
    #define COMPRESSFROM_B_HYBRID (COMPRESSTO_B_HYBRID + KYBER_FRAC_BITS)
    #define LB 12 // needs to be even
    #define SIMPLECOMPBITS_HYBRID COMPRESSTO_B_HYBRID + NCOEFFS_C / 32 * COMPRESSTO_C
#else
#error
#endif

#define SIMPLECOMPBITS NCOEFFS_B / 32 * COMPRESSTO_B + NCOEFFS_C / 32 * COMPRESSTO_C



#endif