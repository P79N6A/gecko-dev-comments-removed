









































#ifndef __NGRAM_MODEL_H__
#define __NGRAM_MODEL_H__

#include <stdarg.h>


#include <sphinxbase/sphinxbase_export.h>
#include <sphinxbase/prim_type.h>
#include <sphinxbase/cmd_ln.h>
#include <sphinxbase/logmath.h>
#include <sphinxbase/mmio.h>

#ifdef __cplusplus
extern "C" {
#endif
#if 0

}
#endif




typedef struct ngram_model_s ngram_model_t;




typedef struct ngram_class_s ngram_class_t;




typedef enum ngram_file_type_e {
    NGRAM_INVALID = -1, 
    NGRAM_AUTO,  
    NGRAM_ARPA,  
    NGRAM_DMP,   
    NGRAM_DMP32, 
} ngram_file_type_t;

#define NGRAM_INVALID_WID -1 /**< Impossible word ID */





















SPHINXBASE_EXPORT
ngram_model_t *ngram_model_read(cmd_ln_t *config,
				const char *file_name,
                                ngram_file_type_t file_type,
				logmath_t *lmath);






SPHINXBASE_EXPORT
int ngram_model_write(ngram_model_t *model, const char *file_name,
		      ngram_file_type_t format);






SPHINXBASE_EXPORT
ngram_file_type_t ngram_file_name_to_type(const char *file_name);






SPHINXBASE_EXPORT
ngram_file_type_t ngram_str_to_type(const char *str_name);







SPHINXBASE_EXPORT
char const *ngram_type_to_str(int type);






SPHINXBASE_EXPORT
ngram_model_t *ngram_model_retain(ngram_model_t *model);






SPHINXBASE_EXPORT
int ngram_model_free(ngram_model_t *model);




typedef enum ngram_case_e {
    NGRAM_UPPER,
    NGRAM_LOWER
} ngram_case_t;







SPHINXBASE_EXPORT
int ngram_model_casefold(ngram_model_t *model, int kase);












SPHINXBASE_EXPORT
int ngram_model_apply_weights(ngram_model_t *model,
                              float32 lw, float32 wip, float32 uw);









SPHINXBASE_EXPORT
float32 ngram_model_get_weights(ngram_model_t *model, int32 *out_log_wip,
                                int32 *out_log_uw);

































SPHINXBASE_EXPORT
int32 ngram_score(ngram_model_t *model, const char *word, ...);




SPHINXBASE_EXPORT
int32 ngram_tg_score(ngram_model_t *model,
                     int32 w3, int32 w2, int32 w1,
                     int32 *n_used);




SPHINXBASE_EXPORT
int32 ngram_bg_score(ngram_model_t *model,
                     int32 w2, int32 w1,
                     int32 *n_used);




SPHINXBASE_EXPORT
int32 ngram_ng_score(ngram_model_t *model, int32 wid, int32 *history,
                     int32 n_hist, int32 *n_used);











SPHINXBASE_EXPORT
int32 ngram_probv(ngram_model_t *model, const char *word, ...);











SPHINXBASE_EXPORT
int32 ngram_prob(ngram_model_t *model, const char *const *words, int32 n);







SPHINXBASE_EXPORT
int32 ngram_ng_prob(ngram_model_t *model, int32 wid, int32 *history,
                    int32 n_hist, int32 *n_used);












SPHINXBASE_EXPORT
int32 ngram_score_to_prob(ngram_model_t *model, int32 score);




SPHINXBASE_EXPORT
int32 ngram_wid(ngram_model_t *model, const char *word);




SPHINXBASE_EXPORT
const char *ngram_word(ngram_model_t *model, int32 wid);














SPHINXBASE_EXPORT
int32 ngram_unknown_wid(ngram_model_t *model);




SPHINXBASE_EXPORT
int32 ngram_zero(ngram_model_t *model);




SPHINXBASE_EXPORT
int32 ngram_model_get_size(ngram_model_t *model);




SPHINXBASE_EXPORT
int32 const *ngram_model_get_counts(ngram_model_t *model);




typedef struct ngram_iter_s ngram_iter_t;









