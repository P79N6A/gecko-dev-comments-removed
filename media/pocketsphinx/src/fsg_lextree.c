








































#include <stdio.h>
#include <string.h>
#include <assert.h>


#include <sphinxbase/ckd_alloc.h>
#include <sphinxbase/err.h>


#include "fsg_lextree.h"

#define __FSG_DBG__		0


typedef struct fsg_glist_linklist_t {
    int32    ci, rc;
    glist_t  glist;
    struct   fsg_glist_linklist_t *next;
} fsg_glist_linklist_t;







static fsg_pnode_t *fsg_psubtree_init(fsg_lextree_t *tree,
                                      fsg_model_t *fsg,
                                      int32 from_state,
                                      fsg_pnode_t **alloc_head);





static void fsg_psubtree_free(fsg_pnode_t *alloc_head);





static void fsg_psubtree_dump(fsg_lextree_t *tree, fsg_pnode_t *root, FILE *fp);




static void
fsg_lextree_lc_rc(fsg_lextree_t *lextree)
{
    int32 s, i, j;
    int32 n_ci;
    fsg_model_t *fsg;
    int32 silcipid;
    int32 len;

    silcipid = bin_mdef_silphone(lextree->mdef);
    assert(silcipid >= 0);
    n_ci = bin_mdef_n_ciphone(lextree->mdef);

    fsg = lextree->fsg;
    



    lextree->lc = ckd_calloc_2d(fsg->n_state, n_ci + 1, sizeof(**lextree->lc));
    lextree->rc = ckd_calloc_2d(fsg->n_state, n_ci + 1, sizeof(**lextree->rc));
    E_INFO("Allocated %d bytes (%d KiB) for left and right context phones\n",
           fsg->n_state * (n_ci + 1) * 2,
           fsg->n_state * (n_ci + 1) * 2 / 1024);


    for (s = 0; s < fsg->n_state; s++) {
        fsg_arciter_t *itor;
        for (itor = fsg_model_arcs(fsg, s); itor; itor = fsg_arciter_next(itor)) {
            fsg_link_t *l = fsg_arciter_get(itor);
            int32 dictwid; 

            if (fsg_link_wid(l) >= 0) {
                dictwid = dict_wordid(lextree->dict,
                                      fsg_model_word_str(lextree->fsg, l->wid));

                









                if (fsg_model_is_filler(fsg, fsg_link_wid(l))) {
                    
                    lextree->rc[fsg_link_from_state(l)][silcipid] = 1;
                    lextree->lc[fsg_link_to_state(l)][silcipid] = 1;
                }
                else {
                    len = dict_pronlen(lextree->dict, dictwid);
                    lextree->rc[fsg_link_from_state(l)][dict_pron(lextree->dict, dictwid, 0)] = 1;
                    lextree->lc[fsg_link_to_state(l)][dict_pron(lextree->dict, dictwid, len - 1)] = 1;
                }
            }
        }
    }

    for (s = 0; s < fsg->n_state; s++) {
        






        lextree->lc[s][silcipid] = 1;
        lextree->rc[s][silcipid] = 1;
    }

    







    for (s = 0; s < fsg->n_state; s++) {
        fsg_arciter_t *itor;
        for (itor = fsg_model_arcs(fsg, s); itor; itor = fsg_arciter_next(itor)) {
            fsg_link_t *l = fsg_arciter_get(itor);
            if (fsg_link_wid(l) < 0) {
                



                for (i = 0; i < n_ci; i++)
                    lextree->lc[fsg_link_to_state(l)][i] |= lextree->lc[fsg_link_from_state(l)][i];
                




                for (i = 0; i < n_ci; i++)
                    lextree->rc[fsg_link_from_state(l)][i] |= lextree->rc[fsg_link_to_state(l)][i];
            }
        }
    }

    
    for (s = 0; s < fsg->n_state; s++) {
        j = 0;
        for (i = 0; i < n_ci; i++) {
            if (lextree->lc[s][i]) {
                lextree->lc[s][j] = i;
                j++;
            }
        }
        lextree->lc[s][j] = -1;     

        j = 0;
        for (i = 0; i < n_ci; i++) {
            if (lextree->rc[s][i]) {
                lextree->rc[s][j] = i;
                j++;
            }
        }
        lextree->rc[s][j] = -1;     
    }
}




