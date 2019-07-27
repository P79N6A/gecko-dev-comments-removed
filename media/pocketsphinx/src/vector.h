















































#ifndef __VECTOR_H__
#define __VECTOR_H__


#include <stdio.h>


#include <sphinxbase/prim_type.h>

typedef float32 *vector_t;








void vector_floor(vector_t v, int32 dim, float64 f);



void vector_nz_floor(vector_t v, int32 dim, float64 f);






float64 vector_sum_norm(vector_t v, int32 dim);



void vector_print(FILE *fp, vector_t v, int32 dim);



int32 vector_is_zero (float32 *vec,	
		      int32 len);	

#endif  
