










#ifndef VP9_ENCODER_VP9_AQ_COMPLEXITY_H_
#define VP9_ENCODER_VP9_AQ_COMPLEXITY_H_

#ifdef __cplusplus
extern "C" {
#endif

struct VP9_COMP;


void vp9_select_in_frame_q_segment(struct VP9_COMP *cpi, int mi_row, int mi_col,
                                   int output_enabled, int projected_rate);




void vp9_setup_in_frame_q_adj(struct VP9_COMP *cpi);

#ifdef __cplusplus
}  
#endif

#endif