fsg_lextree_t *
fsg_lextree_init(fsg_model_t * fsg, dict_t *dict, dict2pid_t *d2p,
                 bin_mdef_t *mdef, hmm_context_t *ctx,
                 int32 wip, int32 pip)
{
    int32 s, n_leaves;
    fsg_lextree_t *lextree;
    fsg_pnode_t *pn;

    lextree = ckd_calloc(1, sizeof(fsg_lextree_t));
    lextree->fsg = fsg;
    lextree->root = ckd_calloc(fsg_model_n_state(fsg),
                               sizeof(fsg_pnode_t *));
    lextree->alloc_head = ckd_calloc(fsg_model_n_state(fsg),
                                     sizeof(fsg_pnode_t *));
    lextree->ctx = ctx;
    lextree->dict = dict;
    lextree->d2p = d2p;
    lextree->mdef = mdef;
    lextree->wip = wip;
    lextree->pip = pip;

    
    fsg_lextree_lc_rc(lextree);

    



    lextree->n_pnode = 0;
    n_leaves = 0;
    for (s = 0; s < fsg_model_n_state(fsg); s++) {
        lextree->root[s] =
            fsg_psubtree_init(lextree, fsg, s, &(lextree->alloc_head[s]));

        for (pn = lextree->alloc_head[s]; pn; pn = pn->alloc_next) {
            lextree->n_pnode++;
            if (pn->leaf)
                ++n_leaves;
        }
    }
    E_INFO("%d HMM nodes in lextree (%d leaves)\n",
           lextree->n_pnode, n_leaves);
    E_INFO("Allocated %d bytes (%d KiB) for all lextree nodes\n",
           lextree->n_pnode * sizeof(fsg_pnode_t),
           lextree->n_pnode * sizeof(fsg_pnode_t) / 1024);
    E_INFO("Allocated %d bytes (%d KiB) for lextree leafnodes\n",
           n_leaves * sizeof(fsg_pnode_t),
           n_leaves * sizeof(fsg_pnode_t) / 1024);

#if __FSG_DBG__
    fsg_lextree_dump(lextree, stdout);
#endif

    return lextree;
}


void
fsg_lextree_dump(fsg_lextree_t * lextree, FILE * fp)
{
    int32 s;

    for (s = 0; s < fsg_model_n_state(lextree->fsg); s++) {
        fprintf(fp, "State %5d root %p\n", s, lextree->root[s]);
        fsg_psubtree_dump(lextree, lextree->root[s], fp);
    }
    fflush(fp);
}


void
fsg_lextree_free(fsg_lextree_t * lextree)
{
    int32 s;

    if (lextree == NULL)
        return;

    if (lextree->fsg)
        for (s = 0; s < fsg_model_n_state(lextree->fsg); s++)
            fsg_psubtree_free(lextree->alloc_head[s]);

    ckd_free_2d(lextree->lc);
    ckd_free_2d(lextree->rc);
    ckd_free(lextree->root);
    ckd_free(lextree->alloc_head);
    ckd_free(lextree);
}





void fsg_glist_linklist_free(fsg_glist_linklist_t *glist)
{
    if (glist) {
        fsg_glist_linklist_t *nxtglist;
        if (glist->glist)
            glist_free(glist->glist);
        nxtglist = glist->next;
        while (nxtglist) {
            ckd_free(glist);
            glist = nxtglist;
            if (glist->glist)
                glist_free(glist->glist);
            nxtglist = glist->next;
        }
        ckd_free(glist);
    }
    return;
}

void
fsg_pnode_add_all_ctxt(fsg_pnode_ctxt_t * ctxt)
{
    int32 i;

    for (i = 0; i < FSG_PNODE_CTXT_BVSZ; i++)
        ctxt->bv[i] = 0xffffffff;
}

uint32 fsg_pnode_ctxt_sub_generic(fsg_pnode_ctxt_t *src, fsg_pnode_ctxt_t *sub)
{
    int32 i;
    uint32 res = 0;
    
    for (i = 0; i < FSG_PNODE_CTXT_BVSZ; i++)
        res |= (src->bv[i] = ~(sub->bv[i]) & src->bv[i]);
    return res;
}


















