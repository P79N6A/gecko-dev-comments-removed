










#ifndef VP9_ENCODER_VP9_SUBEXP_H_
#define VP9_ENCODER_VP9_SUBEXP_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "vp9/common/vp9_prob.h"

struct vp9_writer;

void vp9_write_prob_diff_update(struct vp9_writer *w,
                                vp9_prob newp, vp9_prob oldp);

void vp9_cond_prob_diff_update(struct vp9_writer *w, vp9_prob *oldp,
                               const unsigned int ct[2]);

int vp9_prob_diff_update_savings_search(const unsigned int *ct,
                                        vp9_prob oldp, vp9_prob *bestp,
                                        vp9_prob upd);


int vp9_prob_diff_update_savings_search_model(const unsigned int *ct,
                                              const vp9_prob *oldp,
                                              vp9_prob *bestp,
                                              vp9_prob upd,
                                              int stepsize);

#ifdef __cplusplus
}  
#endif

#endif
