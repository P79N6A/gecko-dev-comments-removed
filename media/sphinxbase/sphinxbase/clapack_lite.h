
#ifndef __CLAPACK_LITE_H
#define __CLAPACK_LITE_H

#include "f2c.h"
 

#ifdef __cplusplus
extern "C" {
#endif
#if 0

}
#endif

 int sgemm_(char *transa, char *transb, integer *m, integer *
                            n, integer *k, real *alpha, real *a, integer *lda, real *b, integer *
                            ldb, real *beta, real *c__, integer *ldc);
 int sgemv_(char *trans, integer *m, integer *n, real *alpha,
                            real *a, integer *lda, real *x, integer *incx, real *beta, real *y,
                            integer *incy);
 int ssymm_(char *side, char *uplo, integer *m, integer *n,
                            real *alpha, real *a, integer *lda, real *b, integer *ldb, real *beta,
                            real *c__, integer *ldc);

 int sposv_(char *uplo, integer *n, integer *nrhs, real *a,
                            integer *lda, real *b, integer *ldb, integer *info);
 int spotrf_(char *uplo, integer *n, real *a, integer *lda,
                             integer *info);

#ifdef __cplusplus
}
#endif

 
#endif
