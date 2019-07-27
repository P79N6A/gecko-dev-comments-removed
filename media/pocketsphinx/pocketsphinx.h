




































#ifndef __POCKETSPHINX_H__
#define __POCKETSPHINX_H__

#ifdef __cplusplus
extern "C" {
#endif
#if 0
}
#endif


#include <stdio.h>


#include <sphinxbase/cmd_ln.h>
#include <sphinxbase/logmath.h>
#include <sphinxbase/fe.h>
#include <sphinxbase/feat.h>


#include <pocketsphinx_export.h>
#include <cmdln_macro.h>
#include <ps_lattice.h>
#include <ps_mllr.h>




typedef struct ps_decoder_s ps_decoder_t;

#include <ps_search.h>




typedef struct ps_astar_s ps_nbest_t;




typedef struct ps_seg_s ps_seg_t;





POCKETSPHINX_EXPORT void
ps_default_search_args(cmd_ln_t *);











POCKETSPHINX_EXPORT
ps_decoder_t *ps_init(cmd_ln_t *config);


















POCKETSPHINX_EXPORT
int ps_reinit(ps_decoder_t *ps, cmd_ln_t *config);







POCKETSPHINX_EXPORT
arg_t const *ps_args(void);











POCKETSPHINX_EXPORT
ps_decoder_t *ps_retain(ps_decoder_t *ps);











POCKETSPHINX_EXPORT
int ps_free(ps_decoder_t *ps);









POCKETSPHINX_EXPORT
cmd_ln_t *ps_get_config(ps_decoder_t *ps);









POCKETSPHINX_EXPORT
logmath_t *ps_get_logmath(ps_decoder_t *ps);









POCKETSPHINX_EXPORT
fe_t *ps_get_fe(ps_decoder_t *ps);









POCKETSPHINX_EXPORT
feat_t *ps_get_feat(ps_decoder_t *ps);












POCKETSPHINX_EXPORT
ps_mllr_t *ps_update_mllr(ps_decoder_t *ps, ps_mllr_t *mllr);















POCKETSPHINX_EXPORT
int ps_load_dict(ps_decoder_t *ps, char const *dictfile,
                 char const *fdictfile, char const *format);










POCKETSPHINX_EXPORT
int ps_save_dict(ps_decoder_t *ps, char const *dictfile, char const *format);




















POCKETSPHINX_EXPORT
int ps_add_word(ps_decoder_t *ps,
                char const *word,
                char const *phones,
                int update);












POCKETSPHINX_EXPORT
char *ps_lookup_word(ps_decoder_t *ps, 
	             const char *word);















POCKETSPHINX_EXPORT
long ps_decode_raw(ps_decoder_t *ps, FILE *rawfh,
                   long maxsamps);








POCKETSPHINX_EXPORT
int ps_decode_senscr(ps_decoder_t *ps, FILE *senfh);








POCKETSPHINX_EXPORT
int ps_start_stream(ps_decoder_t *ps);











POCKETSPHINX_EXPORT
int ps_start_utt(ps_decoder_t *ps);














POCKETSPHINX_EXPORT
int ps_process_raw(ps_decoder_t *ps,
                   int16 const *data,
                   size_t n_samples,
                   int no_search,
                   int full_utt);














POCKETSPHINX_EXPORT
int ps_process_cep(ps_decoder_t *ps,
                   mfcc_t **data,
                   int n_frames,
                   int no_search,
                   int full_utt);














POCKETSPHINX_EXPORT
int ps_get_n_frames(ps_decoder_t *ps);







POCKETSPHINX_EXPORT
int ps_end_utt(ps_decoder_t *ps);









POCKETSPHINX_EXPORT
char const *ps_get_hyp(ps_decoder_t *ps, int32 *out_best_score);









POCKETSPHINX_EXPORT
char const *ps_get_hyp_final(ps_decoder_t *ps, int32 *out_is_final);














POCKETSPHINX_EXPORT
int32 ps_get_prob(ps_decoder_t *ps);














POCKETSPHINX_EXPORT
ps_lattice_t *ps_get_lattice(ps_decoder_t *ps);









POCKETSPHINX_EXPORT
ps_seg_t *ps_seg_iter(ps_decoder_t *ps, int32 *out_best_score);








POCKETSPHINX_EXPORT
ps_seg_t *ps_seg_next(ps_seg_t *seg);








POCKETSPHINX_EXPORT
char const *ps_seg_word(ps_seg_t *seg);












POCKETSPHINX_EXPORT
void ps_seg_frames(ps_seg_t *seg, int *out_sf, int *out_ef);























POCKETSPHINX_EXPORT
int32 ps_seg_prob(ps_seg_t *seg, int32 *out_ascr, int32 *out_lscr, int32 *out_lback);




POCKETSPHINX_EXPORT
void ps_seg_free(ps_seg_t *seg);















POCKETSPHINX_EXPORT
ps_nbest_t *ps_nbest(ps_decoder_t *ps, int sf, int ef,
                     char const *ctx1, char const *ctx2);








POCKETSPHINX_EXPORT 
ps_nbest_t *ps_nbest_next(ps_nbest_t *nbest);








POCKETSPHINX_EXPORT
char const *ps_nbest_hyp(ps_nbest_t *nbest, int32 *out_score);








POCKETSPHINX_EXPORT
ps_seg_t *ps_nbest_seg(ps_nbest_t *nbest, int32 *out_score);






POCKETSPHINX_EXPORT
void ps_nbest_free(ps_nbest_t *nbest);









POCKETSPHINX_EXPORT
void ps_get_utt_time(ps_decoder_t *ps, double *out_nspeech,
                     double *out_ncpu, double *out_nwall);









POCKETSPHINX_EXPORT
void ps_get_all_time(ps_decoder_t *ps, double *out_nspeech,
                     double *out_ncpu, double *out_nwall);







POCKETSPHINX_EXPORT
uint8 ps_get_in_speech(ps_decoder_t *ps);









POCKETSPHINX_EXPORT
void ps_set_rawdata_size(ps_decoder_t *ps, int32 size);










POCKETSPHINX_EXPORT
void ps_get_rawdata(ps_decoder_t *ps, int16 **buffer, int32 *size);














#ifdef __cplusplus
} 
#endif

#endif
