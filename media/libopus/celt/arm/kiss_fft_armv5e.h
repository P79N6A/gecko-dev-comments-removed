
























#ifndef KISS_FFT_ARMv5E_H
#define KISS_FFT_ARMv5E_H

#if !defined(KISS_FFT_GUTS_H)
#error "This file should only be included from _kiss_fft_guts.h"
#endif

#ifdef FIXED_POINT

#if defined(__thumb__)||defined(__thumb2__)
#define LDRD_CONS "Q"
#else
#define LDRD_CONS "Uq"
#endif

#undef C_MUL
#define C_MUL(m,a,b) \
    do{ \
        int mr1__; \
        int mr2__; \
        int mi__; \
        long long aval__; \
        int bval__; \
        __asm__( \
            "#C_MUL\n\t" \
            "ldrd %[aval], %H[aval], %[ap]\n\t" \
            "ldr %[bval], %[bp]\n\t" \
            "smulwb %[mi], %H[aval], %[bval]\n\t" \
            "smulwb %[mr1], %[aval], %[bval]\n\t" \
            "smulwt %[mr2], %H[aval], %[bval]\n\t" \
            "smlawt %[mi], %[aval], %[bval], %[mi]\n\t" \
            : [mr1]"=r"(mr1__), [mr2]"=r"(mr2__), [mi]"=r"(mi__), \
              [aval]"=&r"(aval__), [bval]"=r"(bval__) \
            : [ap]LDRD_CONS(a), [bp]"m"(b) \
        ); \
        (m).r = SHL32(SUB32(mr1__, mr2__), 1); \
        (m).i = SHL32(mi__, 1); \
    } \
    while(0)

#undef C_MUL4
#define C_MUL4(m,a,b) \
    do{ \
        int mr1__; \
        int mr2__; \
        int mi__; \
        long long aval__; \
        int bval__; \
        __asm__( \
            "#C_MUL4\n\t" \
            "ldrd %[aval], %H[aval], %[ap]\n\t" \
            "ldr %[bval], %[bp]\n\t" \
            "smulwb %[mi], %H[aval], %[bval]\n\t" \
            "smulwb %[mr1], %[aval], %[bval]\n\t" \
            "smulwt %[mr2], %H[aval], %[bval]\n\t" \
            "smlawt %[mi], %[aval], %[bval], %[mi]\n\t" \
            : [mr1]"=r"(mr1__), [mr2]"=r"(mr2__), [mi]"=r"(mi__), \
              [aval]"=&r"(aval__), [bval]"=r"(bval__) \
            : [ap]LDRD_CONS(a), [bp]"m"(b) \
        ); \
        (m).r = SHR32(SUB32(mr1__, mr2__), 1); \
        (m).i = SHR32(mi__, 1); \
    } \
    while(0)

#undef C_MULC
#define C_MULC(m,a,b) \
    do{ \
        int mr__; \
        int mi1__; \
        int mi2__; \
        long long aval__; \
        int bval__; \
        __asm__( \
            "#C_MULC\n\t" \
            "ldrd %[aval], %H[aval], %[ap]\n\t" \
            "ldr %[bval], %[bp]\n\t" \
            "smulwb %[mr], %[aval], %[bval]\n\t" \
            "smulwb %[mi1], %H[aval], %[bval]\n\t" \
            "smulwt %[mi2], %[aval], %[bval]\n\t" \
            "smlawt %[mr], %H[aval], %[bval], %[mr]\n\t" \
            : [mr]"=r"(mr__), [mi1]"=r"(mi1__), [mi2]"=r"(mi2__), \
              [aval]"=&r"(aval__), [bval]"=r"(bval__) \
            : [ap]LDRD_CONS(a), [bp]"m"(b) \
        ); \
        (m).r = SHL32(mr__, 1); \
        (m).i = SHL32(SUB32(mi1__, mi2__), 1); \
    } \
    while(0)

#endif 

#endif 
