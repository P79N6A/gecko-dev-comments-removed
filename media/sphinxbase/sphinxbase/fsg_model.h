













































#ifndef __FSG_MODEL_H__
#define __FSG_MODEL_H__


#include <stdio.h>
#include <string.h>


#include <sphinxbase/prim_type.h>
#include <sphinxbase/glist.h>
#include <sphinxbase/logmath.h>
#include <sphinxbase/bitvec.h>
#include <sphinxbase/hash_table.h>
#include <sphinxbase/listelem_alloc.h>
#include <sphinxbase/sphinxbase_export.h>




typedef struct fsg_link_s {
    int32 from_state;
    int32 to_state;
    int32 logs2prob;	
    int32 wid;		
} fsg_link_t;


#define fsg_link_from_state(l)	((l)->from_state)
#define fsg_link_to_state(l)	((l)->to_state)
#define fsg_link_wid(l)		((l)->wid)
#define fsg_link_logs2prob(l)	((l)->logs2prob)




typedef struct trans_list_s trans_list_t;








typedef struct fsg_model_s {
    int refcount;       
    char *name;		
    int32 n_word;       
    int32 n_word_alloc; 
    char **vocab;       
    bitvec_t *silwords; 
    bitvec_t *altwords; 
    logmath_t *lmath;	
    int32 n_state;	
    int32 start_state;	
    int32 final_state;	
    float32 lw;		

    trans_list_t *trans; 
    listelem_alloc_t *link_alloc; 
} fsg_model_t;


#define fsg_model_name(f)		((f)->name)
#define fsg_model_n_state(f)		((f)->n_state)
#define fsg_model_start_state(f)	((f)->start_state)
#define fsg_model_final_state(f)	((f)->final_state)
#define fsg_model_log(f,p)		logmath_log((f)->lmath, p)
#define fsg_model_lw(f)			((f)->lw)
#define fsg_model_n_word(f)		((f)->n_word)
#define fsg_model_word_str(f,wid)       (wid == -1 ? "(NULL)" : (f)->vocab[wid])




typedef struct fsg_arciter_s fsg_arciter_t;




#define fsg_model_has_sil(f)            ((f)->silwords != NULL)




#define fsg_model_has_alt(f)            ((f)->altwords != NULL)

#define fsg_model_is_filler(f,wid) \
    (fsg_model_has_sil(f) ? bitvec_is_set((f)->silwords, wid) : FALSE)
#define fsg_model_is_alt(f,wid) \
    (fsg_model_has_alt(f) ? bitvec_is_set((f)->altwords, wid) : FALSE)




SPHINXBASE_EXPORT
fsg_model_t *fsg_model_init(char const *name, logmath_t *lmath,
                            float32 lw, int32 n_state);








































SPHINXBASE_EXPORT
fsg_model_t *fsg_model_readfile(const char *file, logmath_t *lmath, float32 lw);




SPHINXBASE_EXPORT
fsg_model_t *fsg_model_read(FILE *fp, logmath_t *lmath, float32 lw);






SPHINXBASE_EXPORT
fsg_model_t *fsg_model_retain(fsg_model_t *fsg);






SPHINXBASE_EXPORT
int fsg_model_free(fsg_model_t *fsg);






SPHINXBASE_EXPORT
int fsg_model_word_add(fsg_model_t *fsg, char const *word);






SPHINXBASE_EXPORT
int fsg_model_word_id(fsg_model_t *fsg, char const *word);







SPHINXBASE_EXPORT
void fsg_model_trans_add(fsg_model_t * fsg,
                         int32 from, int32 to, int32 logp, int32 wid);











SPHINXBASE_EXPORT
int32 fsg_model_null_trans_add(fsg_model_t * fsg, int32 from, int32 to, int32 logp);















SPHINXBASE_EXPORT
int32 fsg_model_tag_trans_add(fsg_model_t * fsg, int32 from, int32 to,
                              int32 logp, int32 wid);







SPHINXBASE_EXPORT
glist_t fsg_model_null_trans_closure(fsg_model_t * fsg, glist_t nulls);




SPHINXBASE_EXPORT
glist_t fsg_model_trans(fsg_model_t *fsg, int32 i, int32 j);




SPHINXBASE_EXPORT
fsg_arciter_t *fsg_model_arcs(fsg_model_t *fsg, int32 i);




SPHINXBASE_EXPORT
fsg_link_t *fsg_arciter_get(fsg_arciter_t *itor);




SPHINXBASE_EXPORT
fsg_arciter_t *fsg_arciter_next(fsg_arciter_t *itor);




SPHINXBASE_EXPORT
void fsg_arciter_free(fsg_arciter_t *itor);



SPHINXBASE_EXPORT
fsg_link_t *fsg_model_null_trans(fsg_model_t *fsg, int32 i, int32 j);







SPHINXBASE_EXPORT
int fsg_model_add_silence(fsg_model_t * fsg, char const *silword,
                          int state, float32 silprob);




SPHINXBASE_EXPORT
int fsg_model_add_alt(fsg_model_t * fsg, char const *baseword,
                      char const *altword);




SPHINXBASE_EXPORT
void fsg_model_write(fsg_model_t *fsg, FILE *fp);




SPHINXBASE_EXPORT
void fsg_model_writefile(fsg_model_t *fsg, char const *file);




SPHINXBASE_EXPORT
void fsg_model_write_fsm(fsg_model_t *fsg, FILE *fp);




SPHINXBASE_EXPORT
void fsg_model_writefile_fsm(fsg_model_t *fsg, char const *file);




SPHINXBASE_EXPORT
void fsg_model_write_symtab(fsg_model_t *fsg, FILE *file);




SPHINXBASE_EXPORT
void fsg_model_writefile_symtab(fsg_model_t *fsg, char const *file);

#endif 
