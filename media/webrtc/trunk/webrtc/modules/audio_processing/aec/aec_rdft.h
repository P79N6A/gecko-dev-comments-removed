









#ifndef WEBRTC_MODULES_AUDIO_PROCESSING_AEC_MAIN_SOURCE_AEC_RDFT_H_
#define WEBRTC_MODULES_AUDIO_PROCESSING_AEC_MAIN_SOURCE_AEC_RDFT_H_



#if defined(_MSC_VER) && _MSC_VER < 1500
#include <emmintrin.h>
static __inline __m128 _mm_castsi128_ps(__m128i a) { return *(__m128*)&a; }
static __inline __m128i _mm_castps_si128(__m128 a) { return *(__m128i*)&a; }
#endif

#ifdef _MSC_VER 
#define ALIGN16_BEG __declspec(align(16))
#define ALIGN16_END
#else 
#define ALIGN16_BEG
#define ALIGN16_END __attribute__((aligned(16)))
#endif


extern float rdft_w[64];

extern float rdft_wk3ri_first[32];
extern float rdft_wk3ri_second[32];

extern ALIGN16_BEG float ALIGN16_END rdft_wk1r[32];
extern ALIGN16_BEG float ALIGN16_END rdft_wk2r[32];
extern ALIGN16_BEG float ALIGN16_END rdft_wk3r[32];
extern ALIGN16_BEG float ALIGN16_END rdft_wk1i[32];
extern ALIGN16_BEG float ALIGN16_END rdft_wk2i[32];
extern ALIGN16_BEG float ALIGN16_END rdft_wk3i[32];
extern ALIGN16_BEG float ALIGN16_END cftmdl_wk1r[4];


typedef void (*rft_sub_128_t)(float* a);
extern rft_sub_128_t rftfsub_128;
extern rft_sub_128_t rftbsub_128;
extern rft_sub_128_t cft1st_128;
extern rft_sub_128_t cftmdl_128;


void aec_rdft_init(void);
void aec_rdft_init_sse2(void);
void aec_rdft_forward_128(float* a);
void aec_rdft_inverse_128(float* a);

#endif  
