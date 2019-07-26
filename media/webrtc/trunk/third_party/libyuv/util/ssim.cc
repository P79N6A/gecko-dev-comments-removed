









#include "../util/ssim.h"  

#include <math.h>
#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef unsigned int uint32;     
typedef unsigned short uint16;   

#if !defined(LIBYUV_DISABLE_X86) && !defined(__SSE2__) && \
  (defined(_M_X64) || (defined(_M_IX86_FP) && (_M_IX86_FP >= 2)))
#define __SSE2__
#endif
#if !defined(LIBYUV_DISABLE_X86) && defined(__SSE2__)
#include <emmintrin.h>
#endif

#ifdef _OPENMP
#include <omp.h>
#endif


enum { KERNEL = 3, KERNEL_SIZE = 2 * KERNEL + 1 };




static const int K[KERNEL_SIZE] = {
  1, 3, 7, 11, 7, 3, 1    
};
static const double kiW[KERNEL + 1 + 1] = {
  1. / 1089.,   
  1. / 1089.,   
  1. / 1056.,   
  1. / 957.,    
  1. / 726.,    
};

#if !defined(LIBYUV_DISABLE_X86) && defined(__SSE2__)

#define PWEIGHT(A, B)  static_cast<uint16>(K[(A)] * K[(B)])   // weight product
#define MAKE_WEIGHT(L)                                               \
  { { { PWEIGHT(L, 0), PWEIGHT(L, 1), PWEIGHT(L, 2), PWEIGHT(L, 3),  \
        PWEIGHT(L, 4), PWEIGHT(L, 5), PWEIGHT(L, 6), 0 } } }



static const struct {
  union {
    uint16 i16_[8];
    __m128i m_;
  } values_;
} W0 = MAKE_WEIGHT(0),
  W1 = MAKE_WEIGHT(1),
  W2 = MAKE_WEIGHT(2),
  W3 = MAKE_WEIGHT(3);
  
#undef MAKE_WEIGHT
#undef PWEIGHT
#endif


static double FinalizeSSIM(double iw, double xm, double ym,
                           double xxm, double xym, double yym) {
  const double iwx = xm * iw;
  const double iwy = ym * iw;
  double sxx = xxm * iw - iwx * iwx;
  double syy = yym * iw - iwy * iwy;
  
  if (sxx < 0.) sxx = 0.;
  if (syy < 0.) syy = 0.;
  const double sxsy = sqrt(sxx * syy);
  const double sxy = xym * iw - iwx * iwy;
  static const double C11 = (0.01 * 0.01) * (255 * 255);
  static const double C22 = (0.03 * 0.03) * (255 * 255);
  static const double C33 = (0.015 * 0.015) * (255 * 255);
  const double l = (2. * iwx * iwy + C11) / (iwx * iwx + iwy * iwy + C11);
  const double c = (2. * sxsy      + C22) / (sxx + syy + C22);
  const double s = (sxy + C33) / (sxsy + C33);
  return l * c * s;
}







double GetSSIM(const uint8 *org, const uint8 *rec,
               int xo, int yo, int W, int H, int stride) {
  uint32 ws = 0, xm = 0, ym = 0, xxm = 0, xym = 0, yym = 0;
  org += (yo - KERNEL) * stride;
  org += (xo - KERNEL);
  rec += (yo - KERNEL) * stride;
  rec += (xo - KERNEL);
  for (int y_ = 0; y_ < KERNEL_SIZE; ++y_, org += stride, rec += stride) {
    if (((yo - KERNEL + y_) < 0) || ((yo - KERNEL + y_) >= H)) continue;
    const int Wy = K[y_];
    for (int x_ = 0; x_ < KERNEL_SIZE; ++x_) {
      const int Wxy = Wy * K[x_];
      if (((xo - KERNEL + x_) >= 0) && ((xo - KERNEL + x_) < W)) {
        const int org_x = org[x_];
        const int rec_x = rec[x_];
        ws += Wxy;
        xm  += Wxy * org_x;
        ym  += Wxy * rec_x;
        xxm += Wxy * org_x * org_x;
        xym += Wxy * org_x * rec_x;
        yym += Wxy * rec_x * rec_x;
      }
    }
  }
  return FinalizeSSIM(1. / ws, xm, ym, xxm, xym, yym);
}

