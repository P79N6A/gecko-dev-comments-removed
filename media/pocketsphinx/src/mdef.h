
















































#ifndef __MDEF_H__
#define __MDEF_H__



#include <stdio.h>


#include <sphinxbase/hash_table.h>

#ifdef __cplusplus
extern "C" {
#endif









typedef enum {
    WORD_POSN_INTERNAL = 0,	
    WORD_POSN_BEGIN = 1,	
    WORD_POSN_END = 2,		
    WORD_POSN_SINGLE = 3,	
    WORD_POSN_UNDEFINED = 4	
} word_posn_t;
#define N_WORD_POSN	4	/**< total # of word positions (excluding undefined) */
#define WPOS_NAME	"ibesu"	/**< Printable code for each word position above */
#define S3_SILENCE_CIPHONE "SIL" /**< Hard-coded silence CI phone name */





typedef struct {
    char *name;                 
    int32 filler;		

} ciphone_t;





typedef struct {
    int32 ssid;			


    int32 tmat;			
    int16 ci, lc, rc;		
    word_posn_t wpos;		
    
} phone_t;






typedef struct ph_rc_s {
    int16 rc;			
    int32 pid;			
    struct ph_rc_s *next;	
} ph_rc_t;






typedef struct ph_lc_s {
    int16 lc;			
    ph_rc_t *rclist;		
    struct ph_lc_s *next;	
} ph_lc_t;







typedef struct {
    int32 n_ciphone;		
    int32 n_phone;		
    int32 n_emit_state;		
    int32 n_ci_sen;		
    int32 n_sen;		
    int32 n_tmat;		
    
    hash_table_t *ciphone_ht;	
    ciphone_t *ciphone;		
    phone_t *phone;		
    uint16 **sseq;		

    int32 n_sseq;		
    
    int16 *cd2cisen;		


    int16 *sen2cimap;		
    
    int16 sil;			
    
    ph_lc_t ***wpos_ci_lclist;	




} mdef_t;


#define mdef_is_fillerphone(m,p)	((m)->ciphone[p].filler)
#define mdef_n_ciphone(m)		((m)->n_ciphone)
#define mdef_n_phone(m)			((m)->n_phone)
#define mdef_n_sseq(m)			((m)->n_sseq)
#define mdef_n_emit_state(m)		((m)->n_emit_state)
#define mdef_n_sen(m)			((m)->n_sen)
#define mdef_n_tmat(m)			((m)->n_tmat)
#define mdef_pid2ssid(m,p)		((m)->phone[p].ssid)
#define mdef_pid2tmatid(m,p)		((m)->phone[p].tmat)
#define mdef_silphone(m)		((m)->sil)
#define mdef_sen2cimap(m)		((m)->sen2cimap)
#define mdef_sseq2sen(m,ss,pos)		((m)->sseq[ss][pos])
#define mdef_pid2ci(m,p)		((m)->phone[p].ci)
#define mdef_cd2cisen(m)		((m)->cd2cisen)






mdef_t *mdef_init (char *mdeffile, 
		   int breport     
    );






int mdef_ciphone_id(mdef_t *m,		
                    char *ciphone	
    );





const char *mdef_ciphone_str(mdef_t *m,	
                             int ci	
    );





int mdef_is_ciphone (mdef_t *m,		
                     int p		
    );




  
int mdef_is_cisenone(mdef_t *m,               
                     int s		        
    );





int mdef_phone_id (mdef_t *m,		
                   int b,		
                   int l,		
                   int r,		
                   word_posn_t pos	
    );





int mdef_phone_str(mdef_t *m,		
                   int pid,		
                   char *buf		
    );






int mdef_hmm_cmp (mdef_t *m,	
                  int p1, 	
                  int p2	
    );


void mdef_report(mdef_t *m 
    );


void mdef_free_recursive_lc (ph_lc_t *lc 
    );
void mdef_free_recursive_rc (ph_rc_t *rc 
    );


void mdef_free (mdef_t *mdef 
    );


#ifdef __cplusplus
}
#endif

#endif
