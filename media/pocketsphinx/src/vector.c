



































































#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <math.h>


#include <sphinxbase/err.h>
#include <sphinxbase/ckd_alloc.h>
#include <sphinxbase/bitvec.h>


#include "vector.h"

#if defined(_WIN32)
#define srandom	srand
#define random	rand
#endif


float64
vector_sum_norm(float32 * vec, int32 len)
{
    float64 sum, f;
    int32 i;

    sum = 0.0;
    for (i = 0; i < len; i++)
        sum += vec[i];

    if (sum != 0.0) {
        f = 1.0 / sum;
        for (i = 0; i < len; i++)
            vec[i] *= f;
    }

    return sum;
}


void
vector_floor(float32 * vec, int32 len, float64 flr)
{
    int32 i;

    for (i = 0; i < len; i++)
        if (vec[i] < flr)
            vec[i] = (float32) flr;
}


void
vector_nz_floor(float32 * vec, int32 len, float64 flr)
{
    int32 i;

    for (i = 0; i < len; i++)
        if ((vec[i] != 0.0) && (vec[i] < flr))
            vec[i] = (float32) flr;
}


void
vector_print(FILE * fp, vector_t v, int32 dim)
{
    int32 i;

    for (i = 0; i < dim; i++)
        fprintf(fp, " %11.4e", v[i]);
    fprintf(fp, "\n");
    fflush(fp);
}


int32
vector_is_zero(float32 * vec, int32 len)
{
    int32 i;

    for (i = 0; (i < len) && (vec[i] == 0.0); i++);
    return (i == len);          
}