static fsg_pnode_t *
psubtree_add_trans(fsg_lextree_t *lextree, 
                   fsg_pnode_t * root,
                   fsg_glist_linklist_t **curglist,
                   fsg_link_t * fsglink,
                   int16 *lclist, int16 *rclist,
                   fsg_pnode_t ** alloc_head)
{
    int32 silcipid;             
    int32 pronlen;              
    int32 wid;                  
    int32 dictwid;              
    int32 ssid;                 
    int32 tmatid;
    gnode_t *gn;
    fsg_pnode_t *pnode, *pred, *head;
    int32 n_ci, p, lc, rc;
    glist_t lc_pnodelist;       
    glist_t rc_pnodelist;       
    int32 i, j;
    int n_lc_alloc = 0, n_int_alloc = 0, n_rc_alloc = 0;

    silcipid = bin_mdef_silphone(lextree->mdef);
    n_ci = bin_mdef_n_ciphone(lextree->mdef);

    wid = fsg_link_wid(fsglink);
    assert(wid >= 0);           
    dictwid = dict_wordid(lextree->dict,
                          fsg_model_word_str(lextree->fsg, wid));
    pronlen = dict_pronlen(lextree->dict, dictwid);
    assert(pronlen >= 1);

    assert(lclist[0] >= 0);     
    assert(rclist[0] >= 0);

    head = *alloc_head;
    pred = NULL;

    if (pronlen == 1) {         
        int ci = dict_first_phone(lextree->dict, dictwid);
        
        if (!dict_filler_word(lextree->dict, dictwid)) {
            



            lc_pnodelist = NULL;

            for (i = 0; lclist[i] >= 0; i++) {
                lc = lclist[i];
                ssid = dict2pid_lrdiph_rc(lextree->d2p, ci, lc, silcipid);
                tmatid = bin_mdef_pid2tmatid(lextree->mdef, dict_first_phone(lextree->dict, dictwid));
                
                for (gn = lc_pnodelist; gn; gn = gnode_next(gn)) {
                    pnode = (fsg_pnode_t *) gnode_ptr(gn);

                    if (hmm_nonmpx_ssid(&pnode->hmm) == ssid) {
                        
                        fsg_pnode_add_ctxt(pnode, lc);
                        break;
                    }
                }

                if (!gn) {      
                    pnode =
                        (fsg_pnode_t *) ckd_calloc(1, sizeof(fsg_pnode_t));
                    pnode->ctx = lextree->ctx;
                    pnode->next.fsglink = fsglink;
                    pnode->logs2prob =
                        (fsg_link_logs2prob(fsglink) >> SENSCR_SHIFT)
                        + lextree->wip + lextree->pip;
                    pnode->ci_ext = dict_first_phone(lextree->dict, dictwid);
                    pnode->ppos = 0;
                    pnode->leaf = TRUE;
                    pnode->sibling = root;      
                    fsg_pnode_add_ctxt(pnode, lc);      
                    pnode->alloc_next = head;
                    head = pnode;
                    root = pnode;
                    ++n_lc_alloc;

                    hmm_init(lextree->ctx, &pnode->hmm, FALSE, ssid, tmatid);

                    lc_pnodelist =
                        glist_add_ptr(lc_pnodelist, (void *) pnode);
                }
            }

            glist_free(lc_pnodelist);
        }
        else {                  
            ssid = bin_mdef_pid2ssid(lextree->mdef, ci); 
            tmatid = bin_mdef_pid2tmatid(lextree->mdef, ci);

            pnode = (fsg_pnode_t *) ckd_calloc(1, sizeof(fsg_pnode_t));
            pnode->ctx = lextree->ctx;
            pnode->next.fsglink = fsglink;
            pnode->logs2prob = (fsg_link_logs2prob(fsglink) >> SENSCR_SHIFT)
                + lextree->wip + lextree->pip;
            pnode->ci_ext = silcipid;   
            pnode->ppos = 0;
            pnode->leaf = TRUE;
            pnode->sibling = root;
            fsg_pnode_add_all_ctxt(&(pnode->ctxt));
            pnode->alloc_next = head;
            head = pnode;
            root = pnode;
            ++n_int_alloc;

            hmm_init(lextree->ctx, &pnode->hmm, FALSE, ssid, tmatid);
        }
    }
    else {                      
        fsg_pnode_t **ssid_pnode_map;       
        ssid_pnode_map =
            (fsg_pnode_t **) ckd_calloc(n_ci, sizeof(fsg_pnode_t *));
        lc_pnodelist = NULL;
        rc_pnodelist = NULL;

        for (p = 0; p < pronlen; p++) {
            int ci = dict_pron(lextree->dict, dictwid, p);
            if (p == 0) {       
                
		fsg_glist_linklist_t *glist;

                rc = dict_pron(lextree->dict, dictwid, 1);
		for (glist = *curglist;
                     glist && glist->glist;
                     glist = glist->next) {
		    if (glist->ci == ci && glist->rc == rc)
			break;
		}
		if (glist && glist->glist) {
		    assert(glist->ci == ci && glist->rc == rc);
		    
                    E_DEBUG(2,("Found match for (%d,%d)\n", ci, rc));
		    lc_pnodelist = glist->glist;
                    
		    pred = (fsg_pnode_t *) gnode_ptr(lc_pnodelist);
		    continue;
		}
		else {
		    



		    if (glist == NULL) { 
		        glist = (fsg_glist_linklist_t*) ckd_calloc(1, sizeof(fsg_glist_linklist_t));
			glist->next = *curglist;
                        *curglist = glist;
		    }
		    glist->ci = ci;
                    glist->rc = rc;
		    lc_pnodelist = glist->glist = NULL; 
		}

                for (i = 0; lclist[i] >= 0; i++) {
                    lc = lclist[i];
                    ssid = dict2pid_ldiph_lc(lextree->d2p, ci, rc, lc);
                    tmatid = bin_mdef_pid2tmatid(lextree->mdef, dict_first_phone(lextree->dict, dictwid));
                    


                    pnode = ssid_pnode_map[0];
                    for (j = 0; j < n_ci && ssid_pnode_map[j] != NULL; ++j) {
                        pnode = ssid_pnode_map[j];
                        if (hmm_nonmpx_ssid(&pnode->hmm) == ssid)
                            break;
                    }
                    assert(j < n_ci);
                    if (!pnode) {       
                        pnode =
                            (fsg_pnode_t *) ckd_calloc(1,
                                                       sizeof
                                                       (fsg_pnode_t));
                        pnode->ctx = lextree->ctx;
	                
                        

                        pnode->logs2prob = lextree->wip + lextree->pip;
                        pnode->ci_ext = dict_first_phone(lextree->dict, dictwid);
                        pnode->ppos = 0;
                        pnode->leaf = FALSE;
                        pnode->sibling = root;  
                        pnode->alloc_next = head;
                        head = pnode;
                        root = pnode;
                        ++n_lc_alloc;

                        hmm_init(lextree->ctx, &pnode->hmm, FALSE, ssid, tmatid);

                        lc_pnodelist =
                            glist_add_ptr(lc_pnodelist, (void *) pnode);
                        ssid_pnode_map[j] = pnode;
                    }
                    fsg_pnode_add_ctxt(pnode, lc);
                }
		
		glist->glist = lc_pnodelist;

                
		pred = root;
            }
            else if (p != pronlen - 1) {        
                fsg_pnode_t    *pnodeyoungest;

                ssid = dict2pid_internal(lextree->d2p, dictwid, p);
                tmatid = bin_mdef_pid2tmatid(lextree->mdef, dict_pron (lextree->dict, dictwid, p));
	        
		pnode = pred->next.succ;
		pnodeyoungest = pnode; 
		while (pnode && (hmm_nonmpx_ssid(&pnode->hmm) != ssid || pnode->leaf)) {
		    pnode = pnode->sibling;
		}
		if (pnode && (hmm_nonmpx_ssid(&pnode->hmm) == ssid && !pnode->leaf)) {
		    
                    E_DEBUG(2,("Found match for %d\n", ci));
		    pred = pnode;
		    continue;
		}

		
                pnode = (fsg_pnode_t *) ckd_calloc(1, sizeof(fsg_pnode_t));
                pnode->ctx = lextree->ctx;
                pnode->logs2prob = lextree->pip;
                pnode->ci_ext = dict_pron(lextree->dict, dictwid, p);
                pnode->ppos = p;
                pnode->leaf = FALSE;
                pnode->sibling = pnodeyoungest; 
                if (p == 1) {   
                    for (gn = lc_pnodelist; gn; gn = gnode_next(gn)) {
                        pred = (fsg_pnode_t *) gnode_ptr(gn);
                        pred->next.succ = pnode;
                    }
                }
                else {          
                    pred->next.succ = pnode;
                }
                pnode->alloc_next = head;
                head = pnode;
                ++n_int_alloc;

                hmm_init(lextree->ctx, &pnode->hmm, FALSE, ssid, tmatid);

                pred = pnode;
            }
            else {              
	        
                xwdssid_t *rssid;
                memset((void *) ssid_pnode_map, 0,
                       n_ci * sizeof(fsg_pnode_t *));
                lc = dict_pron(lextree->dict, dictwid, p-1);
                rssid = dict2pid_rssid(lextree->d2p, ci, lc);
                tmatid = bin_mdef_pid2tmatid(lextree->mdef, dict_pron (lextree->dict, dictwid, p));

                for (i = 0; rclist[i] >= 0; i++) {
                    rc = rclist[i];

                    j = rssid->cimap[rc];
                    ssid = rssid->ssid[j];
                    pnode = ssid_pnode_map[j];

                    if (!pnode) {       
                        pnode =
                            (fsg_pnode_t *) ckd_calloc(1,
                                                       sizeof
                                                       (fsg_pnode_t));
                        pnode->ctx = lextree->ctx;
			
                        
                        pnode->logs2prob = (fsg_link_logs2prob(fsglink) >> SENSCR_SHIFT)
                            + lextree->pip;
                        pnode->ci_ext = dict_pron(lextree->dict, dictwid, p);
                        pnode->ppos = p;
                        pnode->leaf = TRUE;
                        pnode->sibling = rc_pnodelist ?
                            (fsg_pnode_t *) gnode_ptr(rc_pnodelist) : NULL;
                        pnode->next.fsglink = fsglink;
                        pnode->alloc_next = head;
                        head = pnode;
                        ++n_rc_alloc;

                        hmm_init(lextree->ctx, &pnode->hmm, FALSE, ssid, tmatid);

                        rc_pnodelist =
                            glist_add_ptr(rc_pnodelist, (void *) pnode);
                        ssid_pnode_map[j] = pnode;
                    }
                    else {
                        assert(hmm_nonmpx_ssid(&pnode->hmm) == ssid);
                    }
                    fsg_pnode_add_ctxt(pnode, rc);
                }

                if (p == 1) {   
                    for (gn = lc_pnodelist; gn; gn = gnode_next(gn)) {
                        pred = (fsg_pnode_t *) gnode_ptr(gn);
                        if (!pred->next.succ)
                            pred->next.succ = (fsg_pnode_t *) gnode_ptr(rc_pnodelist);
                        else {
                            
                            fsg_pnode_t *succ = pred->next.succ;
                            while (succ->sibling) succ = succ->sibling;
                            succ->sibling = (fsg_pnode_t*) gnode_ptr(rc_pnodelist);
                            

                            break; 
                        }
                    }
                }
                else {          
                    if (!pred->next.succ)
                        pred->next.succ = (fsg_pnode_t *) gnode_ptr(rc_pnodelist);
                    else {
                        
                        fsg_pnode_t *succ = pred->next.succ;
                        while (succ->sibling) succ = succ->sibling;
                        succ->sibling = (fsg_pnode_t *) gnode_ptr(rc_pnodelist);
                    }
                }
            }
        }

        ckd_free((void *) ssid_pnode_map);
        
        glist_free(rc_pnodelist);
    }

    E_DEBUG(2,("Allocated %d HMMs (%d lc, %d rc, %d internal)\n",
               n_lc_alloc + n_rc_alloc + n_int_alloc,
               n_lc_alloc, n_rc_alloc, n_int_alloc));
    *alloc_head = head;

    return root;
}


