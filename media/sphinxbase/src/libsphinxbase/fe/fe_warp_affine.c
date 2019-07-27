
















































#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>

#ifdef _MSC_VER
#pragma warning (disable: 4996)
#endif

#include "sphinxbase/strfuncs.h"
#include "sphinxbase/err.h"

#include "fe_warp.h"
#include "fe_warp_affine.h"

#define N_PARAM		2
#define YES             1
#define NO              0





static float params[N_PARAM] = { 1.0f, 0.0f };
static int32 is_neutral = YES;
static char p_str[256] = "";
static float nyquist_frequency = 0.0f;


const char *
fe_warp_affine_doc()
{
    return "affine :== < w' = a * x + b >";
}

uint32
fe_warp_affine_id()
{
    return FE_WARP_ID_AFFINE;
}

uint32
fe_warp_affine_n_param()
{
    return N_PARAM;
}

void
fe_warp_affine_set_parameters(char const *param_str, float sampling_rate)
{
    char *tok;
    char *seps = " \t";
    char temp_param_str[256];
    int param_index = 0;

    nyquist_frequency = sampling_rate / 2;
    if (param_str == NULL) {
        is_neutral = YES;
        return;
    }
    
    if (strcmp(param_str, p_str) == 0) {
        return;
    }
    is_neutral = NO;
    strcpy(temp_param_str, param_str);
    memset(params, 0, N_PARAM * sizeof(float));
    strcpy(p_str, param_str);
    
    tok = strtok(temp_param_str, seps);
    while (tok != NULL) {
        params[param_index++] = (float) atof_c(tok);
        tok = strtok(NULL, seps);
        if (param_index >= N_PARAM) {
            break;
        }
    }
    if (tok != NULL) {
        E_INFO
            ("Affine warping takes up to two arguments, %s ignored.\n",
             tok);
    }
    if (params[0] == 0) {
        is_neutral = YES;
        E_INFO
            ("Affine warping cannot have slope zero, warping not applied.\n");
    }
}

float
fe_warp_affine_warped_to_unwarped(float nonlinear)
{
    if (is_neutral) {
        return nonlinear;
    }
    else {
        
        float temp = nonlinear - params[1];
        temp /= params[0];
        if (temp > nyquist_frequency) {
            E_WARN
                ("Warp factor %g results in frequency (%.1f) higher than Nyquist (%.1f)\n",
                 params[0], temp, nyquist_frequency);
        }
        return temp;
    }
}

float
fe_warp_affine_unwarped_to_warped(float linear)
{
    if (is_neutral) {
        return linear;
    }
    else {
        
        float temp = linear * params[0];
        temp += params[1];
        return temp;
    }
}

void
fe_warp_affine_print(const char *label)
{
    uint32 i;

    for (i = 0; i < N_PARAM; i++) {
        printf("%s[%04u]: %6.3f ", label, i, params[i]);
    }
    printf("\n");
}
