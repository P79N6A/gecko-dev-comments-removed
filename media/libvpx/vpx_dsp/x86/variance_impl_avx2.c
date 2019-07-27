









#include <immintrin.h>  

#include "./vpx_dsp_rtcd.h"

void vpx_get16x16var_avx2(const unsigned char *src_ptr,
                          int source_stride,
                          const unsigned char *ref_ptr,
                          int recon_stride,
                          unsigned int *SSE,
                          int *Sum) {
    __m256i src, src_expand_low, src_expand_high, ref, ref_expand_low;
    __m256i ref_expand_high, madd_low, madd_high;
    unsigned int i, src_2strides, ref_2strides;
    __m256i zero_reg = _mm256_set1_epi16(0);
    __m256i sum_ref_src = _mm256_set1_epi16(0);
    __m256i madd_ref_src = _mm256_set1_epi16(0);

    
    
    src_2strides = source_stride << 1;
    ref_2strides = recon_stride << 1;
    for (i = 0; i < 8; i++) {
        src = _mm256_castsi128_si256(
              _mm_loadu_si128((__m128i const *) (src_ptr)));
        src = _mm256_inserti128_si256(src,
              _mm_loadu_si128((__m128i const *)(src_ptr+source_stride)), 1);

        ref =_mm256_castsi128_si256(
             _mm_loadu_si128((__m128i const *) (ref_ptr)));
        ref = _mm256_inserti128_si256(ref,
              _mm_loadu_si128((__m128i const *)(ref_ptr+recon_stride)), 1);

        
        src_expand_low = _mm256_unpacklo_epi8(src, zero_reg);
        src_expand_high = _mm256_unpackhi_epi8(src, zero_reg);

        ref_expand_low = _mm256_unpacklo_epi8(ref, zero_reg);
        ref_expand_high = _mm256_unpackhi_epi8(ref, zero_reg);

        
        src_expand_low = _mm256_sub_epi16(src_expand_low, ref_expand_low);
        src_expand_high = _mm256_sub_epi16(src_expand_high, ref_expand_high);

        
        madd_low = _mm256_madd_epi16(src_expand_low, src_expand_low);

        
        src_expand_low = _mm256_add_epi16(src_expand_low, src_expand_high);

        
        madd_high = _mm256_madd_epi16(src_expand_high, src_expand_high);

        sum_ref_src = _mm256_add_epi16(sum_ref_src, src_expand_low);

        
        madd_ref_src = _mm256_add_epi32(madd_ref_src,
                       _mm256_add_epi32(madd_low, madd_high));

        src_ptr+= src_2strides;
        ref_ptr+= ref_2strides;
    }

    {
        __m128i sum_res, madd_res;
        __m128i expand_sum_low, expand_sum_high, expand_sum;
        __m128i expand_madd_low, expand_madd_high, expand_madd;
        __m128i ex_expand_sum_low, ex_expand_sum_high, ex_expand_sum;

        
        sum_res = _mm_add_epi16(_mm256_castsi256_si128(sum_ref_src),
                                _mm256_extractf128_si256(sum_ref_src, 1));

        madd_res = _mm_add_epi32(_mm256_castsi256_si128(madd_ref_src),
                                 _mm256_extractf128_si256(madd_ref_src, 1));

        
        expand_sum_low = _mm_unpacklo_epi16(_mm256_castsi256_si128(zero_reg),
                                            sum_res);
        expand_sum_high = _mm_unpackhi_epi16(_mm256_castsi256_si128(zero_reg),
                                             sum_res);

        
        expand_sum_low = _mm_srai_epi32(expand_sum_low, 16);
        expand_sum_high = _mm_srai_epi32(expand_sum_high, 16);

        expand_sum = _mm_add_epi32(expand_sum_low, expand_sum_high);

        
        expand_madd_low = _mm_unpacklo_epi32(madd_res,
                          _mm256_castsi256_si128(zero_reg));
        expand_madd_high = _mm_unpackhi_epi32(madd_res,
                           _mm256_castsi256_si128(zero_reg));

        expand_madd = _mm_add_epi32(expand_madd_low, expand_madd_high);

        ex_expand_sum_low = _mm_unpacklo_epi32(expand_sum,
                            _mm256_castsi256_si128(zero_reg));
        ex_expand_sum_high = _mm_unpackhi_epi32(expand_sum,
                             _mm256_castsi256_si128(zero_reg));

        ex_expand_sum = _mm_add_epi32(ex_expand_sum_low, ex_expand_sum_high);

        
        madd_res = _mm_srli_si128(expand_madd, 8);
        sum_res = _mm_srli_si128(ex_expand_sum, 8);

        madd_res = _mm_add_epi32(madd_res, expand_madd);
        sum_res = _mm_add_epi32(sum_res, ex_expand_sum);

        *((int*)SSE)= _mm_cvtsi128_si32(madd_res);

        *((int*)Sum)= _mm_cvtsi128_si32(sum_res);
    }
}

