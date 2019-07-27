








































#ifndef __PS_MLLR_H__
#define __PS_MLLR_H__


#include <sphinxbase/prim_type.h>
#include <sphinxbase/ngram_model.h>


#include <pocketsphinx_export.h>




typedef struct ps_mllr_s ps_mllr_t;




POCKETSPHINX_EXPORT
ps_mllr_t *ps_mllr_read(char const *file);




POCKETSPHINX_EXPORT
ps_mllr_t *ps_mllr_retain(ps_mllr_t *mllr);




POCKETSPHINX_EXPORT
int ps_mllr_free(ps_mllr_t *mllr);

#endif 
