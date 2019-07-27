









#include <immintrin.h>
#include "vpx_ports/mem.h"


DECLARE_ALIGNED(32, const unsigned char, filt1_global_avx2[32])= {
  0, 1, 1, 2, 2, 3, 3, 4, 4, 5, 5, 6, 6, 7, 7, 8,
  0, 1, 1, 2, 2, 3, 3, 4, 4, 5, 5, 6, 6, 7, 7, 8};

DECLARE_ALIGNED(32, const unsigned char, filt2_global_avx2[32])= {
  2, 3, 3, 4, 4, 5, 5, 6, 6, 7, 7, 8, 8, 9, 9, 10,
  2, 3, 3, 4, 4, 5, 5, 6, 6, 7, 7, 8, 8, 9, 9, 10};

DECLARE_ALIGNED(32, const unsigned char, filt3_global_avx2[32])= {
  4, 5, 5, 6, 6, 7, 7, 8, 8, 9, 9, 10, 10, 11, 11, 12,
  4, 5, 5, 6, 6, 7, 7, 8, 8, 9, 9, 10, 10, 11, 11, 12};

DECLARE_ALIGNED(32, const unsigned char, filt4_global_avx2[32])= {
  6, 7, 7, 8, 8, 9, 9, 10, 10, 11, 11, 12, 12, 13, 13, 14,
  6, 7, 7, 8, 8, 9, 9, 10, 10, 11, 11, 12, 12, 13, 13, 14};

#if defined(__clang__)
# if __clang_major__ < 3 || (__clang_major__ == 3 && __clang_minor__ <= 3) || \
     (defined(__APPLE__) && __clang_major__ == 5 && __clang_minor__ == 0)
#  define MM256_BROADCASTSI128_SI256(x) \
         _mm_broadcastsi128_si256((__m128i const *)&(x))
# else  
#  define MM256_BROADCASTSI128_SI256(x) _mm256_broadcastsi128_si256(x)
# endif  
#elif defined(__GNUC__)
# if __GNUC__ < 4 || (__GNUC__ == 4 && __GNUC_MINOR__ <= 6)
#  define MM256_BROADCASTSI128_SI256(x) \
         _mm_broadcastsi128_si256((__m128i const *)&(x))
# elif __GNUC__ == 4 && __GNUC_MINOR__ == 7
#  define MM256_BROADCASTSI128_SI256(x) _mm_broadcastsi128_si256(x)
# else  
#  define MM256_BROADCASTSI128_SI256(x) _mm256_broadcastsi128_si256(x)
# endif  
#else  
# define MM256_BROADCASTSI128_SI256(x) _mm256_broadcastsi128_si256(x)
#endif  

