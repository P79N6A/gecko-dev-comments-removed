
















#ifndef WEBRTC_SPL_SIGNAL_PROCESSING_LIBRARY_H_
#define WEBRTC_SPL_SIGNAL_PROCESSING_LIBRARY_H_

#include <string.h>
#include "typedefs.h"


#define WEBRTC_SPL_WORD16_MAX       32767
#define WEBRTC_SPL_WORD16_MIN       -32768
#define WEBRTC_SPL_WORD32_MAX       (WebRtc_Word32)0x7fffffff
#define WEBRTC_SPL_WORD32_MIN       (WebRtc_Word32)0x80000000
#define WEBRTC_SPL_MAX_LPC_ORDER    14
#define WEBRTC_SPL_MAX_SEED_USED    0x80000000L
#define WEBRTC_SPL_MIN(A, B)        (A < B ? A : B) // Get min value
#define WEBRTC_SPL_MAX(A, B)        (A > B ? A : B) // Get max value


#define WEBRTC_SPL_ABS_W16(a) \
    (((WebRtc_Word16)a >= 0) ? ((WebRtc_Word16)a) : -((WebRtc_Word16)a))
#define WEBRTC_SPL_ABS_W32(a) \
    (((WebRtc_Word32)a >= 0) ? ((WebRtc_Word32)a) : -((WebRtc_Word32)a))

#ifdef WEBRTC_ARCH_LITTLE_ENDIAN
#define WEBRTC_SPL_GET_BYTE(a, nr)  (((WebRtc_Word8 *)a)[nr])
#define WEBRTC_SPL_SET_BYTE(d_ptr, val, index) \
    (((WebRtc_Word8 *)d_ptr)[index] = (val))
#else
#define WEBRTC_SPL_GET_BYTE(a, nr) \
    ((((WebRtc_Word16 *)a)[nr >> 1]) >> (((nr + 1) & 0x1) * 8) & 0x00ff)
#define WEBRTC_SPL_SET_BYTE(d_ptr, val, index) \
    ((WebRtc_Word16 *)d_ptr)[index >> 1] = \
    ((((WebRtc_Word16 *)d_ptr)[index >> 1]) \
    & (0x00ff << (8 * ((index) & 0x1)))) | (val << (8 * ((index + 1) & 0x1)))
#endif

#define WEBRTC_SPL_MUL(a, b) \
    ((WebRtc_Word32) ((WebRtc_Word32)(a) * (WebRtc_Word32)(b)))
#define WEBRTC_SPL_UMUL(a, b) \
    ((WebRtc_UWord32) ((WebRtc_UWord32)(a) * (WebRtc_UWord32)(b)))
#define WEBRTC_SPL_UMUL_RSFT16(a, b) \
    ((WebRtc_UWord32) ((WebRtc_UWord32)(a) * (WebRtc_UWord32)(b)) >> 16)
#define WEBRTC_SPL_UMUL_16_16(a, b) \
    ((WebRtc_UWord32) (WebRtc_UWord16)(a) * (WebRtc_UWord16)(b))
#define WEBRTC_SPL_UMUL_16_16_RSFT16(a, b) \
    (((WebRtc_UWord32) (WebRtc_UWord16)(a) * (WebRtc_UWord16)(b)) >> 16)
#define WEBRTC_SPL_UMUL_32_16(a, b) \
    ((WebRtc_UWord32) ((WebRtc_UWord32)(a) * (WebRtc_UWord16)(b)))
#define WEBRTC_SPL_UMUL_32_16_RSFT16(a, b) \
    ((WebRtc_UWord32) ((WebRtc_UWord32)(a) * (WebRtc_UWord16)(b)) >> 16)
#define WEBRTC_SPL_MUL_16_U16(a, b) \
    ((WebRtc_Word32)(WebRtc_Word16)(a) * (WebRtc_UWord16)(b))
#define WEBRTC_SPL_DIV(a, b) \
    ((WebRtc_Word32) ((WebRtc_Word32)(a) / (WebRtc_Word32)(b)))
