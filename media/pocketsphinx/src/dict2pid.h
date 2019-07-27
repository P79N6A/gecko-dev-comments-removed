




































#ifndef _S3_DICT2PID_H_
#define _S3_DICT2PID_H_


#include <stdio.h>


#include <sphinxbase/logmath.h>
#include <sphinxbase/bitvec.h>


#include "s3types.h"
#include "bin_mdef.h"
#include "dict.h"












#ifdef __cplusplus
extern "C" {
#endif






typedef struct {
    s3ssid_t  *ssid;	
    s3cipid_t *cimap;	
    int32     n_ssid;	
} xwdssid_t;






typedef struct {
    int refcount;

    bin_mdef_t *mdef;           

    dict_t *dict;               

    
    


    s3ssid_t ***ldiph_lc;	



    xwdssid_t **rssid;          





    s3ssid_t ***lrdiph_rc;      


    xwdssid_t **lrssid;          



} dict2pid_t;


#define dict2pid_rssid(d,ci,lc)  (&(d)->rssid[ci][lc])
#define dict2pid_ldiph_lc(d,b,r,l) ((d)->ldiph_lc[b][r][l])
#define dict2pid_lrdiph_rc(d,b,l,r) ((d)->lrdiph_rc[b][l][r])




dict2pid_t *dict2pid_build(bin_mdef_t *mdef,   
                           dict_t *dict        
    );




dict2pid_t *dict2pid_retain(dict2pid_t *d2p);  




int dict2pid_free(dict2pid_t *d2p 
    );




s3ssid_t dict2pid_internal(dict2pid_t *d2p,
                           int32 wid,
                           int pos);




int dict2pid_add_word(dict2pid_t *d2p,
                      int32 wid);




void dict2pid_dump(FILE *fp,        
                   dict2pid_t *d2p 
    );


void dict2pid_report(dict2pid_t *d2p 
    );




int32 get_rc_nssid(dict2pid_t *d2p,  
		   s3wid_t w         
    );




s3cipid_t* dict2pid_get_rcmap(dict2pid_t *d2p,  
			      s3wid_t w        
    );

#ifdef __cplusplus
}
#endif


#endif