SPHINXBASE_EXPORT
ngram_iter_t *ngram_model_mgrams(ngram_model_t *model, int m);




SPHINXBASE_EXPORT
ngram_iter_t *ngram_iter(ngram_model_t *model, const char *word, ...);




SPHINXBASE_EXPORT
ngram_iter_t *ngram_ng_iter(ngram_model_t *model, int32 wid, int32 *history, int32 n_hist);









SPHINXBASE_EXPORT
int32 const *ngram_iter_get(ngram_iter_t *itor,
                            int32 *out_score,
                            int32 *out_bowt);






SPHINXBASE_EXPORT
ngram_iter_t *ngram_iter_successors(ngram_iter_t *itor);




SPHINXBASE_EXPORT
ngram_iter_t *ngram_iter_next(ngram_iter_t *itor);




SPHINXBASE_EXPORT
void ngram_iter_free(ngram_iter_t *itor);













SPHINXBASE_EXPORT
int32 ngram_model_add_word(ngram_model_t *model,
                           const char *word, float32 weight);














SPHINXBASE_EXPORT
int32 ngram_model_read_classdef(ngram_model_t *model,
                                const char *file_name);









SPHINXBASE_EXPORT
int32 ngram_model_add_class(ngram_model_t *model,
                            const char *classname,
                            float32 classweight,
                            char **words,
                            const float32 *weights,
                            int32 n_words);










SPHINXBASE_EXPORT
int32 ngram_model_add_class_word(ngram_model_t *model,
                                 const char *classname,
                                 const char *word,
                                 float32 weight);

























SPHINXBASE_EXPORT
ngram_model_t *ngram_model_set_init(cmd_ln_t *config,
                                    ngram_model_t **models,
                                    char **names,
                                    const float32 *weights,
                                    int32 n_models);































SPHINXBASE_EXPORT
ngram_model_t *ngram_model_set_read(cmd_ln_t *config,
                                    const char *lmctlfile,
                                    logmath_t *lmath);




SPHINXBASE_EXPORT
int32 ngram_model_set_count(ngram_model_t *set);




typedef struct ngram_model_set_iter_s ngram_model_set_iter_t;






SPHINXBASE_EXPORT
ngram_model_set_iter_t *ngram_model_set_iter(ngram_model_t *set);






SPHINXBASE_EXPORT
ngram_model_set_iter_t *ngram_model_set_iter_next(ngram_model_set_iter_t *itor);




SPHINXBASE_EXPORT
void ngram_model_set_iter_free(ngram_model_set_iter_t *itor);








SPHINXBASE_EXPORT
ngram_model_t *ngram_model_set_iter_model(ngram_model_set_iter_t *itor,
                                          char const **lmname);







SPHINXBASE_EXPORT
ngram_model_t *ngram_model_set_select(ngram_model_t *set,
                                      const char *name);







SPHINXBASE_EXPORT
ngram_model_t *ngram_model_set_lookup(ngram_model_t *set,
                                      const char *name);




SPHINXBASE_EXPORT
const char *ngram_model_set_current(ngram_model_t *set);








SPHINXBASE_EXPORT
ngram_model_t *ngram_model_set_interp(ngram_model_t *set,
                                      const char **names,
                                      const float32 *weights);













SPHINXBASE_EXPORT
ngram_model_t *ngram_model_set_add(ngram_model_t *set,
                                   ngram_model_t *model,
                                   const char *name,
                                   float32 weight,
                                   int reuse_widmap);









SPHINXBASE_EXPORT
ngram_model_t *ngram_model_set_remove(ngram_model_t *set,
                                      const char *name,
                                      int reuse_widmap);




SPHINXBASE_EXPORT
void ngram_model_set_map_words(ngram_model_t *set,
                               const char **words,
                               int32 n_words);








SPHINXBASE_EXPORT
int32 ngram_model_set_current_wid(ngram_model_t *set,
                                  int32 set_wid);










SPHINXBASE_EXPORT
int32 ngram_model_set_known_wid(ngram_model_t *set, int32 set_wid);








SPHINXBASE_EXPORT
void ngram_model_flush(ngram_model_t *lm);

#ifdef __cplusplus
}
#endif


#endif
