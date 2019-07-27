



































#ifndef FE_WARP_AFFINE_H
#define FE_WARP_AFFINE_H

#include "sphinxbase/fe.h"


#ifdef __cplusplus
extern "C" {
#endif
#if 0

}
#endif

const char *
fe_warp_affine_doc(void);

uint32
fe_warp_affine_id(void);

uint32
fe_warp_affine_n_param(void);

void
fe_warp_affine_set_parameters(char const *param_str, float sampling_rate);

float
fe_warp_affine_warped_to_unwarped(float nonlinear);

float
fe_warp_affine_unwarped_to_warped(float linear);

void
fe_warp_affine_print(const char *label);

#ifdef __cplusplus
}
#endif

#endif 
