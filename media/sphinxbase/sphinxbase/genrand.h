
















































































































#ifndef _LIBUTIL_GENRAND_H_
#define _LIBUTIL_GENRAND_H_

#define S3_RAND_MAX_INT32 0x7fffffff
#include <stdio.h>


#include <sphinxbase/sphinxbase_export.h>










#ifdef __cplusplus
extern "C" {
#endif
#if 0

}
#endif





#define s3_rand_seed(s) genrand_seed(s);
#define s3_rand_int31()  genrand_int31()
#define s3_rand_real() genrand_real3()
#define s3_rand_res53()  genrand_res53()




SPHINXBASE_EXPORT
void genrand_seed(unsigned long s);




SPHINXBASE_EXPORT
long genrand_int31(void);




SPHINXBASE_EXPORT
double genrand_real3(void);




SPHINXBASE_EXPORT
double genrand_res53(void);

#ifdef __cplusplus
}
#endif

#endif



