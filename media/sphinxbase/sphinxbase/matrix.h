













































#ifndef MATRIX_H
#define MATRIX_H











#ifdef __cplusplus
extern "C" {
#endif
#if 0

}
#endif


#include <sphinxbase/sphinxbase_export.h>
#include <sphinxbase/prim_type.h>









SPHINXBASE_EXPORT void norm_3d(float32 ***arr, uint32 d1, uint32 d2, uint32 d3);









SPHINXBASE_EXPORT void
accum_3d(float32 ***out, float32 ***in, uint32 d1, uint32 d2, uint32 d3);






SPHINXBASE_EXPORT void band_nz_1d(float32 *v, uint32 d1, float32 band);









SPHINXBASE_EXPORT void floor_nz_3d(float32 ***m, uint32 d1, uint32 d2, uint32 d3, float32 floor);







SPHINXBASE_EXPORT void floor_nz_1d(float32 *v, uint32 d1, float32 floor);













SPHINXBASE_EXPORT
float64 determinant(float32 **a, int32 len);











SPHINXBASE_EXPORT
int32 invert(float32 **out_ainv, float32 **a, int32 len);












SPHINXBASE_EXPORT
int32 solve(float32 **a, float32 *b,
            float32 *out_x, int32 n);









SPHINXBASE_EXPORT
void outerproduct(float32 **out_a, float32 *x, float32 *y, int32 len);








SPHINXBASE_EXPORT
void matrixmultiply(float32 **out_c, 
                    float32 **a,  float32 **b,
                    int32 n);







SPHINXBASE_EXPORT
void scalarmultiply(float32 **inout_a, float32 x, int32 n);







SPHINXBASE_EXPORT
void matrixadd(float32 **inout_a, float32 **b, int32 n);

#if 0
{ 
#endif
#ifdef __cplusplus
}
#endif

#endif 