#define WEBRTC_SPL_UDIV(a, b) \
    ((WebRtc_UWord32) ((WebRtc_UWord32)(a) / (WebRtc_UWord32)(b)))

#ifndef WEBRTC_ARCH_ARM_V7

#define WEBRTC_SPL_MUL_16_16(a, b) \
    ((WebRtc_Word32) (((WebRtc_Word16)(a)) * ((WebRtc_Word16)(b))))
#define WEBRTC_SPL_MUL_16_32_RSFT16(a, b) \
    (WEBRTC_SPL_MUL_16_16(a, b >> 16) \
     + ((WEBRTC_SPL_MUL_16_16(a, (b & 0xffff) >> 1) + 0x4000) >> 15))
#define WEBRTC_SPL_MUL_32_32_RSFT32(a32a, a32b, b32) \
    ((WebRtc_Word32)(WEBRTC_SPL_MUL_16_32_RSFT16(a32a, b32) \
    + (WEBRTC_SPL_MUL_16_32_RSFT16(a32b, b32) >> 16)))
#define WEBRTC_SPL_MUL_32_32_RSFT32BI(a32, b32) \
    ((WebRtc_Word32)(WEBRTC_SPL_MUL_16_32_RSFT16(( \
    (WebRtc_Word16)(a32 >> 16)), b32) + \
    (WEBRTC_SPL_MUL_16_32_RSFT16(( \
    (WebRtc_Word16)((a32 & 0x0000FFFF) >> 1)), b32) >> 15)))
#endif

#define WEBRTC_SPL_MUL_16_32_RSFT11(a, b) \
    ((WEBRTC_SPL_MUL_16_16(a, (b) >> 16) << 5) \
    + (((WEBRTC_SPL_MUL_16_U16(a, (WebRtc_UWord16)(b)) >> 1) + 0x0200) >> 10))
#define WEBRTC_SPL_MUL_16_32_RSFT14(a, b) \
    ((WEBRTC_SPL_MUL_16_16(a, (b) >> 16) << 2) \
    + (((WEBRTC_SPL_MUL_16_U16(a, (WebRtc_UWord16)(b)) >> 1) + 0x1000) >> 13))
#define WEBRTC_SPL_MUL_16_32_RSFT15(a, b) \
    ((WEBRTC_SPL_MUL_16_16(a, (b) >> 16) << 1) \
    + (((WEBRTC_SPL_MUL_16_U16(a, (WebRtc_UWord16)(b)) >> 1) + 0x2000) >> 14))

#define WEBRTC_SPL_MUL_16_16_RSFT(a, b, c) \
    (WEBRTC_SPL_MUL_16_16(a, b) >> (c))

#define WEBRTC_SPL_MUL_16_16_RSFT_WITH_ROUND(a, b, c) \
    ((WEBRTC_SPL_MUL_16_16(a, b) + ((WebRtc_Word32) \
                                  (((WebRtc_Word32)1) << ((c) - 1)))) >> (c))
#define WEBRTC_SPL_MUL_16_16_RSFT_WITH_FIXROUND(a, b) \
    ((WEBRTC_SPL_MUL_16_16(a, b) + ((WebRtc_Word32) (1 << 14))) >> 15)


#define WEBRTC_SPL_SCALEDIFF32(A, B, C) \
    (C + (B >> 16) * A + (((WebRtc_UWord32)(0x0000FFFF & B) * A) >> 16))

#define WEBRTC_SPL_ADD_SAT_W32(a, b)    WebRtcSpl_AddSatW32(a, b)
#define WEBRTC_SPL_SAT(a, b, c)         (b > a ? a : b < c ? c : b)
#define WEBRTC_SPL_MUL_32_16(a, b)      ((a) * (b))

#define WEBRTC_SPL_SUB_SAT_W32(a, b)    WebRtcSpl_SubSatW32(a, b)
#define WEBRTC_SPL_ADD_SAT_W16(a, b)    WebRtcSpl_AddSatW16(a, b)
#define WEBRTC_SPL_SUB_SAT_W16(a, b)    WebRtcSpl_SubSatW16(a, b)


