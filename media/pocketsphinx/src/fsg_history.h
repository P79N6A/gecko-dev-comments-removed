








































































#ifndef __S2_FSG_HISTORY_H__
#define __S2_FSG_HISTORY_H__



#include <sphinxbase/prim_type.h>
#include <sphinxbase/fsg_model.h>


#include "blkarray_list.h"
#include "fsg_lextree.h"
#include "dict.h"









typedef struct fsg_hist_entry_s {
    fsg_link_t *fsglink;		
    int32 score;			

    int32 pred; 			
    frame_idx_t frame;			
    int16 lc;			        

    fsg_pnode_ctxt_t rc;		

} fsg_hist_entry_t;


#define fsg_hist_entry_fsglink(v)	((v)->fsglink)
#define fsg_hist_entry_frame(v)		((v)->frame)
#define fsg_hist_entry_score(v)		((v)->score)
#define fsg_hist_entry_pred(v)		((v)->pred)
#define fsg_hist_entry_lc(v)		((v)->lc)
#define fsg_hist_entry_rc(v)		((v)->rc)





























typedef struct fsg_history_s {
    fsg_model_t *fsg;		
    blkarray_list_t *entries;	

    glist_t **frame_entries;
    int n_ciphone;
} fsg_history_t;






fsg_history_t *fsg_history_init(fsg_model_t *fsg, dict_t *dict);

void fsg_history_utt_start(fsg_history_t *h);

void fsg_history_utt_end(fsg_history_t *h);










void fsg_history_entry_add (fsg_history_t *h,
			    fsg_link_t *l,	
			    int32 frame,
			    int32 score,
			    int32 pred,
			    int32 lc,
			    fsg_pnode_ctxt_t rc);








void fsg_history_end_frame (fsg_history_t *h);



void fsg_history_reset (fsg_history_t *h);



int32 fsg_history_n_entries (fsg_history_t *h);





fsg_hist_entry_t *fsg_history_entry_get(fsg_history_t *h, int32 id);






void fsg_history_set_fsg (fsg_history_t *h, fsg_model_t *fsg, dict_t *dict);


void fsg_history_free (fsg_history_t *h);


void fsg_history_print(fsg_history_t *h, dict_t *dict);
				     
#endif
