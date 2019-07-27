











































#ifndef __BIN_MDEF_H__
#define __BIN_MDEF_H__

#ifdef __cplusplus
extern "C" {
#endif


#include <sphinxbase/mmio.h>
#include <sphinxbase/cmd_ln.h>
#include <pocketsphinx_export.h>

#include "mdef.h"

#define BIN_MDEF_FORMAT_VERSION 1

#define BIN_MDEF_NATIVE_ENDIAN 0x46444d42 /* 'BMDF' in little-endian order */
#define BIN_MDEF_OTHER_ENDIAN 0x424d4446  /* 'BMDF' in big-endian order */
#ifdef __GNUC__
#define __ATTRIBUTE_PACKED __attribute__((packed))
#else
#define __ATTRIBUTE_PACKED
#endif




typedef struct mdef_entry_s mdef_entry_t;
struct mdef_entry_s {
	int32 ssid; 
	int32 tmat; 
	
	union {
		
		struct {
			uint8 filler;
			uint8 reserved[3];
		} ci;
		
		struct {
			uint8 wpos;
			uint8 ctx[3]; 
		} cd;
	} info;
} __ATTRIBUTE_PACKED;




#define BAD_SSID 0xffff



#define BAD_SENID 0xffff




typedef struct cd_tree_s cd_tree_t;
struct cd_tree_s {
	int16 ctx; 
	int16 n_down; 
	union {
		int32 pid; 
		int32 down;  
	} c;
};




typedef struct bin_mdef_s bin_mdef_t;
struct bin_mdef_s {
	int refcnt;
	int32 n_ciphone;    
	int32 n_phone;	    
	int32 n_emit_state; 
	int32 n_ci_sen;	    
	int32 n_sen;	    
	int32 n_tmat;	    
	int32 n_sseq;       
	int32 n_ctx;	    
	int32 n_cd_tree;    
	int16 sil;	    

	mmio_file_t *filemap;
	char **ciname;       
	cd_tree_t *cd_tree;  
	mdef_entry_t *phone; 
	uint16 **sseq;       
	uint8 *sseq_len;     

	
	int16 *cd2cisen;	
	int16 *sen2cimap;	

	
	enum { BIN_MDEF_FROM_TEXT, BIN_MDEF_IN_MEMORY, BIN_MDEF_ON_DISK } alloc_mode;
};

#define bin_mdef_is_fillerphone(m,p)	(((p) < (m)->n_ciphone) \
		                         ? (m)->phone[p].info.ci.filler \
					 : (m)->phone[(m)->phone[p].info.cd.ctx[0]].info.ci.filler)
#define bin_mdef_is_ciphone(m,p)	((p) < (m)->n_ciphone)
#define bin_mdef_n_ciphone(m)		((m)->n_ciphone)
#define bin_mdef_n_phone(m)		((m)->n_phone)
#define bin_mdef_n_sseq(m)		((m)->n_sseq)
#define bin_mdef_n_emit_state(m)	((m)->n_emit_state)
#define bin_mdef_n_emit_state_phone(m,p) ((m)->n_emit_state ? (m)->n_emit_state \
					  : (m)->sseq_len[(m)->phone[p].ssid])
#define bin_mdef_n_sen(m)		((m)->n_sen)
#define bin_mdef_n_tmat(m)		((m)->n_tmat)
#define bin_mdef_pid2ssid(m,p)		((m)->phone[p].ssid)
#define bin_mdef_pid2tmatid(m,p)	((m)->phone[p].tmat)
#define bin_mdef_silphone(m)		((m)->sil)
#define bin_mdef_sen2cimap(m,s)		((m)->sen2cimap[s])
#define bin_mdef_sseq2sen(m,ss,pos)	((m)->sseq[ss][pos])
#define bin_mdef_pid2ci(m,p)		(((p) < (m)->n_ciphone) ? (p) \
                                         : (m)->phone[p].info.cd.ctx[0])




POCKETSPHINX_EXPORT
bin_mdef_t *bin_mdef_read(cmd_ln_t *config, const char *filename);



POCKETSPHINX_EXPORT
bin_mdef_t *bin_mdef_read_text(cmd_ln_t *config, const char *filename);



POCKETSPHINX_EXPORT
int bin_mdef_write(bin_mdef_t *m, const char *filename);



POCKETSPHINX_EXPORT
int bin_mdef_write_text(bin_mdef_t *m, const char *filename);



bin_mdef_t *bin_mdef_retain(bin_mdef_t *m);



int bin_mdef_free(bin_mdef_t *m);





int bin_mdef_ciphone_id(bin_mdef_t *m,	       
			const char *ciphone);  





int bin_mdef_ciphone_id_nocase(bin_mdef_t *m,	     
			       const char *ciphone); 


const char *bin_mdef_ciphone_str(bin_mdef_t *m,	
				 int32 ci);	


int bin_mdef_phone_id(bin_mdef_t *m,	
		      int32 b,		
		      int32 l,		
		      int32 r,		
		      int32 pos);	


int bin_mdef_phone_id_nearest(bin_mdef_t * m, int32 b,
			      int32 l, int32 r, int32 pos);






int bin_mdef_phone_str(bin_mdef_t *m,	
		       int pid,		
		       char *buf);	

#ifdef __cplusplus
}; 
#endif 

#endif
