



































#include <xmmintrin.h>

#define OVERRIDE_INNER_PRODUCT_SINGLE
static inline float inner_product_single(const float *a, const float *b, unsigned int len)
{
   int i;
   float ret;
   if (1)
   {
      __m128 sum = _mm_setzero_ps();
      for (i=0;i<len;i+=8)
      {
         sum = _mm_add_ps(sum, _mm_mul_ps(_mm_loadu_ps(a+i), _mm_loadu_ps(b+i)));
         sum = _mm_add_ps(sum, _mm_mul_ps(_mm_loadu_ps(a+i+4), _mm_loadu_ps(b+i+4)));
      }
      sum = _mm_add_ps(sum, _mm_movehl_ps(sum, sum));
      sum = _mm_add_ss(sum, _mm_shuffle_ps(sum, sum, 0x55));
      _mm_store_ss(&ret, sum);
   }
   else
   {
      ret = 0;
      for (i=0;i<len;i++) ret += a[i] * b[i];
   }
   return ret;
}

#define OVERRIDE_INTERPOLATE_PRODUCT_SINGLE
static inline float interpolate_product_single(const float *a, const float *b, unsigned int len, const spx_uint32_t oversample, float *frac) {
  int i;
  float ret;
  if (1)
  {
    __m128 sum = _mm_setzero_ps();
    __m128 f = _mm_loadu_ps(frac);
    for(i=0;i<len;i+=2)
    {
      sum = _mm_add_ps(sum, _mm_mul_ps(_mm_load1_ps(a+i), _mm_loadu_ps(b+i*oversample)));
      sum = _mm_add_ps(sum, _mm_mul_ps(_mm_load1_ps(a+i+1), _mm_loadu_ps(b+(i+1)*oversample)));
    }
    sum = _mm_mul_ps(f, sum);
    sum = _mm_add_ps(sum, _mm_movehl_ps(sum, sum));
    sum = _mm_add_ss(sum, _mm_shuffle_ps(sum, sum, 0x55));
    _mm_store_ss(&ret, sum);
  }
  else
  {
    float accum[4] = {0,0,0,0};
    for(i=0;i<len;i++)
    {
      const float curr_in=a[i];
      accum[0] += curr_in * b[i * oversample + 0];
      accum[1] += curr_in * b[i * oversample + 1];
      accum[2] += curr_in * b[i * oversample + 2];
      accum[3] += curr_in * b[i * oversample + 3];
    }
    ret = accum[0] * frac[0] + accum[1] * frac[1] + accum[2] * frac[2] + accum[3] * frac[3];
  }
  return ret;
}

#ifdef __SSE2__
#include <emmintrin.h>
#define OVERRIDE_INNER_PRODUCT_DOUBLE

static inline double inner_product_double(const float *a, const float *b, unsigned int len)
{
   int i;
   double ret;
   __m128d sum = _mm_setzero_pd();
   __m128 t;
   for (i=0;i<len;i+=8)
   {
      t = _mm_mul_ps(_mm_loadu_ps(a+i), _mm_loadu_ps(b+i));
      sum = _mm_add_pd(sum, _mm_cvtps_pd(t));
      sum = _mm_add_pd(sum, _mm_cvtps_pd(_mm_movehl_ps(t, t)));

      t = _mm_mul_ps(_mm_loadu_ps(a+i+4), _mm_loadu_ps(b+i+4));
      sum = _mm_add_pd(sum, _mm_cvtps_pd(t));
      sum = _mm_add_pd(sum, _mm_cvtps_pd(_mm_movehl_ps(t, t)));
   }
   sum = _mm_add_sd(sum, (__m128d) _mm_movehl_ps((__m128) sum, (__m128) sum));
   _mm_store_sd(&ret, sum);
   return ret;
}

#define OVERRIDE_INTERPOLATE_PRODUCT_DOUBLE
static inline double interpolate_product_double(const float *a, const float *b, unsigned int len, const spx_uint32_t oversample, float *frac) {
  int i;
  double ret;
  __m128d sum;
  __m128d sum1 = _mm_setzero_pd();
  __m128d sum2 = _mm_setzero_pd();
  __m128 f = _mm_loadu_ps(frac);
  __m128d f1 = _mm_cvtps_pd(f);
  __m128d f2 = _mm_cvtps_pd(_mm_movehl_ps(f,f));
  __m128 t;
  for(i=0;i<len;i+=2)
  {
    t = _mm_mul_ps(_mm_load1_ps(a+i), _mm_loadu_ps(b+i*oversample));
    sum1 = _mm_add_pd(sum1, _mm_cvtps_pd(t));
    sum2 = _mm_add_pd(sum2, _mm_cvtps_pd(_mm_movehl_ps(t, t)));

    t = _mm_mul_ps(_mm_load1_ps(a+i+1), _mm_loadu_ps(b+(i+1)*oversample));
    sum1 = _mm_add_pd(sum1, _mm_cvtps_pd(t));
    sum2 = _mm_add_pd(sum2, _mm_cvtps_pd(_mm_movehl_ps(t, t)));
  }
  sum1 = _mm_mul_pd(f1, sum1);
  sum2 = _mm_mul_pd(f2, sum2);
  sum = _mm_add_pd(sum1, sum2);
  sum = _mm_add_sd(sum, (__m128d) _mm_movehl_ps((__m128) sum, (__m128) sum));
  _mm_store_sd(&ret, sum);
  return ret;
}

#endif