void vpx_get32x32var_avx2(const unsigned char *src_ptr,
                          int source_stride,
                          const unsigned char *ref_ptr,
                          int recon_stride,
                          unsigned int *SSE,
                          int *Sum) {
    __m256i src, src_expand_low, src_expand_high, ref, ref_expand_low;
    __m256i ref_expand_high, madd_low, madd_high;
    unsigned int i;
    __m256i zero_reg = _mm256_set1_epi16(0);
    __m256i sum_ref_src = _mm256_set1_epi16(0);
    __m256i madd_ref_src = _mm256_set1_epi16(0);

    
    for (i = 0; i < 16; i++) {
       src = _mm256_loadu_si256((__m256i const *) (src_ptr));

       ref = _mm256_loadu_si256((__m256i const *) (ref_ptr));

       
       src_expand_low = _mm256_unpacklo_epi8(src, zero_reg);
       src_expand_high = _mm256_unpackhi_epi8(src, zero_reg);

       ref_expand_low = _mm256_unpacklo_epi8(ref, zero_reg);
       ref_expand_high = _mm256_unpackhi_epi8(ref, zero_reg);

       
       src_expand_low = _mm256_sub_epi16(src_expand_low, ref_expand_low);
       src_expand_high = _mm256_sub_epi16(src_expand_high, ref_expand_high);

       
       madd_low = _mm256_madd_epi16(src_expand_low, src_expand_low);

       
       src_expand_low = _mm256_add_epi16(src_expand_low, src_expand_high);

       
       madd_high = _mm256_madd_epi16(src_expand_high, src_expand_high);

       sum_ref_src = _mm256_add_epi16(sum_ref_src, src_expand_low);

       
       madd_ref_src = _mm256_add_epi32(madd_ref_src,
                      _mm256_add_epi32(madd_low, madd_high));

       src_ptr+= source_stride;
       ref_ptr+= recon_stride;
    }

    {
      __m256i expand_sum_low, expand_sum_high, expand_sum;
      __m256i expand_madd_low, expand_madd_high, expand_madd;
      __m256i ex_expand_sum_low, ex_expand_sum_high, ex_expand_sum;

      
      expand_sum_low = _mm256_unpacklo_epi16(zero_reg, sum_ref_src);
      expand_sum_high = _mm256_unpackhi_epi16(zero_reg, sum_ref_src);

      
      expand_sum_low = _mm256_srai_epi32(expand_sum_low, 16);
      expand_sum_high = _mm256_srai_epi32(expand_sum_high, 16);

      expand_sum = _mm256_add_epi32(expand_sum_low, expand_sum_high);

      
      expand_madd_low = _mm256_unpacklo_epi32(madd_ref_src, zero_reg);
      expand_madd_high = _mm256_unpackhi_epi32(madd_ref_src, zero_reg);

      expand_madd = _mm256_add_epi32(expand_madd_low, expand_madd_high);

      ex_expand_sum_low = _mm256_unpacklo_epi32(expand_sum, zero_reg);
      ex_expand_sum_high = _mm256_unpackhi_epi32(expand_sum, zero_reg);

      ex_expand_sum = _mm256_add_epi32(ex_expand_sum_low, ex_expand_sum_high);

      
      madd_ref_src = _mm256_srli_si256(expand_madd, 8);
      sum_ref_src = _mm256_srli_si256(ex_expand_sum, 8);

      madd_ref_src = _mm256_add_epi32(madd_ref_src, expand_madd);
      sum_ref_src = _mm256_add_epi32(sum_ref_src, ex_expand_sum);

      
      *((int*)SSE)= _mm_cvtsi128_si32(_mm256_castsi256_si128(madd_ref_src)) +
      _mm_cvtsi128_si32(_mm256_extractf128_si256(madd_ref_src, 1));

      *((int*)Sum)= _mm_cvtsi128_si32(_mm256_castsi256_si128(sum_ref_src)) +
      _mm_cvtsi128_si32(_mm256_extractf128_si256(sum_ref_src, 1));
    }
}
