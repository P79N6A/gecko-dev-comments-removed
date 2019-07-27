







































#ifndef _S3_FEAT_H_
#define _S3_FEAT_H_

#include <stdio.h>


#include <sphinxbase/sphinxbase_export.h>
#include <sphinxbase/prim_type.h>
#include <sphinxbase/fe.h>
#include <sphinxbase/cmn.h>
#include <sphinxbase/agc.h>

#ifdef __cplusplus
extern "C" {
#endif
#if 0

}
#endif




#define LIVEBUFBLOCKSIZE        256    /** Blocks of 256 vectors allocated 
					   for livemode decoder */
#define S3_MAX_FRAMES		15000    /* RAH, I believe this is still too large, but better than before */

#define cepstral_to_feature_command_line_macro()                        \
{ "-feat",                                                              \
      ARG_STRING,                                                       \
      "1s_c_d_dd",                                                      \
      "Feature stream type, depends on the acoustic model" },           \
{ "-ceplen",                                                            \
      ARG_INT32,                                                        \
      "13",                                                             \
     "Number of components in the input feature vector" },              \
{ "-cmn",                                                               \
      ARG_STRING,                                                       \
      "current",                                                        \
      "Cepstral mean normalization scheme ('current', 'prior', or 'none')" }, \
{ "-cmninit",                                                           \
      ARG_STRING,                                                       \
      "8.0",                                                            \
      "Initial values (comma-separated) for cepstral mean when 'prior' is used" }, \
{ "-varnorm",                                                           \
      ARG_BOOLEAN,                                                      \
      "no",                                                             \
      "Variance normalize each utterance (only if CMN == current)" },   \
{ "-agc",                                                               \
      ARG_STRING,                                                       \
      "none",                                                           \
      "Automatic gain control for c0 ('max', 'emax', 'noise', or 'none')" }, \
{ "-agcthresh",                                                         \
      ARG_FLOAT32,                                                      \
      "2.0",                                                            \
      "Initial threshold for automatic gain control" },                 \
{ "-lda",                                                               \
      ARG_STRING,                                                       \
      NULL,                                                             \
      "File containing transformation matrix to be applied to features (single-stream features only)" }, \
{ "-ldadim",                                                            \
      ARG_INT32,                                                        \
      "0",                                                              \
      "Dimensionality of output of feature transformation (0 to use entire matrix)" }, \
{"-svspec",                                                             \
     ARG_STRING,                                                        \
     NULL,                                                           \
     "Subvector specification (e.g., 24,0-11/25,12-23/26-38 or 0-12/13-25/26-38)"}








typedef struct feat_s {
    int refcount;       
    char *name;		
    int32 cepsize;	
    int32 n_stream;	
    uint32 *stream_len;	
    int32 window_size;	

    int32 n_sv;         
    uint32 *sv_len;      
    int32 **subvecs;    
    mfcc_t *sv_buf;      
    int32 sv_dim;       

    cmn_type_t cmn;	
    int32 varnorm;	

    agc_type_t agc;	

    











    void (*compute_feat)(struct feat_s *fcb, mfcc_t **input, mfcc_t **feat);
    cmn_t *cmn_struct;	

    agc_t *agc_struct;	


    mfcc_t **cepbuf;    
    mfcc_t **tmpcepbuf; 
    int32   bufpos;     
    int32   curpos;     

    mfcc_t ***lda; 
    uint32 n_lda;   
    uint32 out_dim; 
} feat_t;




#define feat_name(f)		((f)->name)



#define feat_cepsize(f)		((f)->cepsize)



#define feat_window_size(f)	((f)->window_size)





#define feat_n_stream(f)	((f)->n_stream)





#define feat_stream_len(f,i)	((f)->stream_len[i])



#define feat_dimension1(f)	((f)->n_sv ? (f)->n_sv : f->n_stream)



#define feat_dimension2(f,i)	((f)->lda ? (f)->out_dim : ((f)->sv_len ? (f)->sv_len[i] : f->stream_len[i]))



#define feat_dimension(f)	((f)->out_dim)



#define feat_stream_lengths(f)  ((f)->lda ? (&(f)->out_dim) : (f)->sv_len ? (f)->sv_len : f->stream_len)























SPHINXBASE_EXPORT
int32 **parse_subvecs(char const *str);




SPHINXBASE_EXPORT
void subvecs_free(int32 **subvecs);














SPHINXBASE_EXPORT
mfcc_t ***feat_array_alloc(feat_t *fcb,	

                           int32 nfr	
    );




SPHINXBASE_EXPORT
mfcc_t ***feat_array_realloc(feat_t *fcb, 

			     mfcc_t ***old_feat, 
                             int32 ofr,	
                             int32 nfr	
    );




SPHINXBASE_EXPORT
void feat_array_free(mfcc_t ***feat);

















SPHINXBASE_EXPORT
feat_t *feat_init(char const *type,
                  cmn_type_t cmn, 


                  int32 varnorm,  


                  agc_type_t agc, 

                  int32 breport, 
                  int32 cepsize  


    );





SPHINXBASE_EXPORT
int32 feat_read_lda(feat_t *feat,	 
                    const char *ldafile, 
                    int32 dim		 
    );




SPHINXBASE_EXPORT
void feat_lda_transform(feat_t *fcb,		
                        mfcc_t ***inout_feat,	
                        uint32 nfr		
    );



















SPHINXBASE_EXPORT
int feat_set_subvecs(feat_t *fcb, int32 **subvecs);




SPHINXBASE_EXPORT
void feat_print(feat_t *fcb,		
		mfcc_t ***feat,		
		int32 nfr,		
		FILE *fp		
    );

  
















SPHINXBASE_EXPORT
int32 feat_s2mfc2feat(feat_t *fcb,	
		      const char *file,	
		      const char *dir,	

		      const char *cepext,


		      int32 sf, int32 ef,   



		      mfcc_t ***feat,	

		      int32 maxfr	



    );






























SPHINXBASE_EXPORT
int32 feat_s2mfc2feat_live(feat_t  *fcb,     
                           mfcc_t **uttcep,  
                           int32 *inout_ncep,

                           int32 beginutt,   
                           int32 endutt,     
                           mfcc_t ***ofeat   


    );






SPHINXBASE_EXPORT
void feat_update_stats(feat_t *fcb);







SPHINXBASE_EXPORT
feat_t *feat_retain(feat_t *f);






SPHINXBASE_EXPORT
int feat_free(feat_t *f 
    );




SPHINXBASE_EXPORT
void feat_report(feat_t *f 
    );
#ifdef __cplusplus
}
#endif


#endif