double GetSSIMFullKernel(const uint8 *org, const uint8 *rec,
                         int xo, int yo, int stride,
                         double area_weight) {
  uint32 xm = 0, ym = 0, xxm = 0, xym = 0, yym = 0;

#if defined(LIBYUV_DISABLE_X86) || !defined(__SSE2__)

  org += yo * stride + xo;
  rec += yo * stride + xo;
  for (int y = 1; y <= KERNEL; y++) {
    const int dy1 = y * stride;
    const int dy2 = y * stride;
    const int Wy = K[KERNEL + y];

    for (int x = 1; x <= KERNEL; x++) {
      
      
      
      
      
      
      
      
      
      
      const int Wxy = Wy * K[KERNEL + x];
      const int ul1 = org[-dy1 - x];
      const int ur1 = org[-dy1 + x];
      const int ll1 = org[dy1 - x];
      const int lr1 = org[dy1 + x];

      const int ul2 = rec[-dy2 - x];
      const int ur2 = rec[-dy2 + x];
      const int ll2 = rec[dy2 - x];
      const int lr2 = rec[dy2 + x];

      xm  += Wxy * (ul1 + ur1 + ll1 + lr1);
      ym  += Wxy * (ul2 + ur2 + ll2 + lr2);
      xxm += Wxy * (ul1 * ul1 + ur1 * ur1 + ll1 * ll1 + lr1 * lr1);
      xym += Wxy * (ul1 * ul2 + ur1 * ur2 + ll1 * ll2 + lr1 * lr2);
      yym += Wxy * (ul2 * ul2 + ur2 * ur2 + ll2 * ll2 + lr2 * lr2);
    }

    
    
    
    
    
    
    
    
    
    
    const int Wxy = Wy * K[KERNEL];
    const int u1 = org[-dy1];
    const int d1 = org[dy1];
    const int l1 = org[-y];
    const int r1 = org[y];

    const int u2 = rec[-dy2];
    const int d2 = rec[dy2];
    const int l2 = rec[-y];
    const int r2 = rec[y];

    xm  += Wxy * (u1 + d1 + l1 + r1);
    ym  += Wxy * (u2 + d2 + l2 + r2);
    xxm += Wxy * (u1 * u1 + d1 * d1 + l1 * l1 + r1 * r1);
    xym += Wxy * (u1 * u2 + d1 * d2 + l1 * l2 + r1 * r2);
    yym += Wxy * (u2 * u2 + d2 * d2 + l2 * l2 + r2 * r2);
  }

  
  const int Wxy = K[KERNEL] * K[KERNEL];
  const int s1 = org[0];
  const int s2 = rec[0];

  xm  += Wxy * s1;
  ym  += Wxy * s2;
  xxm += Wxy * s1 * s1;
  xym += Wxy * s1 * s2;
  yym += Wxy * s2 * s2;

#else   

  org += (yo - KERNEL) * stride + (xo - KERNEL);
  rec += (yo - KERNEL) * stride + (xo - KERNEL);

  const __m128i zero = _mm_setzero_si128();
  __m128i x = zero;
  __m128i y = zero;
  __m128i xx = zero;
  __m128i xy = zero;
  __m128i yy = zero;



#define LOAD_LINE_PAIR(L, WEIGHT) do {                                       \
  const __m128i v0 =                                                         \
      _mm_loadl_epi64(reinterpret_cast<const __m128i*>(org + (L) * stride)); \
  const __m128i v1 =                                                         \
      _mm_loadl_epi64(reinterpret_cast<const __m128i*>(rec + (L) * stride)); \
  const __m128i w0 = _mm_unpacklo_epi8(v0, zero);                            \
  const __m128i w1 = _mm_unpacklo_epi8(v1, zero);                            \
  const __m128i ww0 = _mm_mullo_epi16(w0, (WEIGHT).values_.m_);              \
  const __m128i ww1 = _mm_mullo_epi16(w1, (WEIGHT).values_.m_);              \
  x = _mm_add_epi32(x, _mm_unpacklo_epi16(ww0, zero));                       \
  y = _mm_add_epi32(y, _mm_unpacklo_epi16(ww1, zero));                       \
  x = _mm_add_epi32(x, _mm_unpackhi_epi16(ww0, zero));                       \
  y = _mm_add_epi32(y, _mm_unpackhi_epi16(ww1, zero));                       \
  xx = _mm_add_epi32(xx, _mm_madd_epi16(ww0, w0));                           \
  xy = _mm_add_epi32(xy, _mm_madd_epi16(ww0, w1));                           \
  yy = _mm_add_epi32(yy, _mm_madd_epi16(ww1, w1));                           \
} while (0)