static fsg_pnode_t *
fsg_psubtree_init(fsg_lextree_t *lextree,
                  fsg_model_t * fsg, int32 from_state,
                  fsg_pnode_t ** alloc_head)
{
    fsg_arciter_t *itor;
    fsg_link_t *fsglink;
    fsg_pnode_t *root;
    int32 n_ci, n_arc;
    fsg_glist_linklist_t *glist = NULL;

    root = NULL;
    assert(*alloc_head == NULL);

    n_ci = bin_mdef_n_ciphone(lextree->mdef);
    if (n_ci > (FSG_PNODE_CTXT_BVSZ * 32)) {
        E_FATAL
            ("#phones > %d; increase FSG_PNODE_CTXT_BVSZ and recompile\n",
             FSG_PNODE_CTXT_BVSZ * 32);
    }

    n_arc = 0;
    for (itor = fsg_model_arcs(fsg, from_state); itor; 
         itor = fsg_arciter_next(itor)) {
        int32 dst;
        fsglink = fsg_arciter_get(itor);
        dst = fsglink->to_state;

        if (fsg_link_wid(fsglink) < 0)
            continue;

        E_DEBUG(2,("Building lextree for arc from %d to %d: %s\n",
                   from_state, dst, fsg_model_word_str(fsg, fsg_link_wid(fsglink))));
        root = psubtree_add_trans(lextree, root, &glist, fsglink,
                                  lextree->lc[from_state],
                                  lextree->rc[dst],
                                  alloc_head);
        ++n_arc;
    }
    E_DEBUG(2,("State %d has %d outgoing arcs\n", from_state, n_arc));

    fsg_glist_linklist_free(glist);

    return root;
}