#define WEBRTC_SPL_IS_NEG(a)            ((a) & 0x80000000)


#define WEBRTC_SPL_SHIFT_W16(x, c) \
    (((c) >= 0) ? ((x) << (c)) : ((x) >> (-(c))))
#define WEBRTC_SPL_SHIFT_W32(x, c) \
    (((c) >= 0) ? ((x) << (c)) : ((x) >> (-(c))))



#define WEBRTC_SPL_RSHIFT_W16(x, c)     ((x) >> (c))
#define WEBRTC_SPL_LSHIFT_W16(x, c)     ((x) << (c))
#define WEBRTC_SPL_RSHIFT_W32(x, c)     ((x) >> (c))
#define WEBRTC_SPL_LSHIFT_W32(x, c)     ((x) << (c))

#define WEBRTC_SPL_RSHIFT_U16(x, c)     ((WebRtc_UWord16)(x) >> (c))
#define WEBRTC_SPL_LSHIFT_U16(x, c)     ((WebRtc_UWord16)(x) << (c))
#define WEBRTC_SPL_RSHIFT_U32(x, c)     ((WebRtc_UWord32)(x) >> (c))
#define WEBRTC_SPL_LSHIFT_U32(x, c)     ((WebRtc_UWord32)(x) << (c))

#define WEBRTC_SPL_VNEW(t, n)           (t *) malloc (sizeof (t) * (n))
#define WEBRTC_SPL_FREE                 free

#define WEBRTC_SPL_RAND(a) \
    ((WebRtc_Word16)(WEBRTC_SPL_MUL_16_16_RSFT((a), 18816, 7) & 0x00007fff))

