









#ifndef WEBRTC_MODULES_AUDIO_PROCESSING_AEC_MAIN_SOURCE_AEC_RDFT_H_
#define WEBRTC_MODULES_AUDIO_PROCESSING_AEC_MAIN_SOURCE_AEC_RDFT_H_

#include "webrtc/modules/audio_processing/aec/aec_common.h"



#if defined(_MSC_VER) && _MSC_VER < 1500
#include <emmintrin.h>
static __inline __m128 _mm_castsi128_ps(__m128i a) { return *(__m128*)&a; }
static __inline __m128i _mm_castps_si128(__m128 a) { return *(__m128i*)&a; }
#endif


extern const float rdft_w[64];

extern const float rdft_wk3ri_first[16];
extern const float rdft_wk3ri_second[16];

extern ALIGN16_BEG const float ALIGN16_END rdft_wk1r[32];
extern ALIGN16_BEG const float ALIGN16_END rdft_wk2r[32];
extern ALIGN16_BEG const float ALIGN16_END rdft_wk3r[32];
extern ALIGN16_BEG const float ALIGN16_END rdft_wk1i[32];
extern ALIGN16_BEG const float ALIGN16_END rdft_wk2i[32];
extern ALIGN16_BEG const float ALIGN16_END rdft_wk3i[32];
extern ALIGN16_BEG const float ALIGN16_END cftmdl_wk1r[4];


typedef void (*rft_sub_128_t)(float* a);
extern rft_sub_128_t rftfsub_128;
extern rft_sub_128_t rftbsub_128;
extern rft_sub_128_t cft1st_128;
extern rft_sub_128_t cftmdl_128;
extern rft_sub_128_t cftfsub_128;
extern rft_sub_128_t cftbsub_128;
extern rft_sub_128_t bitrv2_128;


void aec_rdft_init(void);
void aec_rdft_init_sse2(void);
void aec_rdft_forward_128(float* a);
void aec_rdft_inverse_128(float* a);

#if defined(MIPS_FPU_LE)
void aec_rdft_init_mips(void);
#endif
#if defined(WEBRTC_DETECT_ARM_NEON) || defined(WEBRTC_ARCH_ARM_NEON)
void aec_rdft_init_neon(void);
#endif

#endif  
