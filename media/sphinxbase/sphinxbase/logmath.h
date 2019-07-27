






































































#ifndef __LOGMATH_H__
#define __LOGMATH_H__

#include <sphinxbase/sphinxbase_export.h>
#include <sphinxbase/prim_type.h>
#include <sphinxbase/cmd_ln.h>


#ifdef __cplusplus
extern "C" {
#endif
#if 0

}
#endif






typedef struct logadd_s logadd_t;
struct logadd_s {
    
    void *table;
    
    uint32 table_size;
    
    uint8 width;
    
    int8 shift;
};




typedef struct logmath_s logmath_t;




#define LOGMATH_TABLE(lm) ((logadd_t *)lm)








SPHINXBASE_EXPORT
logmath_t *logmath_init(float64 base, int shift, int use_table);




SPHINXBASE_EXPORT
logmath_t *logmath_read(const char *filename);




SPHINXBASE_EXPORT
int32 logmath_write(logmath_t *lmath, const char *filename);




SPHINXBASE_EXPORT
int32 logmath_get_table_shape(logmath_t *lmath, uint32 *out_size,
                              uint32 *out_width, uint32 *out_shift);




SPHINXBASE_EXPORT
float64 logmath_get_base(logmath_t *lmath);




SPHINXBASE_EXPORT
int logmath_get_zero(logmath_t *lmath);




SPHINXBASE_EXPORT
int logmath_get_width(logmath_t *lmath);




SPHINXBASE_EXPORT
int logmath_get_shift(logmath_t *lmath);






SPHINXBASE_EXPORT
logmath_t *logmath_retain(logmath_t *lmath);






SPHINXBASE_EXPORT
int logmath_free(logmath_t *lmath);




SPHINXBASE_EXPORT
int logmath_add_exact(logmath_t *lmath, int logb_p, int logb_q);




SPHINXBASE_EXPORT
int logmath_add(logmath_t *lmath, int logb_p, int logb_q);




SPHINXBASE_EXPORT
int logmath_log(logmath_t *lmath, float64 p);




SPHINXBASE_EXPORT
float64 logmath_exp(logmath_t *lmath, int logb_p);




SPHINXBASE_EXPORT
int logmath_ln_to_log(logmath_t *lmath, float64 log_p);




SPHINXBASE_EXPORT
float64 logmath_log_to_ln(logmath_t *lmath, int logb_p);




SPHINXBASE_EXPORT
int logmath_log10_to_log(logmath_t *lmath, float64 log_p);




SPHINXBASE_EXPORT
float64 logmath_log_to_log10(logmath_t *lmath, int logb_p);

#ifdef __cplusplus
}
#endif


#endif