#define ADD_AND_STORE_FOUR_EPI32(M, OUT) do {                                \
  uint32 tmp[4];                                                             \
  _mm_storeu_si128(reinterpret_cast<__m128i*>(tmp), (M));                    \
  (OUT) = tmp[3] + tmp[2] + tmp[1] + tmp[0];                                 \
} while (0)

  LOAD_LINE_PAIR(0, W0);
  LOAD_LINE_PAIR(1, W1);
  LOAD_LINE_PAIR(2, W2);
  LOAD_LINE_PAIR(3, W3);
  LOAD_LINE_PAIR(4, W2);
  LOAD_LINE_PAIR(5, W1);
  LOAD_LINE_PAIR(6, W0);

  ADD_AND_STORE_FOUR_EPI32(x, xm);
  ADD_AND_STORE_FOUR_EPI32(y, ym);
  ADD_AND_STORE_FOUR_EPI32(xx, xxm);
  ADD_AND_STORE_FOUR_EPI32(xy, xym);
  ADD_AND_STORE_FOUR_EPI32(yy, yym);

#undef LOAD_LINE_PAIR
#undef ADD_AND_STORE_FOUR_EPI32
#endif

  return FinalizeSSIM(area_weight, xm, ym, xxm, xym, yym);
}

static int start_max(int x, int y) { return (x > y) ? x : y; }

double CalcSSIM(const uint8 *org, const uint8 *rec,
                const int image_width, const int image_height) {
  double SSIM = 0.;
  const int KERNEL_Y = (image_height < KERNEL) ? image_height : KERNEL;
  const int KERNEL_X = (image_width < KERNEL) ? image_width : KERNEL;
  const int start_x = start_max(image_width - 8 + KERNEL_X, KERNEL_X);
  const int start_y = start_max(image_height - KERNEL_Y, KERNEL_Y);
  const int stride = image_width;

  for (int j = 0; j < KERNEL_Y; ++j) {
    for (int i = 0; i < image_width; ++i) {
      SSIM += GetSSIM(org, rec, i, j, image_width, image_height, stride);
    }
  }

#ifdef _OPENMP
  #pragma omp parallel for reduction(+: SSIM)
#endif
  for (int j = KERNEL_Y; j < image_height - KERNEL_Y; ++j) {
    for (int i = 0; i < KERNEL_X; ++i) {
      SSIM += GetSSIM(org, rec, i, j, image_width, image_height, stride);
    }
    for (int i = KERNEL_X; i < start_x; ++i) {
      SSIM += GetSSIMFullKernel(org, rec, i, j, stride, kiW[0]);
    }
    if (start_x < image_width) {
      
      
      
      
      
      
      const int kScratchWidth = 8;
      const int kScratchStride = kScratchWidth + KERNEL + 1;
      uint8 scratch_org[KERNEL_SIZE * kScratchStride] = { 0 };
      uint8 scratch_rec[KERNEL_SIZE * kScratchStride] = { 0 };

      for (int k = 0; k < KERNEL_SIZE; ++k) {
        const int offset =
            (j - KERNEL + k) * stride + image_width - kScratchWidth;
        memcpy(scratch_org + k * kScratchStride, org + offset, kScratchWidth);
        memcpy(scratch_rec + k * kScratchStride, rec + offset, kScratchWidth);
      }
      for (int k = 0;  k <= KERNEL_X + 1; ++k) {
        SSIM += GetSSIMFullKernel(scratch_org, scratch_rec,
                                  KERNEL + k, KERNEL, kScratchStride, kiW[k]);
      }
    }
  }

  for (int j = start_y; j < image_height; ++j) {
    for (int i = 0; i < image_width; ++i) {
      SSIM += GetSSIM(org, rec, i, j, image_width, image_height, stride);
    }
  }
  return SSIM;
}

double CalcLSSIM(double ssim) {
  return -10.0 * log10(1.0 - ssim);
}

#ifdef __cplusplus
}  
#endif

