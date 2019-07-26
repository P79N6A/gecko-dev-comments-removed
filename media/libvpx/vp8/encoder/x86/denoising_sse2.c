









#include "vp8/encoder/denoising.h"
#include "vp8/common/reconinter.h"
#include "vpx/vpx_integer.h"
#include "vpx_mem/vpx_mem.h"
#include "vp8_rtcd.h"

#include <emmintrin.h>
#include "vpx_ports/emmintrin_compat.h"

union sum_union {
    __m128i v;
    signed char e[16];
};

int vp8_denoiser_filter_sse2(YV12_BUFFER_CONFIG *mc_running_avg,
                             YV12_BUFFER_CONFIG *running_avg,
                             MACROBLOCK *signal, unsigned int motion_magnitude,
                             int y_offset, int uv_offset)
{
    unsigned char *sig = signal->thismb;
    int sig_stride = 16;
    unsigned char *mc_running_avg_y = mc_running_avg->y_buffer + y_offset;
    int mc_avg_y_stride = mc_running_avg->y_stride;
    unsigned char *running_avg_y = running_avg->y_buffer + y_offset;
    int avg_y_stride = running_avg->y_stride;
    int r;
    __m128i acc_diff = _mm_setzero_si128();
    const __m128i k_0 = _mm_setzero_si128();
    const __m128i k_4 = _mm_set1_epi8(4);
    const __m128i k_8 = _mm_set1_epi8(8);
    const __m128i k_16 = _mm_set1_epi8(16);
    
    const __m128i l3 = _mm_set1_epi8(
                      (motion_magnitude <= MOTION_MAGNITUDE_THRESHOLD) ? 7 : 6);
    
    const __m128i l32 = _mm_set1_epi8(2);
    
    const __m128i l21 = _mm_set1_epi8(1);

    for (r = 0; r < 16; ++r)
    {
        
        const __m128i v_sig = _mm_loadu_si128((__m128i *)(&sig[0]));
        const __m128i v_mc_running_avg_y = _mm_loadu_si128(
                                           (__m128i *)(&mc_running_avg_y[0]));
        __m128i v_running_avg_y;
        const __m128i pdiff = _mm_subs_epu8(v_mc_running_avg_y, v_sig);
        const __m128i ndiff = _mm_subs_epu8(v_sig, v_mc_running_avg_y);
        
        const __m128i diff_sign = _mm_cmpeq_epi8(pdiff, k_0);
        

        const __m128i clamped_absdiff = _mm_min_epu8(
                                        _mm_or_si128(pdiff, ndiff), k_16);
        
        const __m128i mask2 = _mm_cmpgt_epi8(k_16, clamped_absdiff);
        const __m128i mask1 = _mm_cmpgt_epi8(k_8, clamped_absdiff);
        const __m128i mask0 = _mm_cmpgt_epi8(k_4, clamped_absdiff);
        
        __m128i adj2 = _mm_and_si128(mask2, l32);
        const __m128i adj1 = _mm_and_si128(mask1, l21);
        const __m128i adj0 = _mm_and_si128(mask0, clamped_absdiff);
        __m128i adj,  padj, nadj;

        
        adj2 = _mm_add_epi8(adj2, adj1);
        adj = _mm_sub_epi8(l3, adj2);
        adj = _mm_andnot_si128(mask0, adj);
        adj = _mm_or_si128(adj, adj0);

        
        padj = _mm_andnot_si128(diff_sign, adj);
        nadj = _mm_and_si128(diff_sign, adj);

        
        v_running_avg_y = _mm_adds_epu8(v_sig, padj);
        v_running_avg_y = _mm_subs_epu8(v_running_avg_y, nadj);
        _mm_storeu_si128((__m128i *)running_avg_y, v_running_avg_y);

        


        acc_diff = _mm_adds_epi8(acc_diff, padj);
        acc_diff = _mm_subs_epi8(acc_diff, nadj);

        
        sig += sig_stride;
        mc_running_avg_y += mc_avg_y_stride;
        running_avg_y += avg_y_stride;
    }

    {
        
        union sum_union s;
        int sum_diff = 0;
        s.v = acc_diff;
        sum_diff = s.e[0] + s.e[1] + s.e[2] + s.e[3] + s.e[4] + s.e[5]
                 + s.e[6] + s.e[7] + s.e[8] + s.e[9] + s.e[10] + s.e[11]
                 + s.e[12] + s.e[13] + s.e[14] + s.e[15];

        if (abs(sum_diff) > SUM_DIFF_THRESHOLD)
        {
            return COPY_BLOCK;
        }
    }

    vp8_copy_mem16x16(running_avg->y_buffer + y_offset, avg_y_stride,
                      signal->thismb, sig_stride);
    return FILTER_BLOCK;
}
