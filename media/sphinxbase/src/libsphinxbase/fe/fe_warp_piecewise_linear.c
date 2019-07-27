
























































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
#include "fe_warp_piecewise_linear.h"

#define N_PARAM		2
#define YES             1
#define NO              0





static float params[N_PARAM] = { 1.0f, 6800.0f };
static float final_piece[2];
static int32 is_neutral = YES;
static char p_str[256] = "";
static float nyquist_frequency = 0.0f;


const char *
fe_warp_piecewise_linear_doc()
{
    return "piecewise_linear :== < w' = a * w, w < F >";
}

uint32
fe_warp_piecewise_linear_id()
{
    return FE_WARP_ID_PIECEWISE_LINEAR;
}

uint32
fe_warp_piecewise_linear_n_param()
{
    return N_PARAM;
}

void
fe_warp_piecewise_linear_set_parameters(char const *param_str,
                                        float sampling_rate)
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
    memset(final_piece, 0, 2 * sizeof(float));
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
            ("Piecewise linear warping takes up to two arguments, %s ignored.\n",
             tok);
    }
    if (params[1] < sampling_rate) {
        




        if (params[1] == 0) {
            params[1] = sampling_rate * 0.85f;
        }
        final_piece[0] =
            (nyquist_frequency -
             params[0] * params[1]) / (nyquist_frequency - params[1]);
        final_piece[1] =
            nyquist_frequency * params[1] * (params[0] -
                                         1.0f) / (nyquist_frequency -
                                                  params[1]);
    }
    else {
        memset(final_piece, 0, 2 * sizeof(float));
    }
    if (params[0] == 0) {
        is_neutral = YES;
        E_INFO
            ("Piecewise linear warping cannot have slope zero, warping not applied.\n");
    }
}

float
fe_warp_piecewise_linear_warped_to_unwarped(float nonlinear)
{
    if (is_neutral) {
        return nonlinear;
    }
    else {
        
        float temp;
        if (nonlinear < params[0] * params[1]) {
            temp = nonlinear / params[0];
        }
        else {
            temp = nonlinear - final_piece[1];
            temp /= final_piece[0];
        }
        if (temp > nyquist_frequency) {
            E_WARN
                ("Warp factor %g results in frequency (%.1f) higher than Nyquist (%.1f)\n",
                 params[0], temp, nyquist_frequency);
        }
        return temp;
    }
}

float
fe_warp_piecewise_linear_unwarped_to_warped(float linear)
{
    if (is_neutral) {
        return linear;
    }
    else {
        float temp;
        
        if (linear < params[1]) {
            temp = linear * params[0];
        }
        else {
            temp = final_piece[0] * linear + final_piece[1];
        }
        return temp;
    }
}

void
fe_warp_piecewise_linear_print(const char *label)
{
    uint32 i;

    for (i = 0; i < N_PARAM; i++) {
        printf("%s[%04u]: %6.3f ", label, i, params[i]);
    }
    printf("\n");
}
