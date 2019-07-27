



































#ifndef FE_WARP_H
#define FE_WARP_H

#include "fe_internal.h"

#ifdef __cplusplus
extern "C" {
#endif
#if 0

}
#endif

#define FE_WARP_ID_INVERSE_LINEAR	0
#define FE_WARP_ID_AFFINE	        1
#define FE_WARP_ID_PIECEWISE_LINEAR	2
#define FE_WARP_ID_EIDE_GISH		3
#define FE_WARP_ID_MAX		        2
#define FE_WARP_ID_NONE	       0xffffffff

typedef struct {
    void (*set_parameters)(char const *param_str, float sampling_rate);
    const char * (*doc)(void);
    uint32 (*id)(void);
    uint32 (*n_param)(void);
    float (*warped_to_unwarped)(float nonlinear);
    float (*unwarped_to_warped)(float linear);
    void (*print)(const char *label);
} fe_warp_conf_t;

int fe_warp_set(melfb_t *mel, const char *id_name);

uint32 fe_warp_id(melfb_t *mel);

const char * fe_warp_doc(melfb_t *mel);

void fe_warp_set_parameters(melfb_t *mel, char const *param_str, float sampling_rate);

uint32 fe_warp_n_param(melfb_t *mel);

float fe_warp_warped_to_unwarped(melfb_t *mel, float nonlinear);

float fe_warp_unwarped_to_warped(melfb_t *mel, float linear);

void fe_warp_print(melfb_t *mel, const char *label);

#define FE_WARP_NO_SIZE	0xffffffff

#ifdef __cplusplus
}
#endif


#endif 
