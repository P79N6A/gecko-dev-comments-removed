










#ifndef VP9_ENCODER_X86_VP9_MCOMP_X86_H_
#define VP9_ENCODER_X86_VP9_MCOMP_X86_H_

#ifdef __cplusplus
extern "C" {
#endif

#if HAVE_SSE3
#if !CONFIG_RUNTIME_CPU_DETECT

#undef  vp9_search_full_search
#define vp9_search_full_search vp9_full_search_sadx3

#undef  vp9_search_refining_search
#define vp9_search_refining_search vp9_refining_search_sadx4

#undef  vp9_search_diamond_search
#define vp9_search_diamond_search vp9_diamond_search_sadx4

#endif
#endif

#if HAVE_SSE4_1
#if !CONFIG_RUNTIME_CPU_DETECT

#undef  vp9_search_full_search
#define vp9_search_full_search vp9_full_search_sadx8

#endif
#endif

#ifdef __cplusplus
}  
#endif

#endif