void vp9_filter_block1d16_h8_avx2(unsigned char *src_ptr,
                                  unsigned int src_pixels_per_line,
                                  unsigned char *output_ptr,
                                  unsigned int  output_pitch,
                                  unsigned int  output_height,
                                  int16_t *filter) {
  __m128i filtersReg;
  __m256i addFilterReg64, filt1Reg, filt2Reg, filt3Reg, filt4Reg;
  __m256i firstFilters, secondFilters, thirdFilters, forthFilters;
  __m256i srcRegFilt32b1_1, srcRegFilt32b2_1, srcRegFilt32b2, srcRegFilt32b3;
  __m256i srcReg32b1, srcReg32b2, filtersReg32;
  unsigned int i;
  unsigned int src_stride, dst_stride;

  
  addFilterReg64 = _mm256_set1_epi32((int)0x0400040u);
  filtersReg = _mm_loadu_si128((__m128i *)filter);
  
  
  filtersReg =_mm_packs_epi16(filtersReg, filtersReg);
  
  filtersReg32 = MM256_BROADCASTSI128_SI256(filtersReg);

  
  
  firstFilters = _mm256_shuffle_epi8(filtersReg32,
                 _mm256_set1_epi16(0x100u));
  
  
  secondFilters = _mm256_shuffle_epi8(filtersReg32,
                  _mm256_set1_epi16(0x302u));
  
  
  thirdFilters = _mm256_shuffle_epi8(filtersReg32,
                 _mm256_set1_epi16(0x504u));
  
  
  forthFilters = _mm256_shuffle_epi8(filtersReg32,
                 _mm256_set1_epi16(0x706u));

  filt1Reg = _mm256_load_si256((__m256i const *)filt1_global_avx2);
  filt2Reg = _mm256_load_si256((__m256i const *)filt2_global_avx2);
  filt3Reg = _mm256_load_si256((__m256i const *)filt3_global_avx2);
  filt4Reg = _mm256_load_si256((__m256i const *)filt4_global_avx2);

  
  src_stride = src_pixels_per_line << 1;
  dst_stride = output_pitch << 1;
  for (i = output_height; i > 1; i-=2) {
    
    srcReg32b1 = _mm256_castsi128_si256(
                 _mm_loadu_si128((__m128i *)(src_ptr-3)));
    srcReg32b1 = _mm256_inserti128_si256(srcReg32b1,
                 _mm_loadu_si128((__m128i *)
                 (src_ptr+src_pixels_per_line-3)), 1);

    
    srcRegFilt32b1_1= _mm256_shuffle_epi8(srcReg32b1, filt1Reg);
    srcRegFilt32b2= _mm256_shuffle_epi8(srcReg32b1, filt2Reg);

    
    srcRegFilt32b1_1 = _mm256_maddubs_epi16(srcRegFilt32b1_1, firstFilters);
    srcRegFilt32b2 = _mm256_maddubs_epi16(srcRegFilt32b2, secondFilters);

    
    srcRegFilt32b1_1 = _mm256_adds_epi16(srcRegFilt32b1_1, srcRegFilt32b2);

    
    srcRegFilt32b3= _mm256_shuffle_epi8(srcReg32b1, filt4Reg);
    srcRegFilt32b2= _mm256_shuffle_epi8(srcReg32b1, filt3Reg);

    
    srcRegFilt32b3 = _mm256_maddubs_epi16(srcRegFilt32b3, forthFilters);
    srcRegFilt32b2 = _mm256_maddubs_epi16(srcRegFilt32b2, thirdFilters);

    
    srcRegFilt32b1_1 = _mm256_adds_epi16(srcRegFilt32b1_1,
                       _mm256_min_epi16(srcRegFilt32b3, srcRegFilt32b2));

    
    
    srcReg32b2 = _mm256_castsi128_si256(
                 _mm_loadu_si128((__m128i *)(src_ptr+5)));
    srcReg32b2 = _mm256_inserti128_si256(srcReg32b2,
                 _mm_loadu_si128((__m128i *)
                 (src_ptr+src_pixels_per_line+5)), 1);

    
    srcRegFilt32b1_1 = _mm256_adds_epi16(srcRegFilt32b1_1,
                       _mm256_max_epi16(srcRegFilt32b3, srcRegFilt32b2));

    
    srcRegFilt32b2_1 = _mm256_shuffle_epi8(srcReg32b2, filt1Reg);
    srcRegFilt32b2 = _mm256_shuffle_epi8(srcReg32b2, filt2Reg);

    
    srcRegFilt32b2_1 = _mm256_maddubs_epi16(srcRegFilt32b2_1, firstFilters);
    srcRegFilt32b2 = _mm256_maddubs_epi16(srcRegFilt32b2, secondFilters);

    
    srcRegFilt32b2_1 = _mm256_adds_epi16(srcRegFilt32b2_1, srcRegFilt32b2);

    
    srcRegFilt32b3= _mm256_shuffle_epi8(srcReg32b2, filt4Reg);
    srcRegFilt32b2= _mm256_shuffle_epi8(srcReg32b2, filt3Reg);

    
    srcRegFilt32b3 = _mm256_maddubs_epi16(srcRegFilt32b3, forthFilters);
    srcRegFilt32b2 = _mm256_maddubs_epi16(srcRegFilt32b2, thirdFilters);

    
    srcRegFilt32b2_1 = _mm256_adds_epi16(srcRegFilt32b2_1,
                       _mm256_min_epi16(srcRegFilt32b3, srcRegFilt32b2));
    srcRegFilt32b2_1 = _mm256_adds_epi16(srcRegFilt32b2_1,
                       _mm256_max_epi16(srcRegFilt32b3, srcRegFilt32b2));


    srcRegFilt32b1_1 = _mm256_adds_epi16(srcRegFilt32b1_1, addFilterReg64);

    srcRegFilt32b2_1 = _mm256_adds_epi16(srcRegFilt32b2_1, addFilterReg64);

    
    srcRegFilt32b1_1 = _mm256_srai_epi16(srcRegFilt32b1_1, 7);
    srcRegFilt32b2_1 = _mm256_srai_epi16(srcRegFilt32b2_1, 7);

    
    
    
    srcRegFilt32b1_1 = _mm256_packus_epi16(srcRegFilt32b1_1,
                                           srcRegFilt32b2_1);

    src_ptr+=src_stride;

    
    _mm_store_si128((__m128i*)output_ptr,
    _mm256_castsi256_si128(srcRegFilt32b1_1));

    
    _mm_store_si128((__m128i*)(output_ptr+output_pitch),
    _mm256_extractf128_si256(srcRegFilt32b1_1, 1));
    output_ptr+=dst_stride;
  }

  
  
  if (i > 0) {
    __m128i srcReg1, srcReg2, srcRegFilt1_1, srcRegFilt2_1;
    __m128i srcRegFilt2, srcRegFilt3;

    srcReg1 = _mm_loadu_si128((__m128i *)(src_ptr-3));

    
    srcRegFilt1_1 = _mm_shuffle_epi8(srcReg1,
                    _mm256_castsi256_si128(filt1Reg));
    srcRegFilt2 = _mm_shuffle_epi8(srcReg1,
                  _mm256_castsi256_si128(filt2Reg));

    
    srcRegFilt1_1 = _mm_maddubs_epi16(srcRegFilt1_1,
                    _mm256_castsi256_si128(firstFilters));
    srcRegFilt2 = _mm_maddubs_epi16(srcRegFilt2,
                  _mm256_castsi256_si128(secondFilters));

    
    srcRegFilt1_1 = _mm_adds_epi16(srcRegFilt1_1, srcRegFilt2);

    
    srcRegFilt3= _mm_shuffle_epi8(srcReg1,
                 _mm256_castsi256_si128(filt4Reg));
    srcRegFilt2= _mm_shuffle_epi8(srcReg1,
                 _mm256_castsi256_si128(filt3Reg));

    
    srcRegFilt3 = _mm_maddubs_epi16(srcRegFilt3,
                  _mm256_castsi256_si128(forthFilters));
    srcRegFilt2 = _mm_maddubs_epi16(srcRegFilt2,
                  _mm256_castsi256_si128(thirdFilters));

    
    srcRegFilt1_1 = _mm_adds_epi16(srcRegFilt1_1,
                    _mm_min_epi16(srcRegFilt3, srcRegFilt2));

    
    
    srcReg2 = _mm_loadu_si128((__m128i *)(src_ptr+5));

    
    srcRegFilt1_1 = _mm_adds_epi16(srcRegFilt1_1,
                    _mm_max_epi16(srcRegFilt3, srcRegFilt2));

    
    srcRegFilt2_1 = _mm_shuffle_epi8(srcReg2,
                    _mm256_castsi256_si128(filt1Reg));
    srcRegFilt2 = _mm_shuffle_epi8(srcReg2,
                  _mm256_castsi256_si128(filt2Reg));

    
    srcRegFilt2_1 = _mm_maddubs_epi16(srcRegFilt2_1,
                    _mm256_castsi256_si128(firstFilters));
    srcRegFilt2 = _mm_maddubs_epi16(srcRegFilt2,
                  _mm256_castsi256_si128(secondFilters));

    
    srcRegFilt2_1 = _mm_adds_epi16(srcRegFilt2_1, srcRegFilt2);

    
    srcRegFilt3 = _mm_shuffle_epi8(srcReg2,
                  _mm256_castsi256_si128(filt4Reg));
    srcRegFilt2 = _mm_shuffle_epi8(srcReg2,
                  _mm256_castsi256_si128(filt3Reg));

    
    srcRegFilt3 = _mm_maddubs_epi16(srcRegFilt3,
                  _mm256_castsi256_si128(forthFilters));
    srcRegFilt2 = _mm_maddubs_epi16(srcRegFilt2,
                  _mm256_castsi256_si128(thirdFilters));

    
    srcRegFilt2_1 = _mm_adds_epi16(srcRegFilt2_1,
                    _mm_min_epi16(srcRegFilt3, srcRegFilt2));
    srcRegFilt2_1 = _mm_adds_epi16(srcRegFilt2_1,
                    _mm_max_epi16(srcRegFilt3, srcRegFilt2));


    srcRegFilt1_1 = _mm_adds_epi16(srcRegFilt1_1,
                    _mm256_castsi256_si128(addFilterReg64));

    srcRegFilt2_1 = _mm_adds_epi16(srcRegFilt2_1,
                    _mm256_castsi256_si128(addFilterReg64));

    
    srcRegFilt1_1 = _mm_srai_epi16(srcRegFilt1_1, 7);
    srcRegFilt2_1 = _mm_srai_epi16(srcRegFilt2_1, 7);

    
    
    
    srcRegFilt1_1 = _mm_packus_epi16(srcRegFilt1_1, srcRegFilt2_1);

    
    _mm_store_si128((__m128i*)output_ptr, srcRegFilt1_1);
  }
}

