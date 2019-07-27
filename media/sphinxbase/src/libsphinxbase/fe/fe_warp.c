













































#include "fe_warp_inverse_linear.h"
#include "fe_warp_affine.h"
#include "fe_warp_piecewise_linear.h"
#include "fe_warp.h"

#include "sphinxbase/err.h"

#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <stdlib.h>




static char *__name2id[] = {
    "inverse",
    "linear",
    "piecewise",
    NULL
};

static char *name2id[] = {
    "inverse_linear",
    "affine",
    "piecewise_linear",
    NULL
};

static fe_warp_conf_t fe_warp_conf[FE_WARP_ID_MAX + 1] = {
    {fe_warp_inverse_linear_set_parameters,
     fe_warp_inverse_linear_doc,
     fe_warp_inverse_linear_id,
     fe_warp_inverse_linear_n_param,
     fe_warp_inverse_linear_warped_to_unwarped,
     fe_warp_inverse_linear_unwarped_to_warped,
     fe_warp_inverse_linear_print},     
    {fe_warp_affine_set_parameters,
     fe_warp_affine_doc,
     fe_warp_affine_id,
     fe_warp_affine_n_param,
     fe_warp_affine_warped_to_unwarped,
     fe_warp_affine_unwarped_to_warped,
     fe_warp_affine_print},     
    {fe_warp_piecewise_linear_set_parameters,
     fe_warp_piecewise_linear_doc,
     fe_warp_piecewise_linear_id,
     fe_warp_piecewise_linear_n_param,
     fe_warp_piecewise_linear_warped_to_unwarped,
     fe_warp_piecewise_linear_unwarped_to_warped,
     fe_warp_piecewise_linear_print},   
};

int
fe_warp_set(melfb_t *mel, const char *id_name)
{
    uint32 i;

    for (i = 0; name2id[i]; i++) {
        if (strcmp(id_name, name2id[i]) == 0) {
            mel->warp_id = i;
            break;
        }
    }

    if (name2id[i] == NULL) {
        for (i = 0; __name2id[i]; i++) {
            if (strcmp(id_name, __name2id[i]) == 0) {
                mel->warp_id = i;
                break;
            }
        }
        if (__name2id[i] == NULL) {
            E_ERROR("Unimplemented warping function %s\n", id_name);
            E_ERROR("Implemented functions are:\n");
            for (i = 0; name2id[i]; i++) {
                fprintf(stderr, "\t%s\n", name2id[i]);
            }
            mel->warp_id = FE_WARP_ID_NONE;

            return FE_START_ERROR;
        }
    }

    return FE_SUCCESS;
}

void
fe_warp_set_parameters(melfb_t *mel, char const *param_str, float sampling_rate)
{
    if (mel->warp_id <= FE_WARP_ID_MAX) {
        fe_warp_conf[mel->warp_id].set_parameters(param_str, sampling_rate);
    }
    else if (mel->warp_id == FE_WARP_ID_NONE) {
        E_FATAL("feat module must be configured w/ a valid ID\n");
    }
    else {
        E_FATAL
            ("fe_warp module misconfigured with invalid fe_warp_id %u\n",
             mel->warp_id);
    }
}

const char *
fe_warp_doc(melfb_t *mel)
{
    if (mel->warp_id <= FE_WARP_ID_MAX) {
        return fe_warp_conf[mel->warp_id].doc();
    }
    else if (mel->warp_id == FE_WARP_ID_NONE) {
        E_FATAL("fe_warp module must be configured w/ a valid ID\n");
    }
    else {
        E_FATAL
            ("fe_warp module misconfigured with invalid fe_warp_id %u\n",
             mel->warp_id);
    }

    return NULL;
}

uint32
fe_warp_id(melfb_t *mel)
{
    if (mel->warp_id <= FE_WARP_ID_MAX) {
        assert(mel->warp_id == fe_warp_conf[mel->warp_id].id());
        return mel->warp_id;
    }
    else if (mel->warp_id != FE_WARP_ID_NONE) {
        E_FATAL
            ("fe_warp module misconfigured with invalid fe_warp_id %u\n",
             mel->warp_id);
    }

    return FE_WARP_ID_NONE;
}

uint32
fe_warp_n_param(melfb_t *mel)
{
    if (mel->warp_id <= FE_WARP_ID_MAX) {
        return fe_warp_conf[mel->warp_id].n_param();
    }
    else if (mel->warp_id == FE_WARP_ID_NONE) {
        E_FATAL("fe_warp module must be configured w/ a valid ID\n");
    }
    else {
        E_FATAL
            ("fe_warp module misconfigured with invalid fe_warp_id %u\n",
             mel->warp_id);
    }

    return 0;
}

float
fe_warp_warped_to_unwarped(melfb_t *mel, float nonlinear)
{
    if (mel->warp_id <= FE_WARP_ID_MAX) {
        return fe_warp_conf[mel->warp_id].warped_to_unwarped(nonlinear);
    }
    else if (mel->warp_id == FE_WARP_ID_NONE) {
        E_FATAL("fe_warp module must be configured w/ a valid ID\n");
    }
    else {
        E_FATAL
            ("fe_warp module misconfigured with invalid fe_warp_id %u\n",
             mel->warp_id);
    }

    return 0;
}

float
fe_warp_unwarped_to_warped(melfb_t *mel,float linear)
{
    if (mel->warp_id <= FE_WARP_ID_MAX) {
        return fe_warp_conf[mel->warp_id].unwarped_to_warped(linear);
    }
    else if (mel->warp_id == FE_WARP_ID_NONE) {
        E_FATAL("fe_warp module must be configured w/ a valid ID\n");
    }
    else {
        E_FATAL
            ("fe_warp module misconfigured with invalid fe_warp_id %u\n",
             mel->warp_id);
    }

    return 0;
}

void
fe_warp_print(melfb_t *mel, const char *label)
{
    if (mel->warp_id <= FE_WARP_ID_MAX) {
        fe_warp_conf[mel->warp_id].print(label);
    }
    else if (mel->warp_id == FE_WARP_ID_NONE) {
        E_FATAL("fe_warp module must be configured w/ a valid ID\n");
    }
    else {
        E_FATAL
            ("fe_warp module misconfigured with invalid fe_warp_id %u\n",
             mel->warp_id);
    }
}
