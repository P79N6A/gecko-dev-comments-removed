
















#include "webrtc/common_audio/signal_processing/complex_fft_tables.h"
#include "webrtc/common_audio/signal_processing/include/signal_processing_library.h"

#define CFFTSFT 14
#define CFFTRND 1
#define CFFTRND2 16384

#define CIFFTSFT 14
#define CIFFTRND 1


int WebRtcSpl_ComplexFFT(int16_t frfi[], int stages, int mode)
{
    int i, j, l, k, istep, n, m;
    int16_t wr, wi;
    int32_t tr32, ti32, qr32, qi32;

    


    n = 1 << stages;
    if (n > 1024)
        return -1;

    l = 1;
    k = 10 - 1; 


    if (mode == 0)
    {
        
        while (l < n)
        {
            istep = l << 1;

            for (m = 0; m < l; ++m)
            {
                j = m << k;

                



                wr = kSinTable1024[j + 256];
                wi = -kSinTable1024[j];

                for (i = m; i < n; i += istep)
                {
                    j = i + l;

                    tr32 = WEBRTC_SPL_RSHIFT_W32((WEBRTC_SPL_MUL_16_16(wr, frfi[2 * j])
                            - WEBRTC_SPL_MUL_16_16(wi, frfi[2 * j + 1])), 15);

                    ti32 = WEBRTC_SPL_RSHIFT_W32((WEBRTC_SPL_MUL_16_16(wr, frfi[2 * j + 1])
                            + WEBRTC_SPL_MUL_16_16(wi, frfi[2 * j])), 15);

                    qr32 = (int32_t)frfi[2 * i];
                    qi32 = (int32_t)frfi[2 * i + 1];
                    frfi[2 * j] = (int16_t)WEBRTC_SPL_RSHIFT_W32(qr32 - tr32, 1);
                    frfi[2 * j + 1] = (int16_t)WEBRTC_SPL_RSHIFT_W32(qi32 - ti32, 1);
                    frfi[2 * i] = (int16_t)WEBRTC_SPL_RSHIFT_W32(qr32 + tr32, 1);
                    frfi[2 * i + 1] = (int16_t)WEBRTC_SPL_RSHIFT_W32(qi32 + ti32, 1);
                }
            }

            --k;
            l = istep;

        }

    } else
    {
        
        while (l < n)
        {
            istep = l << 1;

            for (m = 0; m < l; ++m)
            {
                j = m << k;

                



                wr = kSinTable1024[j + 256];
                wi = -kSinTable1024[j];

#ifdef WEBRTC_ARCH_ARM_V7
                int32_t wri = 0;
                int32_t frfi_r = 0;
                __asm __volatile("pkhbt %0, %1, %2, lsl #16" : "=r"(wri) :
                    "r"((int32_t)wr), "r"((int32_t)wi));
#endif

                for (i = m; i < n; i += istep)
                {
                    j = i + l;

#ifdef WEBRTC_ARCH_ARM_V7
                    __asm __volatile(
                      "pkhbt %[frfi_r], %[frfi_even], %[frfi_odd], lsl #16\n\t"
                      "smlsd %[tr32], %[wri], %[frfi_r], %[cfftrnd]\n\t"
                      "smladx %[ti32], %[wri], %[frfi_r], %[cfftrnd]\n\t"
                      :[frfi_r]"+r"(frfi_r),
                       [tr32]"=r"(tr32),
                       [ti32]"=r"(ti32)
                      :[frfi_even]"r"((int32_t)frfi[2*j]),
                       [frfi_odd]"r"((int32_t)frfi[2*j +1]),
                       [wri]"r"(wri),
                       [cfftrnd]"r"(CFFTRND)
                    );
    
#else
                    tr32 = WEBRTC_SPL_MUL_16_16(wr, frfi[2 * j])
                            - WEBRTC_SPL_MUL_16_16(wi, frfi[2 * j + 1]) + CFFTRND;

                    ti32 = WEBRTC_SPL_MUL_16_16(wr, frfi[2 * j + 1])
                            + WEBRTC_SPL_MUL_16_16(wi, frfi[2 * j]) + CFFTRND;
#endif

                    tr32 = WEBRTC_SPL_RSHIFT_W32(tr32, 15 - CFFTSFT);
                    ti32 = WEBRTC_SPL_RSHIFT_W32(ti32, 15 - CFFTSFT);

                    qr32 = ((int32_t)frfi[2 * i]) << CFFTSFT;
                    qi32 = ((int32_t)frfi[2 * i + 1]) << CFFTSFT;

                    frfi[2 * j] = (int16_t)WEBRTC_SPL_RSHIFT_W32(
                            (qr32 - tr32 + CFFTRND2), 1 + CFFTSFT);
                    frfi[2 * j + 1] = (int16_t)WEBRTC_SPL_RSHIFT_W32(
                            (qi32 - ti32 + CFFTRND2), 1 + CFFTSFT);
                    frfi[2 * i] = (int16_t)WEBRTC_SPL_RSHIFT_W32(
                            (qr32 + tr32 + CFFTRND2), 1 + CFFTSFT);
                    frfi[2 * i + 1] = (int16_t)WEBRTC_SPL_RSHIFT_W32(
                            (qi32 + ti32 + CFFTRND2), 1 + CFFTSFT);
                }
            }

            --k;
            l = istep;
        }
    }
    return 0;
}