static void
fsg_psubtree_free(fsg_pnode_t * head)
{
    fsg_pnode_t *next;

    while (head) {
        next = head->alloc_next;
        hmm_deinit(&head->hmm);
        ckd_free(head);
        head = next;
    }
}

void fsg_psubtree_dump_node(fsg_lextree_t *tree, fsg_pnode_t *node, FILE *fp)
{    
    int32 i;
    fsg_link_t *tl;

    
    for (i = 0; i <= node->ppos; i++)
        fprintf(fp, "  ");

    fprintf(fp, "%p.@", node);    

    fprintf(fp, " %5d.SS", hmm_nonmpx_ssid(&node->hmm));
    fprintf(fp, " %10d.LP", node->logs2prob);
    fprintf(fp, " %p.SIB", node->sibling);
    fprintf(fp, " %s.%d", bin_mdef_ciphone_str(tree->mdef, node->ci_ext), node->ppos);
    if ((node->ppos == 0) || node->leaf) {
        fprintf(fp, " [");
        for (i = 0; i < FSG_PNODE_CTXT_BVSZ; i++)
            fprintf(fp, "%08x", node->ctxt.bv[i]);
        fprintf(fp, "]");
    }
    if (node->leaf) {
        tl = node->next.fsglink;
        fprintf(fp, " {%s[%d->%d](%d)}",
                fsg_model_word_str(tree->fsg, tl->wid),
                tl->from_state, tl->to_state, tl->logs2prob);
    } else {
        fprintf(fp, " %p.NXT", node->next.succ);
    }
    fprintf(fp, "\n");

    return;
}

void 
fsg_psubtree_dump(fsg_lextree_t *tree, fsg_pnode_t *root, FILE * fp)
{
    fsg_pnode_t *succ;

    if (root == NULL) return;
    if (root->ppos == 0) {
        while(root->sibling && root->sibling->next.succ == root->next.succ) {
            fsg_psubtree_dump_node(tree, root, fp);
            root = root->sibling;
        }
        fflush(fp);
    }
    
    fsg_psubtree_dump_node(tree, root, fp);

    if (root->leaf) {
        if (root->ppos == 0 && root->sibling) { 
            fsg_psubtree_dump(tree, root->sibling,fp);
        }
        return;
    }

    succ = root->next.succ;
    while(succ) {
        fsg_psubtree_dump(tree, succ,fp);
        succ = succ->sibling;
    }

    if (root->ppos == 0) {
        fsg_psubtree_dump(tree, root->sibling,fp);
        fflush(fp);
    }

    return;
}

void
fsg_psubtree_pnode_deactivate(fsg_pnode_t * pnode)
{
    hmm_clear(&pnode->hmm);
}
