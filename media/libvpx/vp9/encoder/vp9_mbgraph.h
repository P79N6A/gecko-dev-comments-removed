









#ifndef VP9_ENCODER_VP9_MBGRAPH_H_
#define VP9_ENCODER_VP9_MBGRAPH_H_

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
  struct {
    int err;
    union {
      int_mv mv;
      MB_PREDICTION_MODE mode;
    } m;
  } ref[MAX_REF_FRAMES];
} MBGRAPH_MB_STATS;

typedef struct {
  MBGRAPH_MB_STATS *mb_stats;
} MBGRAPH_FRAME_STATS;

struct VP9_COMP;

void vp9_update_mbgraph_stats(struct VP9_COMP *cpi);

#ifdef __cplusplus
}  
#endif

#endif
