








































#ifndef __YIN_H__
#define __YIN_H__

#ifdef __cplusplus
extern "C"
#endif
#if 0
} 
#endif


#include <sphinxbase/sphinxbase_export.h>
#include <sphinxbase/prim_type.h>




typedef struct yin_s yin_t;




SPHINXBASE_EXPORT
yin_t *yin_init(int frame_size, float search_threshold,
                float search_range, int smooth_window);




SPHINXBASE_EXPORT
void yin_free(yin_t *pe);




SPHINXBASE_EXPORT
void yin_start(yin_t *pe);




SPHINXBASE_EXPORT
void yin_end(yin_t *pe);








SPHINXBASE_EXPORT
void yin_store(yin_t *pe, int16 const *frame);








SPHINXBASE_EXPORT
void yin_write(yin_t *pe, int16 const *frame);







SPHINXBASE_EXPORT
void yin_write_stored(yin_t *pe);














SPHINXBASE_EXPORT
int yin_read(yin_t *pe, uint16 *out_period, float *out_bestdiff);

#ifdef __cplusplus
}
#endif

#endif 