void vp9_filter_block1d16_v8_avx2(unsigned char *src_ptr,
                                  unsigned int src_pitch,
                                  unsigned char *output_ptr,
                                  unsigned int out_pitch,
                                  unsigned int output_height,
                                  int16_t *filter) {
  __m128i filtersReg;
  __m256i addFilterReg64;
  __m256i srcReg32b1, srcReg32b2, srcReg32b3, srcReg32b4, srcReg32b5;
  __m256i srcReg32b6, srcReg32b7, srcReg32b8, srcReg32b9, srcReg32b10;
  __m256i srcReg32b11, srcReg32b12, srcReg32b13, filtersReg32;
  __m256i firstFilters, secondFilters, thirdFilters, forthFilters;
  unsigned int i;
  unsigned int src_stride, dst_stride;

  
  addFilterReg64 = _mm256_set1_epi32((int)0x0400040u);
  filtersReg = _mm_loadu_si128((__m128i *)filter);
  
  
  filtersReg =_mm_packs_epi16(filtersReg, filtersReg);
  
  filtersReg32 = MM256_BROADCASTSI128_SI256(filtersReg);

  
  
  firstFilters = _mm256_shuffle_epi8(filtersReg32,
                 _mm256_set1_epi16(0x100u));
  
  
  secondFilters = _mm256_shuffle_epi8(filtersReg32,
                  _mm256_set1_epi16(0x302u));
  
  
  thirdFilters = _mm256_shuffle_epi8(filtersReg32,
                 _mm256_set1_epi16(0x504u));
  
  
  forthFilters = _mm256_shuffle_epi8(filtersReg32,
                 _mm256_set1_epi16(0x706u));

  
  src_stride = src_pitch << 1;
  dst_stride = out_pitch << 1;

  
  srcReg32b1 = _mm256_castsi128_si256(
               _mm_loadu_si128((__m128i *)(src_ptr)));
  srcReg32b2 = _mm256_castsi128_si256(
               _mm_loadu_si128((__m128i *)(src_ptr+src_pitch)));
  srcReg32b3 = _mm256_castsi128_si256(
               _mm_loadu_si128((__m128i *)(src_ptr+src_pitch*2)));
  srcReg32b4 = _mm256_castsi128_si256(
               _mm_loadu_si128((__m128i *)(src_ptr+src_pitch*3)));
  srcReg32b5 = _mm256_castsi128_si256(
               _mm_loadu_si128((__m128i *)(src_ptr+src_pitch*4)));
  srcReg32b6 = _mm256_castsi128_si256(
               _mm_loadu_si128((__m128i *)(src_ptr+src_pitch*5)));
  srcReg32b7 = _mm256_castsi128_si256(
               _mm_loadu_si128((__m128i *)(src_ptr+src_pitch*6)));

  
  srcReg32b1 = _mm256_inserti128_si256(srcReg32b1,
               _mm256_castsi256_si128(srcReg32b2), 1);
  srcReg32b2 = _mm256_inserti128_si256(srcReg32b2,
               _mm256_castsi256_si128(srcReg32b3), 1);
  srcReg32b3 = _mm256_inserti128_si256(srcReg32b3,
               _mm256_castsi256_si128(srcReg32b4), 1);
  srcReg32b4 = _mm256_inserti128_si256(srcReg32b4,
               _mm256_castsi256_si128(srcReg32b5), 1);
  srcReg32b5 = _mm256_inserti128_si256(srcReg32b5,
               _mm256_castsi256_si128(srcReg32b6), 1);
  srcReg32b6 = _mm256_inserti128_si256(srcReg32b6,
               _mm256_castsi256_si128(srcReg32b7), 1);

  
  srcReg32b10 = _mm256_unpacklo_epi8(srcReg32b1, srcReg32b2);
  srcReg32b1 = _mm256_unpackhi_epi8(srcReg32b1, srcReg32b2);

  
  srcReg32b11 = _mm256_unpacklo_epi8(srcReg32b3, srcReg32b4);

  
  srcReg32b3 = _mm256_unpackhi_epi8(srcReg32b3, srcReg32b4);

  
  srcReg32b2 = _mm256_unpacklo_epi8(srcReg32b5, srcReg32b6);

  
  srcReg32b5 = _mm256_unpackhi_epi8(srcReg32b5, srcReg32b6);


  for (i = output_height; i > 1; i-=2) {
     
     
     srcReg32b8 = _mm256_castsi128_si256(
     _mm_loadu_si128((__m128i *)(src_ptr+src_pitch*7)));
     srcReg32b7 = _mm256_inserti128_si256(srcReg32b7,
     _mm256_castsi256_si128(srcReg32b8), 1);
     srcReg32b9 = _mm256_castsi128_si256(
     _mm_loadu_si128((__m128i *)(src_ptr+src_pitch*8)));
     srcReg32b8 = _mm256_inserti128_si256(srcReg32b8,
     _mm256_castsi256_si128(srcReg32b9), 1);

     
     
     srcReg32b4 = _mm256_unpacklo_epi8(srcReg32b7, srcReg32b8);
     srcReg32b7 = _mm256_unpackhi_epi8(srcReg32b7, srcReg32b8);

     
     srcReg32b10 = _mm256_maddubs_epi16(srcReg32b10, firstFilters);
     srcReg32b6 = _mm256_maddubs_epi16(srcReg32b4, forthFilters);
     srcReg32b1 = _mm256_maddubs_epi16(srcReg32b1, firstFilters);
     srcReg32b8 = _mm256_maddubs_epi16(srcReg32b7, forthFilters);

     
     srcReg32b10 = _mm256_adds_epi16(srcReg32b10, srcReg32b6);
     srcReg32b1 = _mm256_adds_epi16(srcReg32b1, srcReg32b8);


     
     srcReg32b8 = _mm256_maddubs_epi16(srcReg32b11, secondFilters);
     srcReg32b6 = _mm256_maddubs_epi16(srcReg32b3, secondFilters);

     
     srcReg32b12 = _mm256_maddubs_epi16(srcReg32b2, thirdFilters);
     srcReg32b13 = _mm256_maddubs_epi16(srcReg32b5, thirdFilters);


     
     srcReg32b10 = _mm256_adds_epi16(srcReg32b10,
                   _mm256_min_epi16(srcReg32b8, srcReg32b12));
     srcReg32b1 = _mm256_adds_epi16(srcReg32b1,
                  _mm256_min_epi16(srcReg32b6, srcReg32b13));

     
     srcReg32b10 = _mm256_adds_epi16(srcReg32b10,
                   _mm256_max_epi16(srcReg32b8, srcReg32b12));
     srcReg32b1 = _mm256_adds_epi16(srcReg32b1,
                  _mm256_max_epi16(srcReg32b6, srcReg32b13));


     srcReg32b10 = _mm256_adds_epi16(srcReg32b10, addFilterReg64);
     srcReg32b1 = _mm256_adds_epi16(srcReg32b1, addFilterReg64);

     
     srcReg32b10 = _mm256_srai_epi16(srcReg32b10, 7);
     srcReg32b1 = _mm256_srai_epi16(srcReg32b1, 7);

     
     
     
     srcReg32b1 = _mm256_packus_epi16(srcReg32b10, srcReg32b1);

     src_ptr+=src_stride;

     
     _mm_store_si128((__m128i*)output_ptr,
     _mm256_castsi256_si128(srcReg32b1));

     
     _mm_store_si128((__m128i*)(output_ptr+out_pitch),
     _mm256_extractf128_si256(srcReg32b1, 1));

     output_ptr+=dst_stride;

     
     srcReg32b10 = srcReg32b11;
     srcReg32b1 = srcReg32b3;
     srcReg32b11 = srcReg32b2;
     srcReg32b3 = srcReg32b5;
     srcReg32b2 = srcReg32b4;
     srcReg32b5 = srcReg32b7;
     srcReg32b7 = srcReg32b9;
  }
  if (i > 0) {
    __m128i srcRegFilt1, srcRegFilt3, srcRegFilt4, srcRegFilt5;
    __m128i srcRegFilt6, srcRegFilt7, srcRegFilt8;
    
    srcRegFilt8 = _mm_loadu_si128((__m128i *)(src_ptr+src_pitch*7));

    
    srcRegFilt4 = _mm_unpacklo_epi8(
                  _mm256_castsi256_si128(srcReg32b7), srcRegFilt8);
    srcRegFilt7 = _mm_unpackhi_epi8(
                  _mm256_castsi256_si128(srcReg32b7), srcRegFilt8);

    
    srcRegFilt1 = _mm_maddubs_epi16(_mm256_castsi256_si128(srcReg32b10),
                  _mm256_castsi256_si128(firstFilters));
    srcRegFilt4 = _mm_maddubs_epi16(srcRegFilt4,
                  _mm256_castsi256_si128(forthFilters));
    srcRegFilt3 = _mm_maddubs_epi16(_mm256_castsi256_si128(srcReg32b1),
                  _mm256_castsi256_si128(firstFilters));
    srcRegFilt7 = _mm_maddubs_epi16(srcRegFilt7,
                  _mm256_castsi256_si128(forthFilters));

    
    srcRegFilt1 = _mm_adds_epi16(srcRegFilt1, srcRegFilt4);
    srcRegFilt3 = _mm_adds_epi16(srcRegFilt3, srcRegFilt7);


    
    srcRegFilt4 = _mm_maddubs_epi16(_mm256_castsi256_si128(srcReg32b11),
                  _mm256_castsi256_si128(secondFilters));
    srcRegFilt5 = _mm_maddubs_epi16(_mm256_castsi256_si128(srcReg32b3),
                  _mm256_castsi256_si128(secondFilters));

    
    srcRegFilt6 = _mm_maddubs_epi16(_mm256_castsi256_si128(srcReg32b2),
                  _mm256_castsi256_si128(thirdFilters));
    srcRegFilt7 = _mm_maddubs_epi16(_mm256_castsi256_si128(srcReg32b5),
                  _mm256_castsi256_si128(thirdFilters));

    
    srcRegFilt1 = _mm_adds_epi16(srcRegFilt1,
                  _mm_min_epi16(srcRegFilt4, srcRegFilt6));
    srcRegFilt3 = _mm_adds_epi16(srcRegFilt3,
                  _mm_min_epi16(srcRegFilt5, srcRegFilt7));

    
    srcRegFilt1 = _mm_adds_epi16(srcRegFilt1,
                  _mm_max_epi16(srcRegFilt4, srcRegFilt6));
    srcRegFilt3 = _mm_adds_epi16(srcRegFilt3,
                  _mm_max_epi16(srcRegFilt5, srcRegFilt7));


    srcRegFilt1 = _mm_adds_epi16(srcRegFilt1,
                  _mm256_castsi256_si128(addFilterReg64));
    srcRegFilt3 = _mm_adds_epi16(srcRegFilt3,
                  _mm256_castsi256_si128(addFilterReg64));

    
    srcRegFilt1 = _mm_srai_epi16(srcRegFilt1, 7);
    srcRegFilt3 = _mm_srai_epi16(srcRegFilt3, 7);

    
    
    
    srcRegFilt1 = _mm_packus_epi16(srcRegFilt1, srcRegFilt3);

    
    _mm_store_si128((__m128i*)output_ptr, srcRegFilt1);
  }
}