int WebRtcSpl_ComplexIFFT(int16_t frfi[], int stages, int mode)
{
    int i, j, l, k, istep, n, m, scale, shift;
    int16_t wr, wi;
    int32_t tr32, ti32, qr32, qi32;
    int32_t tmp32, round2;

    


    n = 1 << stages;
    if (n > 1024)
        return -1;

    scale = 0;

    l = 1;
    k = 10 - 1; 


    while (l < n)
    {
        
        shift = 0;
        round2 = 8192;

        tmp32 = (int32_t)WebRtcSpl_MaxAbsValueW16(frfi, 2 * n);
        if (tmp32 > 13573)
        {
            shift++;
            scale++;
            round2 <<= 1;
        }
        if (tmp32 > 27146)
        {
            shift++;
            scale++;
            round2 <<= 1;
        }

        istep = l << 1;

        if (mode == 0)
        {
            
            for (m = 0; m < l; ++m)
            {
                j = m << k;

                



                wr = kSinTable1024[j + 256];
                wi = kSinTable1024[j];

                for (i = m; i < n; i += istep)
                {
                    j = i + l;

                    tr32 = WEBRTC_SPL_RSHIFT_W32((WEBRTC_SPL_MUL_16_16_RSFT(wr, frfi[2 * j], 0)
                            - WEBRTC_SPL_MUL_16_16_RSFT(wi, frfi[2 * j + 1], 0)), 15);

                    ti32 = WEBRTC_SPL_RSHIFT_W32(
                            (WEBRTC_SPL_MUL_16_16_RSFT(wr, frfi[2 * j + 1], 0)
                                    + WEBRTC_SPL_MUL_16_16_RSFT(wi,frfi[2*j],0)), 15);

                    qr32 = (int32_t)frfi[2 * i];
                    qi32 = (int32_t)frfi[2 * i + 1];
                    frfi[2 * j] = (int16_t)WEBRTC_SPL_RSHIFT_W32(qr32 - tr32, shift);
                    frfi[2 * j + 1] = (int16_t)WEBRTC_SPL_RSHIFT_W32(qi32 - ti32, shift);
                    frfi[2 * i] = (int16_t)WEBRTC_SPL_RSHIFT_W32(qr32 + tr32, shift);
                    frfi[2 * i + 1] = (int16_t)WEBRTC_SPL_RSHIFT_W32(qi32 + ti32, shift);
                }
            }
        } else
        {
            

            for (m = 0; m < l; ++m)
            {
                j = m << k;

                



                wr = kSinTable1024[j + 256];
                wi = kSinTable1024[j];

#ifdef WEBRTC_ARCH_ARM_V7
                int32_t wri = 0;
                int32_t frfi_r = 0;
                __asm __volatile("pkhbt %0, %1, %2, lsl #16" : "=r"(wri) :
                    "r"((int32_t)wr), "r"((int32_t)wi));
#endif

                for (i = m; i < n; i += istep)
                {
                    j = i + l;

#ifdef WEBRTC_ARCH_ARM_V7
                    __asm __volatile(
                      "pkhbt %[frfi_r], %[frfi_even], %[frfi_odd], lsl #16\n\t"
                      "smlsd %[tr32], %[wri], %[frfi_r], %[cifftrnd]\n\t"
                      "smladx %[ti32], %[wri], %[frfi_r], %[cifftrnd]\n\t"
                      :[frfi_r]"+r"(frfi_r),
                       [tr32]"=r"(tr32),
                       [ti32]"=r"(ti32)
                      :[frfi_even]"r"((int32_t)frfi[2*j]),
                       [frfi_odd]"r"((int32_t)frfi[2*j +1]),
                       [wri]"r"(wri),
                       [cifftrnd]"r"(CIFFTRND)
                    );
#else

                    tr32 = WEBRTC_SPL_MUL_16_16(wr, frfi[2 * j])
                            - WEBRTC_SPL_MUL_16_16(wi, frfi[2 * j + 1]) + CIFFTRND;

                    ti32 = WEBRTC_SPL_MUL_16_16(wr, frfi[2 * j + 1])
                            + WEBRTC_SPL_MUL_16_16(wi, frfi[2 * j]) + CIFFTRND;
#endif
                    tr32 = WEBRTC_SPL_RSHIFT_W32(tr32, 15 - CIFFTSFT);
                    ti32 = WEBRTC_SPL_RSHIFT_W32(ti32, 15 - CIFFTSFT);

                    qr32 = ((int32_t)frfi[2 * i]) << CIFFTSFT;
                    qi32 = ((int32_t)frfi[2 * i + 1]) << CIFFTSFT;

                    frfi[2 * j] = (int16_t)WEBRTC_SPL_RSHIFT_W32((qr32 - tr32+round2),
                                                                       shift+CIFFTSFT);
                    frfi[2 * j + 1] = (int16_t)WEBRTC_SPL_RSHIFT_W32(
                            (qi32 - ti32 + round2), shift + CIFFTSFT);
                    frfi[2 * i] = (int16_t)WEBRTC_SPL_RSHIFT_W32((qr32 + tr32 + round2),
                                                                       shift + CIFFTSFT);
                    frfi[2 * i + 1] = (int16_t)WEBRTC_SPL_RSHIFT_W32(
                            (qi32 + ti32 + round2), shift + CIFFTSFT);
                }
            }

        }
        --k;
        l = istep;
    }
    return scale;
}
