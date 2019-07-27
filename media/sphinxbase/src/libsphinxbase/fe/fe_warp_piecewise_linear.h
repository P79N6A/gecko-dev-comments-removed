



































#ifndef FE_WARP_PIECEWIDE_LINEAR_H
#define FE_WARP_PIECEWIDE_LINEAR_H

#include "sphinxbase/fe.h"


#ifdef __cplusplus
extern "C" {
#endif
#if 0

}
#endif

const char *
fe_warp_piecewise_linear_doc(void);

uint32
fe_warp_piecewise_linear_id(void);

uint32
fe_warp_piecewise_linear_n_param(void);

void
fe_warp_piecewise_linear_set_parameters(char const *param_str, float sampling_rate);

float
fe_warp_piecewise_linear_warped_to_unwarped(float nonlinear);

float
fe_warp_piecewise_linear_unwarped_to_warped(float linear);

void
fe_warp_piecewise_linear_print(const char *label);

#ifdef __cplusplus
}
#endif


#endif 
