

































































#ifndef __PS_SEARCH_H__
#define __PS_SEARCH_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <sphinxbase/fsg_model.h>
#include <sphinxbase/ngram_model.h>




typedef struct ps_search_iter_s ps_search_iter_t;










POCKETSPHINX_EXPORT
int ps_set_search(ps_decoder_t *ps, const char *name);






POCKETSPHINX_EXPORT 
const char* ps_get_search(ps_decoder_t *ps);











POCKETSPHINX_EXPORT
int ps_unset_search(ps_decoder_t *ps, const char *name);






POCKETSPHINX_EXPORT
ps_search_iter_t *ps_search_iter(ps_decoder_t *ps);








POCKETSPHINX_EXPORT
ps_search_iter_t *ps_search_iter_next(ps_search_iter_t *itor);






POCKETSPHINX_EXPORT
const char* ps_search_iter_val(ps_search_iter_t *itor);






POCKETSPHINX_EXPORT
void ps_search_iter_free(ps_search_iter_t *itor);








POCKETSPHINX_EXPORT
const char* ps_search_iter_val(ps_search_iter_t *itor);













POCKETSPHINX_EXPORT 
ngram_model_t *ps_get_lm(ps_decoder_t *ps, const char *name);








 
POCKETSPHINX_EXPORT
int ps_set_lm(ps_decoder_t *ps, const char *name, ngram_model_t *lm);








POCKETSPHINX_EXPORT
int ps_set_lm_file(ps_decoder_t *ps, const char *name, const char *path);










POCKETSPHINX_EXPORT
fsg_model_t *ps_get_fsg(ps_decoder_t *ps, const char *name);









POCKETSPHINX_EXPORT
int ps_set_fsg(ps_decoder_t *ps, const char *name, fsg_model_t *fsg);








POCKETSPHINX_EXPORT
int ps_set_jsgf_file(ps_decoder_t *ps, const char *name, const char *path);








POCKETSPHINX_EXPORT
int ps_set_jsgf_string(ps_decoder_t *ps, const char *name, const char *jsgf_string);









POCKETSPHINX_EXPORT 
const char* ps_get_kws(ps_decoder_t *ps, const char *name);









POCKETSPHINX_EXPORT 
int ps_set_kws(ps_decoder_t *ps, const char *name, const char *keyfile);









POCKETSPHINX_EXPORT 
int ps_set_keyphrase(ps_decoder_t *ps, const char *name, const char *keyphrase);








 
POCKETSPHINX_EXPORT
int ps_set_allphone(ps_decoder_t *ps, const char *name, ngram_model_t *lm);








POCKETSPHINX_EXPORT
int ps_set_allphone_file(ps_decoder_t *ps, const char *name, const char *path);

#ifdef __cplusplus
}
#endif

#endif
