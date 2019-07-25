










#ifndef __INC_VP8_TEMPORAL_FILTER_X86_H
#define __INC_VP8_TEMPORAL_FILTER_X86_H

#if HAVE_SSE2
extern prototype_apply(vp8_temporal_filter_apply_sse2);

#if !CONFIG_RUNTIME_CPU_DETECT

#undef  vp8_temporal_filter_apply
#define vp8_temporal_filter_apply vp8_temporal_filter_apply_sse2

#endif

#endif

#endif 
