








































#ifndef __PS_LATTICE_H__
#define __PS_LATTICE_H__


#include <sphinxbase/prim_type.h>
#include <sphinxbase/ngram_model.h>


#include <pocketsphinx_export.h>




typedef struct ps_lattice_s ps_lattice_t;







typedef struct ps_latnode_s ps_latnode_t;




typedef struct ps_latnode_s ps_latnode_iter_t; 







typedef struct ps_latlink_s ps_latlink_t;




typedef struct latlink_list_s ps_latlink_iter_t;


struct ps_decoder_s;








POCKETSPHINX_EXPORT
ps_lattice_t *ps_lattice_read(struct ps_decoder_s *ps,
                              char const *file);










POCKETSPHINX_EXPORT
ps_lattice_t *ps_lattice_retain(ps_lattice_t *dag);






POCKETSPHINX_EXPORT
int ps_lattice_free(ps_lattice_t *dag);






POCKETSPHINX_EXPORT
int ps_lattice_write(ps_lattice_t *dag, char const *filename);






POCKETSPHINX_EXPORT
int ps_lattice_write_htk(ps_lattice_t *dag, char const *filename);









POCKETSPHINX_EXPORT
logmath_t *ps_lattice_get_logmath(ps_lattice_t *dag);











POCKETSPHINX_EXPORT
ps_latnode_iter_t *ps_latnode_iter(ps_lattice_t *dag);






POCKETSPHINX_EXPORT
ps_latnode_iter_t *ps_latnode_iter_next(ps_latnode_iter_t *itor);





POCKETSPHINX_EXPORT
void ps_latnode_iter_free(ps_latnode_iter_t *itor);




POCKETSPHINX_EXPORT
ps_latnode_t *ps_latnode_iter_node(ps_latnode_iter_t *itor);









POCKETSPHINX_EXPORT
int ps_latnode_times(ps_latnode_t *node, int16 *out_fef, int16 *out_lef);








POCKETSPHINX_EXPORT
char const *ps_latnode_word(ps_lattice_t *dag, ps_latnode_t *node);








POCKETSPHINX_EXPORT
char const *ps_latnode_baseword(ps_lattice_t *dag, ps_latnode_t *node);







POCKETSPHINX_EXPORT
ps_latlink_iter_t *ps_latnode_exits(ps_latnode_t *node);







POCKETSPHINX_EXPORT
ps_latlink_iter_t *ps_latnode_entries(ps_latnode_t *node);












POCKETSPHINX_EXPORT
int32 ps_latnode_prob(ps_lattice_t *dag, ps_latnode_t *node,
                      ps_latlink_t **out_link);







POCKETSPHINX_EXPORT
ps_latlink_iter_t *ps_latlink_iter_next(ps_latlink_iter_t *itor);





POCKETSPHINX_EXPORT
void ps_latlink_iter_free(ps_latlink_iter_t *itor);




POCKETSPHINX_EXPORT
ps_latlink_t *ps_latlink_iter_link(ps_latlink_iter_t *itor);











POCKETSPHINX_EXPORT
int ps_latlink_times(ps_latlink_t *link, int16 *out_sf);








POCKETSPHINX_EXPORT
ps_latnode_t *ps_latlink_nodes(ps_latlink_t *link, ps_latnode_t **out_src);








POCKETSPHINX_EXPORT
char const *ps_latlink_word(ps_lattice_t *dag, ps_latlink_t *link);








POCKETSPHINX_EXPORT
char const *ps_latlink_baseword(ps_lattice_t *dag, ps_latlink_t *link);







POCKETSPHINX_EXPORT
ps_latlink_t *ps_latlink_pred(ps_latlink_t *link);











POCKETSPHINX_EXPORT
int32 ps_latlink_prob(ps_lattice_t *dag, ps_latlink_t *link, int32 *out_ascr);





POCKETSPHINX_EXPORT
void ps_lattice_link(ps_lattice_t *dag, ps_latnode_t *from, ps_latnode_t *to,
                     int32 score, int32 ef);
















POCKETSPHINX_EXPORT
ps_latlink_t *ps_lattice_traverse_edges(ps_lattice_t *dag, ps_latnode_t *start, ps_latnode_t *end);








POCKETSPHINX_EXPORT
ps_latlink_t *ps_lattice_traverse_next(ps_lattice_t *dag, ps_latnode_t *end);











POCKETSPHINX_EXPORT
ps_latlink_t *ps_lattice_reverse_edges(ps_lattice_t *dag, ps_latnode_t *start, ps_latnode_t *end);








POCKETSPHINX_EXPORT
ps_latlink_t *ps_lattice_reverse_next(ps_lattice_t *dag, ps_latnode_t *start);









POCKETSPHINX_EXPORT
ps_latlink_t *ps_lattice_bestpath(ps_lattice_t *dag, ngram_model_t *lmset,
                                  float32 lwf, float32 ascale);








POCKETSPHINX_EXPORT
int32 ps_lattice_posterior(ps_lattice_t *dag, ngram_model_t *lmset,
                           float32 ascale);












POCKETSPHINX_EXPORT
int32 ps_lattice_posterior_prune(ps_lattice_t *dag, int32 beam);

#ifdef NOT_IMPLEMENTED_YET






POCKETSPHINX_EXPORT
int32 ps_lattice_ngram_expand(ps_lattice_t *dag, ngram_model_t *lm);
#endif







POCKETSPHINX_EXPORT
int ps_lattice_n_frames(ps_lattice_t *dag);

#endif 
