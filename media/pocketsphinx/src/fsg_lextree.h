





































#ifndef __S2_FSG_LEXTREE_H__
#define __S2_FSG_LEXTREE_H__


#include <sphinxbase/cmd_ln.h>
#include <sphinxbase/fsg_model.h>


#include "hmm.h"
#include "dict.h"
#include "dict2pid.h"








#define FSG_PNODE_CTXT_BVSZ	4

typedef struct {
    uint32 bv[FSG_PNODE_CTXT_BVSZ];
} fsg_pnode_ctxt_t;
















typedef struct fsg_pnode_s {
    






    union {
        struct fsg_pnode_s *succ;
        fsg_link_t *fsglink;
    } next;
  
    




    struct fsg_pnode_s *alloc_next;
  
    



    struct fsg_pnode_s *sibling;

    






    int32 logs2prob;
  
    











    fsg_pnode_ctxt_t ctxt;
  
    uint16 ci_ext;	
    uint8 ppos;	
    uint8 leaf;	
  
    
    hmm_context_t *ctx;
    hmm_t hmm;
} fsg_pnode_t;


#define fsg_pnode_leaf(p)	((p)->leaf)
#define fsg_pnode_logs2prob(p)	((p)->logs2prob)
#define fsg_pnode_succ(p)	((p)->next.succ)
#define fsg_pnode_fsglink(p)	((p)->next.fsglink)
#define fsg_pnode_sibling(p)	((p)->sibling)
#define fsg_pnode_hmmptr(p)	(&((p)->hmm))
#define fsg_pnode_ci_ext(p)	((p)->ci_ext)
#define fsg_pnode_ppos(p)	((p)->ppos)
#define fsg_pnode_leaf(p)	((p)->leaf)
#define fsg_pnode_ctxt(p)	((p)->ctxt)

#define fsg_pnode_add_ctxt(p,c)	((p)->ctxt.bv[(c)>>5] |= (1 << ((c)&0x001f)))










#if (FSG_PNODE_CTXT_BVSZ == 1)
    #define FSG_PNODE_CTXT_SUB(src,sub) \
    ((src)->bv[0] = (~((sub)->bv[0]) & (src)->bv[0]))
#elif (FSG_PNODE_CTXT_BVSZ == 2)
    #define FSG_PNODE_CTXT_SUB(src,sub) \
    (((src)->bv[0] = (~((sub)->bv[0]) & (src)->bv[0])) | \
     ((src)->bv[1] = (~((sub)->bv[1]) & (src)->bv[1])))
#elif (FSG_PNODE_CTXT_BVSZ == 4)
    #define FSG_PNODE_CTXT_SUB(src,sub) \
    (((src)->bv[0] = (~((sub)->bv[0]) & (src)->bv[0]))  | \
     ((src)->bv[1] = (~((sub)->bv[1]) & (src)->bv[1]))  | \
     ((src)->bv[2] = (~((sub)->bv[2]) & (src)->bv[2]))  | \
     ((src)->bv[3] = (~((sub)->bv[3]) & (src)->bv[3])))
#else
    #define FSG_PNODE_CTXT_SUB(src,sub) fsg_pnode_ctxt_sub_generic((src),(sub))
#endif




typedef struct fsg_lextree_s {
    fsg_model_t *fsg;	
    hmm_context_t *ctx; 
    dict_t *dict;     
    dict2pid_t *d2p;    
    bin_mdef_t *mdef;   

    
















    int16 **lc;         
    int16 **rc;         

    fsg_pnode_t **root;	



    fsg_pnode_t **alloc_head;	

    int32 n_pnode;	
    int32 wip;
    int32 pip;
} fsg_lextree_t;


#define fsg_lextree_root(lt,s)	((lt)->root[s])
#define fsg_lextree_n_pnode(lt)	((lt)->n_pnode)




fsg_lextree_t *fsg_lextree_init(fsg_model_t *fsg, dict_t *dict,
                                dict2pid_t *d2p,
				bin_mdef_t *mdef, hmm_context_t *ctx,
				int32 wip, int32 pip);




void fsg_lextree_free(fsg_lextree_t *fsg);




void fsg_lextree_dump(fsg_lextree_t *fsg, FILE *fh);




void fsg_psubtree_pnode_deactivate(fsg_pnode_t *pnode);




void fsg_pnode_add_all_ctxt(fsg_pnode_ctxt_t *ctxt);




uint32 fsg_pnode_ctxt_sub_generic(fsg_pnode_ctxt_t *src, fsg_pnode_ctxt_t *sub);

#endif