#ifdef __cplusplus
extern "C"
{
#endif

#define WEBRTC_SPL_MEMCPY_W8(v1, v2, length) \
   memcpy(v1, v2, (length) * sizeof(char))
#define WEBRTC_SPL_MEMCPY_W16(v1, v2, length) \
   memcpy(v1, v2, (length) * sizeof(WebRtc_Word16))

#define WEBRTC_SPL_MEMMOVE_W16(v1, v2, length) \
   memmove(v1, v2, (length) * sizeof(WebRtc_Word16))


#include "spl_inl.h"









void WebRtcSpl_Init();


WebRtc_Word16 WebRtcSpl_get_version(char* version,
                                    WebRtc_Word16 length_in_bytes);

int WebRtcSpl_GetScalingSquare(WebRtc_Word16* in_vector,
                               int in_vector_length,
                               int times);



void WebRtcSpl_MemSetW16(WebRtc_Word16* vector,
                         WebRtc_Word16 set_value,
                         int vector_length);
void WebRtcSpl_MemSetW32(WebRtc_Word32* vector,
                         WebRtc_Word32 set_value,
                         int vector_length);
void WebRtcSpl_MemCpyReversedOrder(WebRtc_Word16* out_vector,
                                   WebRtc_Word16* in_vector,
                                   int vector_length);
WebRtc_Word16 WebRtcSpl_CopyFromEndW16(G_CONST WebRtc_Word16* in_vector,
                                       WebRtc_Word16 in_vector_length,
                                       WebRtc_Word16 samples,
                                       WebRtc_Word16* out_vector);
WebRtc_Word16 WebRtcSpl_ZerosArrayW16(WebRtc_Word16* vector,
                                      WebRtc_Word16 vector_length);
WebRtc_Word16 WebRtcSpl_ZerosArrayW32(WebRtc_Word32* vector,
                                      WebRtc_Word16 vector_length);
WebRtc_Word16 WebRtcSpl_OnesArrayW16(WebRtc_Word16* vector,
                                     WebRtc_Word16 vector_length);
WebRtc_Word16 WebRtcSpl_OnesArrayW32(WebRtc_Word32* vector,
                                     WebRtc_Word16 vector_length);














typedef int16_t (*MaxAbsValueW16)(const int16_t* vector, int length);
extern MaxAbsValueW16 WebRtcSpl_MaxAbsValueW16;
int16_t WebRtcSpl_MaxAbsValueW16C(const int16_t* vector, int length);
#if (defined WEBRTC_DETECT_ARM_NEON) || (defined WEBRTC_ARCH_ARM_NEON)
int16_t WebRtcSpl_MaxAbsValueW16Neon(const int16_t* vector, int length);
#endif









typedef int32_t (*MaxAbsValueW32)(const int32_t* vector, int length);
extern MaxAbsValueW32 WebRtcSpl_MaxAbsValueW32;
int32_t WebRtcSpl_MaxAbsValueW32C(const int32_t* vector, int length);
#if (defined WEBRTC_DETECT_ARM_NEON) || (defined WEBRTC_ARCH_ARM_NEON)
int32_t WebRtcSpl_MaxAbsValueW32Neon(const int32_t* vector, int length);
#endif











typedef int16_t (*MaxValueW16)(const int16_t* vector, int length);
extern MaxValueW16 WebRtcSpl_MaxValueW16;
int16_t WebRtcSpl_MaxValueW16C(const int16_t* vector, int length);
#if (defined WEBRTC_DETECT_ARM_NEON) || (defined WEBRTC_ARCH_ARM_NEON)
int16_t WebRtcSpl_MaxValueW16Neon(const int16_t* vector, int length);
#endif











typedef int32_t (*MaxValueW32)(const int32_t* vector, int length);
extern MaxValueW32 WebRtcSpl_MaxValueW32;
int32_t WebRtcSpl_MaxValueW32C(const int32_t* vector, int length);
#if (defined WEBRTC_DETECT_ARM_NEON) || (defined WEBRTC_ARCH_ARM_NEON)
int32_t WebRtcSpl_MaxValueW32Neon(const int32_t* vector, int length);
#endif











typedef int16_t (*MinValueW16)(const int16_t* vector, int length);
extern MinValueW16 WebRtcSpl_MinValueW16;
int16_t WebRtcSpl_MinValueW16C(const int16_t* vector, int length);
#if (defined WEBRTC_DETECT_ARM_NEON) || (defined WEBRTC_ARCH_ARM_NEON)
int16_t WebRtcSpl_MinValueW16Neon(const int16_t* vector, int length);
#endif











typedef int32_t (*MinValueW32)(const int32_t* vector, int length);
extern MinValueW32 WebRtcSpl_MinValueW32;
int32_t WebRtcSpl_MinValueW32C(const int32_t* vector, int length);
#if (defined WEBRTC_DETECT_ARM_NEON) || (defined WEBRTC_ARCH_ARM_NEON)
int32_t WebRtcSpl_MinValueW32Neon(const int32_t* vector, int length);
#endif












int WebRtcSpl_MaxAbsIndexW16(const int16_t* vector, int length);










int WebRtcSpl_MaxIndexW16(const int16_t* vector, int length);










int WebRtcSpl_MaxIndexW32(const int32_t* vector, int length);










int WebRtcSpl_MinIndexW16(const int16_t* vector, int length);










int WebRtcSpl_MinIndexW32(const int32_t* vector, int length);






void WebRtcSpl_VectorBitShiftW16(WebRtc_Word16* out_vector,
                                 WebRtc_Word16 vector_length,
                                 G_CONST WebRtc_Word16* in_vector,
                                 WebRtc_Word16 right_shifts);
void WebRtcSpl_VectorBitShiftW32(WebRtc_Word32* out_vector,
                                 WebRtc_Word16 vector_length,
                                 G_CONST WebRtc_Word32* in_vector,
                                 WebRtc_Word16 right_shifts);
void WebRtcSpl_VectorBitShiftW32ToW16(int16_t* out_vector,
                                      int vector_length,
                                      const int32_t* in_vector,
                                      int right_shifts);
void WebRtcSpl_ScaleVector(G_CONST WebRtc_Word16* in_vector,
                           WebRtc_Word16* out_vector,
                           WebRtc_Word16 gain,
                           WebRtc_Word16 vector_length,
                           WebRtc_Word16 right_shifts);
void WebRtcSpl_ScaleVectorWithSat(G_CONST WebRtc_Word16* in_vector,
                                  WebRtc_Word16* out_vector,
                                  WebRtc_Word16 gain,
                                  WebRtc_Word16 vector_length,
                                  WebRtc_Word16 right_shifts);
void WebRtcSpl_ScaleAndAddVectors(G_CONST WebRtc_Word16* in_vector1,
                                  WebRtc_Word16 gain1, int right_shifts1,
                                  G_CONST WebRtc_Word16* in_vector2,
                                  WebRtc_Word16 gain2, int right_shifts2,
                                  WebRtc_Word16* out_vector,
                                  int vector_length);



















typedef int (*ScaleAndAddVectorsWithRound)(const int16_t* in_vector1,
                                           int16_t in_vector1_scale,
                                           const int16_t* in_vector2,
                                           int16_t in_vector2_scale,
                                           int right_shifts,
                                           int16_t* out_vector,
                                           int length);
extern ScaleAndAddVectorsWithRound WebRtcSpl_ScaleAndAddVectorsWithRound;
int WebRtcSpl_ScaleAndAddVectorsWithRoundC(const int16_t* in_vector1,
                                           int16_t in_vector1_scale,
                                           const int16_t* in_vector2,
                                           int16_t in_vector2_scale,
                                           int right_shifts,
                                           int16_t* out_vector,
                                           int length);
#if (defined WEBRTC_DETECT_ARM_NEON) || (defined WEBRTC_ARCH_ARM_NEON)
int WebRtcSpl_ScaleAndAddVectorsWithRoundNeon(const int16_t* in_vector1,
                                              int16_t in_vector1_scale,
                                              const int16_t* in_vector2,
                                              int16_t in_vector2_scale,
                                              int right_shifts,
                                              int16_t* out_vector,
                                              int length);
#endif




void WebRtcSpl_ReverseOrderMultArrayElements(WebRtc_Word16* out_vector,
                                             G_CONST WebRtc_Word16* in_vector,
                                             G_CONST WebRtc_Word16* window,
                                             WebRtc_Word16 vector_length,
                                             WebRtc_Word16 right_shifts);
void WebRtcSpl_ElementwiseVectorMult(WebRtc_Word16* out_vector,
                                     G_CONST WebRtc_Word16* in_vector,
                                     G_CONST WebRtc_Word16* window,
                                     WebRtc_Word16 vector_length,
                                     WebRtc_Word16 right_shifts);
void WebRtcSpl_AddVectorsAndShift(WebRtc_Word16* out_vector,
                                  G_CONST WebRtc_Word16* in_vector1,
                                  G_CONST WebRtc_Word16* in_vector2,
                                  WebRtc_Word16 vector_length,
                                  WebRtc_Word16 right_shifts);
void WebRtcSpl_AddAffineVectorToVector(WebRtc_Word16* out_vector,
                                       WebRtc_Word16* in_vector,
                                       WebRtc_Word16 gain,
                                       WebRtc_Word32 add_constant,
                                       WebRtc_Word16 right_shifts,
                                       int vector_length);
void WebRtcSpl_AffineTransformVector(WebRtc_Word16* out_vector,
                                     WebRtc_Word16* in_vector,
                                     WebRtc_Word16 gain,
                                     WebRtc_Word32 add_constant,
                                     WebRtc_Word16 right_shifts,
                                     int vector_length);























int WebRtcSpl_AutoCorrelation(const int16_t* in_vector,
                              int in_vector_length,
                              int order,
                              int32_t* result,
                              int* scale);















WebRtc_Word16 WebRtcSpl_LevinsonDurbin(WebRtc_Word32* auto_corr,
                                       WebRtc_Word16* lpc_coef,
                                       WebRtc_Word16* refl_coef,
                                       WebRtc_Word16 order);















void WebRtcSpl_ReflCoefToLpc(G_CONST WebRtc_Word16* refl_coef,
                             int use_order,
                             WebRtc_Word16* lpc_coef);












void WebRtcSpl_LpcToReflCoef(WebRtc_Word16* lpc_coef,
                             int use_order,
                             WebRtc_Word16* refl_coef);









void WebRtcSpl_AutoCorrToReflCoef(G_CONST WebRtc_Word32* auto_corr,
                                  int use_order,
                                  WebRtc_Word16* refl_coef);
























typedef void (*CrossCorrelation)(int32_t* cross_correlation,
                                 const int16_t* seq1,
                                 const int16_t* seq2,
                                 int16_t dim_seq,
                                 int16_t dim_cross_correlation,
                                 int16_t right_shifts,
                                 int16_t step_seq2);
extern CrossCorrelation WebRtcSpl_CrossCorrelation;
void WebRtcSpl_CrossCorrelationC(int32_t* cross_correlation,
                                 const int16_t* seq1,
                                 const int16_t* seq2,
                                 int16_t dim_seq,
                                 int16_t dim_cross_correlation,
                                 int16_t right_shifts,
                                 int16_t step_seq2);
#if (defined WEBRTC_DETECT_ARM_NEON) || (defined WEBRTC_ARCH_ARM_NEON)
void WebRtcSpl_CrossCorrelationNeon(int32_t* cross_correlation,
                                    const int16_t* seq1,
                                    const int16_t* seq2,
                                    int16_t dim_seq,
                                    int16_t dim_cross_correlation,
                                    int16_t right_shifts,
                                    int16_t step_seq2);
#endif









void WebRtcSpl_GetHanningWindow(WebRtc_Word16* window, WebRtc_Word16 size);










void WebRtcSpl_SqrtOfOneMinusXSquared(WebRtc_Word16* in_vector,
                                      int vector_length,
                                      WebRtc_Word16* out_vector);




WebRtc_UWord32 WebRtcSpl_IncreaseSeed(WebRtc_UWord32* seed);
WebRtc_Word16 WebRtcSpl_RandU(WebRtc_UWord32* seed);
WebRtc_Word16 WebRtcSpl_RandN(WebRtc_UWord32* seed);
WebRtc_Word16 WebRtcSpl_RandUArray(WebRtc_Word16* vector,
                                   WebRtc_Word16 vector_length,
                                   WebRtc_UWord32* seed);



WebRtc_Word32 WebRtcSpl_Sqrt(WebRtc_Word32 value);
WebRtc_Word32 WebRtcSpl_SqrtFloor(WebRtc_Word32 value);



WebRtc_UWord32 WebRtcSpl_DivU32U16(WebRtc_UWord32 num, WebRtc_UWord16 den);
WebRtc_Word32 WebRtcSpl_DivW32W16(WebRtc_Word32 num, WebRtc_Word16 den);
WebRtc_Word16 WebRtcSpl_DivW32W16ResW16(WebRtc_Word32 num, WebRtc_Word16 den);
WebRtc_Word32 WebRtcSpl_DivResultInQ31(WebRtc_Word32 num, WebRtc_Word32 den);
WebRtc_Word32 WebRtcSpl_DivW32HiLow(WebRtc_Word32 num, WebRtc_Word16 den_hi,
                                    WebRtc_Word16 den_low);


WebRtc_Word32 WebRtcSpl_Energy(WebRtc_Word16* vector,
                               int vector_length,
                               int* scale_factor);












int32_t WebRtcSpl_DotProductWithScale(const int16_t* vector1,
                                      const int16_t* vector2,
                                      int length,
                                      int scaling);


int WebRtcSpl_FilterAR(G_CONST WebRtc_Word16* ar_coef, int ar_coef_length,
                       G_CONST WebRtc_Word16* in_vector, int in_vector_length,
                       WebRtc_Word16* filter_state, int filter_state_length,
                       WebRtc_Word16* filter_state_low,
                       int filter_state_low_length, WebRtc_Word16* out_vector,
                       WebRtc_Word16* out_vector_low, int out_vector_low_length);

void WebRtcSpl_FilterMAFastQ12(WebRtc_Word16* in_vector,
                               WebRtc_Word16* out_vector,
                               WebRtc_Word16* ma_coef,
                               WebRtc_Word16 ma_coef_length,
                               WebRtc_Word16 vector_length);











void WebRtcSpl_FilterARFastQ12(const int16_t* data_in,
                               int16_t* data_out,
                               const int16_t* __restrict coefficients,
                               int coefficients_length,
                               int data_length);

















typedef int (*DownsampleFast)(const int16_t* data_in,
                              int data_in_length,
                              int16_t* data_out,
                              int data_out_length,
                              const int16_t* __restrict coefficients,
                              int coefficients_length,
                              int factor,
                              int delay);
extern DownsampleFast WebRtcSpl_DownsampleFast;
int WebRtcSpl_DownsampleFastC(const int16_t* data_in,
                              int data_in_length,
                              int16_t* data_out,
                              int data_out_length,
                              const int16_t* __restrict coefficients,
                              int coefficients_length,
                              int factor,
                              int delay);
#if (defined WEBRTC_DETECT_ARM_NEON) || (defined WEBRTC_ARCH_ARM_NEON)
int WebRtcSpl_DownsampleFastNeon(const int16_t* data_in,
                                 int data_in_length,
                                 int16_t* data_out,
                                 int data_out_length,
                                 const int16_t* __restrict coefficients,
                                 int coefficients_length,
                                 int factor,
                                 int delay);
#endif





int WebRtcSpl_ComplexFFT(WebRtc_Word16 vector[], int stages, int mode);
int WebRtcSpl_ComplexIFFT(WebRtc_Word16 vector[], int stages, int mode);















void WebRtcSpl_ComplexBitReverse(int16_t* __restrict complex_data, int stages);





















typedef struct
{
    WebRtc_Word32 S_22_44[8];
    WebRtc_Word32 S_44_32[8];
    WebRtc_Word32 S_32_16[8];
} WebRtcSpl_State22khzTo16khz;

void WebRtcSpl_Resample22khzTo16khz(const WebRtc_Word16* in,
                                    WebRtc_Word16* out,
                                    WebRtcSpl_State22khzTo16khz* state,
                                    WebRtc_Word32* tmpmem);

void WebRtcSpl_ResetResample22khzTo16khz(WebRtcSpl_State22khzTo16khz* state);


typedef struct
{
    WebRtc_Word32 S_16_32[8];
    WebRtc_Word32 S_32_22[8];
} WebRtcSpl_State16khzTo22khz;

void WebRtcSpl_Resample16khzTo22khz(const WebRtc_Word16* in,
                                    WebRtc_Word16* out,
                                    WebRtcSpl_State16khzTo22khz* state,
                                    WebRtc_Word32* tmpmem);

void WebRtcSpl_ResetResample16khzTo22khz(WebRtcSpl_State16khzTo22khz* state);


typedef struct
{
    WebRtc_Word32 S_22_22[16];
    WebRtc_Word32 S_22_16[8];
    WebRtc_Word32 S_16_8[8];
} WebRtcSpl_State22khzTo8khz;

void WebRtcSpl_Resample22khzTo8khz(const WebRtc_Word16* in, WebRtc_Word16* out,
                                   WebRtcSpl_State22khzTo8khz* state,
                                   WebRtc_Word32* tmpmem);

void WebRtcSpl_ResetResample22khzTo8khz(WebRtcSpl_State22khzTo8khz* state);


typedef struct
{
    WebRtc_Word32 S_8_16[8];
    WebRtc_Word32 S_16_11[8];
    WebRtc_Word32 S_11_22[8];
} WebRtcSpl_State8khzTo22khz;

void WebRtcSpl_Resample8khzTo22khz(const WebRtc_Word16* in, WebRtc_Word16* out,
                                   WebRtcSpl_State8khzTo22khz* state,
                                   WebRtc_Word32* tmpmem);

void WebRtcSpl_ResetResample8khzTo22khz(WebRtcSpl_State8khzTo22khz* state);












void WebRtcSpl_Resample48khzTo32khz(const WebRtc_Word32* In, WebRtc_Word32* Out,
                                    const WebRtc_Word32 K);

void WebRtcSpl_Resample32khzTo24khz(const WebRtc_Word32* In, WebRtc_Word32* Out,
                                    const WebRtc_Word32 K);

void WebRtcSpl_Resample44khzTo32khz(const WebRtc_Word32* In, WebRtc_Word32* Out,
                                    const WebRtc_Word32 K);












typedef struct
{
    WebRtc_Word32 S_48_48[16];
    WebRtc_Word32 S_48_32[8];
    WebRtc_Word32 S_32_16[8];
} WebRtcSpl_State48khzTo16khz;

void WebRtcSpl_Resample48khzTo16khz(const WebRtc_Word16* in, WebRtc_Word16* out,
                                    WebRtcSpl_State48khzTo16khz* state,
                                    WebRtc_Word32* tmpmem);

void WebRtcSpl_ResetResample48khzTo16khz(WebRtcSpl_State48khzTo16khz* state);

typedef struct
{
    WebRtc_Word32 S_16_32[8];
    WebRtc_Word32 S_32_24[8];
    WebRtc_Word32 S_24_48[8];
} WebRtcSpl_State16khzTo48khz;

void WebRtcSpl_Resample16khzTo48khz(const WebRtc_Word16* in, WebRtc_Word16* out,
                                    WebRtcSpl_State16khzTo48khz* state,
                                    WebRtc_Word32* tmpmem);

void WebRtcSpl_ResetResample16khzTo48khz(WebRtcSpl_State16khzTo48khz* state);

typedef struct
{
    WebRtc_Word32 S_48_24[8];
    WebRtc_Word32 S_24_24[16];
    WebRtc_Word32 S_24_16[8];
    WebRtc_Word32 S_16_8[8];
} WebRtcSpl_State48khzTo8khz;

void WebRtcSpl_Resample48khzTo8khz(const WebRtc_Word16* in, WebRtc_Word16* out,
                                   WebRtcSpl_State48khzTo8khz* state,
                                   WebRtc_Word32* tmpmem);

void WebRtcSpl_ResetResample48khzTo8khz(WebRtcSpl_State48khzTo8khz* state);

typedef struct
{
    WebRtc_Word32 S_8_16[8];
    WebRtc_Word32 S_16_12[8];
    WebRtc_Word32 S_12_24[8];
    WebRtc_Word32 S_24_48[8];
} WebRtcSpl_State8khzTo48khz;

void WebRtcSpl_Resample8khzTo48khz(const WebRtc_Word16* in, WebRtc_Word16* out,
                                   WebRtcSpl_State8khzTo48khz* state,
                                   WebRtc_Word32* tmpmem);

void WebRtcSpl_ResetResample8khzTo48khz(WebRtcSpl_State8khzTo48khz* state);








void WebRtcSpl_DownsampleBy2(const WebRtc_Word16* in, const WebRtc_Word16 len,
                             WebRtc_Word16* out, WebRtc_Word32* filtState);

void WebRtcSpl_UpsampleBy2(const WebRtc_Word16* in, WebRtc_Word16 len, WebRtc_Word16* out,
                           WebRtc_Word32* filtState);




void WebRtcSpl_AnalysisQMF(const WebRtc_Word16* in_data,
                           WebRtc_Word16* low_band,
                           WebRtc_Word16* high_band,
                           WebRtc_Word32* filter_state1,
                           WebRtc_Word32* filter_state2);
void WebRtcSpl_SynthesisQMF(const WebRtc_Word16* low_band,
                            const WebRtc_Word16* high_band,
                            WebRtc_Word16* out_data,
                            WebRtc_Word32* filter_state1,
                            WebRtc_Word32* filter_state2);

#ifdef __cplusplus
}
#endif 
#endif






























































































































































































































































































































































































































































































































































































































































































































































































