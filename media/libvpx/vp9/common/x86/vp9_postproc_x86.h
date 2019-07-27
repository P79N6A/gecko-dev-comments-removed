










#ifndef VP9_COMMON_X86_VP9_POSTPROC_X86_H_
#define VP9_COMMON_X86_VP9_POSTPROC_X86_H_

#ifdef __cplusplus
extern "C" {
#endif








#if HAVE_MMX
extern prototype_postproc_inplace(vp9_mbpost_proc_down_mmx);
extern prototype_postproc(vp9_post_proc_down_and_across_mmx);
extern prototype_postproc_addnoise(vp9_plane_add_noise_mmx);

#if !CONFIG_RUNTIME_CPU_DETECT
#undef  vp9_postproc_down
#define vp9_postproc_down vp9_mbpost_proc_down_mmx

#undef  vp9_postproc_downacross
#define vp9_postproc_downacross vp9_post_proc_down_and_across_mmx

#undef  vp9_postproc_addnoise
#define vp9_postproc_addnoise vp9_plane_add_noise_mmx

#endif
#endif


#if HAVE_SSE2
extern prototype_postproc_inplace(vp9_mbpost_proc_down_xmm);
extern prototype_postproc_inplace(vp9_mbpost_proc_across_ip_xmm);
extern prototype_postproc(vp9_post_proc_down_and_across_xmm);
extern prototype_postproc_addnoise(vp9_plane_add_noise_wmt);

#if !CONFIG_RUNTIME_CPU_DETECT
#undef  vp9_postproc_down
#define vp9_postproc_down vp9_mbpost_proc_down_xmm

#undef  vp9_postproc_across
#define vp9_postproc_across vp9_mbpost_proc_across_ip_xmm

#undef  vp9_postproc_downacross
#define vp9_postproc_downacross vp9_post_proc_down_and_across_xmm

#undef  vp9_postproc_addnoise
#define vp9_postproc_addnoise vp9_plane_add_noise_wmt


#endif
#endif

#ifdef __cplusplus
}  
#endif

#endif
