









































#ifndef __ACMOD_H__
#define __ACMOD_H__


#include <stdio.h>


#include <sphinxbase/cmd_ln.h>
#include <sphinxbase/logmath.h>
#include <sphinxbase/fe.h>
#include <sphinxbase/feat.h>
#include <sphinxbase/bitvec.h>
#include <sphinxbase/err.h>
#include <sphinxbase/prim_type.h>


#include "ps_mllr.h"
#include "bin_mdef.h"
#include "tmat.h"
#include "hmm.h"




typedef enum acmod_state_e {
    ACMOD_IDLE,		
    ACMOD_STARTED,      
    ACMOD_PROCESSING,   
    ACMOD_ENDED         
} acmod_state_t;




#define SENSCR_DUMMY 0x7fff




struct ps_mllr_s {
    int refcnt;     
    int n_class;    
    int n_feat;     
    int *veclen;    
    float32 ****A;  
    float32 ***b;   
    float32 ***h;   
    int32 *cb2mllr; 
};




typedef struct ps_mgau_s ps_mgau_t;

typedef struct ps_mgaufuncs_s {
    char const *name;

    int (*frame_eval)(ps_mgau_t *mgau,
                      int16 *senscr,
                      uint8 *senone_active,
                      int32 n_senone_active,
                      mfcc_t ** feat,
                      int32 frame,
                      int32 compallsen);
    int (*transform)(ps_mgau_t *mgau,
                     ps_mllr_t *mllr);
    void (*free)(ps_mgau_t *mgau);
} ps_mgaufuncs_t;    

struct ps_mgau_s {
    ps_mgaufuncs_t *vt;  
    int frame_idx;       
};

#define ps_mgau_base(mg) ((ps_mgau_t *)(mg))
#define ps_mgau_frame_eval(mg,senscr,senone_active,n_senone_active,feat,frame,compallsen) \
    (*ps_mgau_base(mg)->vt->frame_eval)                                 \
    (mg, senscr, senone_active, n_senone_active, feat, frame, compallsen)
#define ps_mgau_transform(mg, mllr)                                  \
    (*ps_mgau_base(mg)->vt->transform)(mg, mllr)
#define ps_mgau_free(mg)                                  \
    (*ps_mgau_base(mg)->vt->free)(mg)






















struct acmod_s {
    
    cmd_ln_t *config;          
    logmath_t *lmath;          
    glist_t strings;           

    
    fe_t *fe;                  
    feat_t *fcb;               

    
    bin_mdef_t *mdef;          
    tmat_t *tmat;              
    ps_mgau_t *mgau;           
    ps_mllr_t *mllr;           

    
    int16 *senone_scores;      
    bitvec_t *senone_active_vec; 
    uint8 *senone_active;      
    int senscr_frame;          
    int n_senone_active;       
    int log_zero;              

    
    mfcc_t **mfc_buf;   
    mfcc_t ***feat_buf; 
    FILE *rawfh;        
    FILE *mfcfh;        
    FILE *senfh;        
    FILE *insenfh;	
    long *framepos;     

    
    int16 *rawdata;
    int32 rawdata_size;
    int32 rawdata_pos;

    
    uint8 state;        
    uint8 compallsen;   
    uint8 grow_feat;    
    uint8 insen_swap;   

    frame_idx_t utt_start_frame; 

    frame_idx_t output_frame; 
    frame_idx_t n_mfc_alloc;  
    frame_idx_t n_mfc_frame;  
    frame_idx_t mfc_outidx;   
    frame_idx_t n_feat_alloc; 
    frame_idx_t n_feat_frame; 
    frame_idx_t feat_outidx;  
};
typedef struct acmod_s acmod_t;

















acmod_t *acmod_init(cmd_ln_t *config, logmath_t *lmath, fe_t *fe, feat_t *fcb);












ps_mllr_t *acmod_update_mllr(acmod_t *acmod, ps_mllr_t *mllr);








int acmod_set_senfh(acmod_t *acmod, FILE *senfh);








int acmod_set_mfcfh(acmod_t *acmod, FILE *logfh);








int acmod_set_rawfh(acmod_t *acmod, FILE *logfh);




void acmod_free(acmod_t *acmod);




int acmod_start_utt(acmod_t *acmod);




int acmod_end_utt(acmod_t *acmod);













int acmod_rewind(acmod_t *acmod);










int acmod_advance(acmod_t *acmod);









int acmod_set_grow(acmod_t *acmod, int grow_feat);



















int acmod_process_raw(acmod_t *acmod,
                      int16 const **inout_raw,
                      size_t *inout_n_samps,
                      int full_utt);












int acmod_process_cep(acmod_t *acmod,
                      mfcc_t ***inout_cep,
                      int *inout_n_frames,
                      int full_utt);














int acmod_process_feat(acmod_t *acmod,
                       mfcc_t **feat);







int acmod_set_insenfh(acmod_t *acmod, FILE *insenfh);






int acmod_read_scores(acmod_t *acmod);










mfcc_t **acmod_get_frame(acmod_t *acmod, int *inout_frame_idx);














int16 const *acmod_score(acmod_t *acmod,
                         int *inout_frame_idx);




int acmod_write_senfh_header(acmod_t *acmod, FILE *logfh);




int acmod_write_scores(acmod_t *acmod, int n_active, uint8 const *active,
                       int16 const *senscr, FILE *senfh);





int acmod_best_score(acmod_t *acmod, int *out_best_senid);




void acmod_clear_active(acmod_t *acmod);




void acmod_activate_hmm(acmod_t *acmod, hmm_t *hmm);




#define acmod_activate_sen(acmod, sen) bitvec_set((acmod)->senone_active_vec, sen)




int32 acmod_flags2list(acmod_t *acmod);




int32 acmod_stream_offset(acmod_t *acmod);




void acmod_start_stream(acmod_t *acmod);




void acmod_set_rawdata_size(acmod_t *acmod, int32 size);




void acmod_get_rawdata(acmod_t *acmod, int16 **buffer, int32 *size);

#endif 
