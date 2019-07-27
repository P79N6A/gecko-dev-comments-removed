










#ifndef VP9_COMMON_VP9_ALLOCCOMMON_H_
#define VP9_COMMON_VP9_ALLOCCOMMON_H_

#define INVALID_IDX -1  // Invalid buffer index.

#ifdef __cplusplus
extern "C" {
#endif

struct VP9Common;
struct BufferPool;

void vp9_remove_common(struct VP9Common *cm);

int vp9_alloc_context_buffers(struct VP9Common *cm, int width, int height);
void vp9_init_context_buffers(struct VP9Common *cm);
void vp9_free_context_buffers(struct VP9Common *cm);

void vp9_free_ref_frame_buffers(struct BufferPool *pool);
void vp9_free_postproc_buffers(struct VP9Common *cm);

int vp9_alloc_state_buffers(struct VP9Common *cm, int width, int height);
void vp9_free_state_buffers(struct VP9Common *cm);

void vp9_set_mb_mi(struct VP9Common *cm, int width, int height);

void vp9_swap_current_and_last_seg_map(struct VP9Common *cm);

#ifdef __cplusplus
}  
#endif

#endif
