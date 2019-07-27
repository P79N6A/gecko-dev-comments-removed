


















































#include <stdio.h>
#include <string.h>
#include <assert.h>


#include <sphinxbase/err.h>
#include <sphinxbase/ckd_alloc.h>
#include <sphinxbase/strfuncs.h>
#include <sphinxbase/cmd_ln.h>


#include "pocketsphinx_internal.h"
#include "ps_lattice_internal.h"
#include "fsg_search_internal.h"
#include "fsg_history.h"
#include "fsg_lextree.h"


#define __FSG_DBG__		0
#define __FSG_DBG_CHAN__	0

static ps_seg_t *fsg_search_seg_iter(ps_search_t *search, int32 *out_score);
static ps_lattice_t *fsg_search_lattice(ps_search_t *search);
static int fsg_search_prob(ps_search_t *search);

static ps_searchfuncs_t fsg_funcs = {
       "fsg",
      fsg_search_start,
       fsg_search_step,
     fsg_search_finish,
     fsg_search_reinit,
       fsg_search_free,
      fsg_search_lattice,
          fsg_search_hyp,
         fsg_search_prob,
     fsg_search_seg_iter,
};

static int
fsg_search_add_silences(fsg_search_t *fsgs, fsg_model_t *fsg)
{
    dict_t *dict;
    int32 wid;
    int n_sil;

    dict = ps_search_dict(fsgs);
    











    
    fsg_model_add_silence(fsg, "<sil>", -1,
                          cmd_ln_float32_r(ps_search_config(fsgs), "-silprob"));
    n_sil = 0;
    
    for (wid = dict_filler_start(dict); wid < dict_filler_end(dict); ++wid) {
        char const *word = dict_wordstr(dict, wid);
        if (wid == dict_startwid(dict) || wid == dict_finishwid(dict))
            continue;
        fsg_model_add_silence(fsg, word, -1,
                              cmd_ln_float32_r(ps_search_config(fsgs), "-fillprob"));
        ++n_sil;
    }

    return n_sil;
}


static int
fsg_search_check_dict(fsg_search_t *fsgs, fsg_model_t *fsg)
{
    dict_t *dict;
    int i;

    dict = ps_search_dict(fsgs);
    for (i = 0; i < fsg_model_n_word(fsg); ++i) {
        char const *word;
        int32 wid;

        word = fsg_model_word_str(fsg, i);
        wid = dict_wordid(dict, word);
        if (wid == BAD_S3WID) {
    	    E_ERROR("The word '%s' is missing in the dictionary\n", word);
    	    return FALSE;
    	}
    }

    return TRUE;
}

static int
fsg_search_add_altpron(fsg_search_t *fsgs, fsg_model_t *fsg)
{
    dict_t *dict;
    int n_alt, n_word;
    int i;

    dict = ps_search_dict(fsgs);
    
    n_alt = 0;
    n_word = fsg_model_n_word(fsg);
    for (i = 0; i < n_word; ++i) {
        char const *word;
        int32 wid;

        word = fsg_model_word_str(fsg, i);
        wid = dict_wordid(dict, word);
        if (wid != BAD_S3WID) {
            while ((wid = dict_nextalt(dict, wid)) != BAD_S3WID) {
	        n_alt += fsg_model_add_alt(fsg, word, dict_wordstr(dict, wid));
    	    }
    	}
    }

    E_INFO("Added %d alternate word transitions\n", n_alt);
    return n_alt;
}

ps_search_t *
fsg_search_init(fsg_model_t *fsg,
                cmd_ln_t *config,
                acmod_t *acmod,
                dict_t *dict,
                dict2pid_t *d2p)
{
    fsg_search_t *fsgs = ckd_calloc(1, sizeof(*fsgs));
    ps_search_init(ps_search_base(fsgs), &fsg_funcs, config, acmod, dict, d2p);

    fsgs->fsg = fsg_model_retain(fsg);
    
    fsgs->hmmctx = hmm_context_init(bin_mdef_n_emit_state(acmod->mdef),
                                    acmod->tmat->tp, NULL, acmod->mdef->sseq);
    if (fsgs->hmmctx == NULL) {
        ps_search_free(ps_search_base(fsgs));
        return NULL;
    }

    
    fsgs->history = fsg_history_init(NULL, dict);
    fsgs->frame = -1;

    
    fsgs->beam_factor = 1.0f;
    fsgs->beam = fsgs->beam_orig
        = (int32) logmath_log(acmod->lmath, cmd_ln_float64_r(config, "-beam"))
        >> SENSCR_SHIFT;
    fsgs->pbeam = fsgs->pbeam_orig
        = (int32) logmath_log(acmod->lmath, cmd_ln_float64_r(config, "-pbeam"))
        >> SENSCR_SHIFT;
    fsgs->wbeam = fsgs->wbeam_orig
        = (int32) logmath_log(acmod->lmath, cmd_ln_float64_r(config, "-wbeam"))
        >> SENSCR_SHIFT;

    
    fsgs->lw = cmd_ln_float32_r(config, "-lw");
    fsgs->pip = (int32) (logmath_log(acmod->lmath, cmd_ln_float32_r(config, "-pip"))
                           * fsgs->lw)
        >> SENSCR_SHIFT;
    fsgs->wip = (int32) (logmath_log(acmod->lmath, cmd_ln_float32_r(config, "-wip"))
                           * fsgs->lw)
        >> SENSCR_SHIFT;

    
    fsgs->ascale = 1.0 / cmd_ln_float32_r(config, "-ascale");

    E_INFO("FSG(beam: %d, pbeam: %d, wbeam: %d; wip: %d, pip: %d)\n",
           fsgs->beam_orig, fsgs->pbeam_orig, fsgs->wbeam_orig,
           fsgs->wip, fsgs->pip);

    if (!fsg_search_check_dict(fsgs, fsg)) {
        fsg_search_free(ps_search_base(fsgs));
        return NULL;
    }

    if (cmd_ln_boolean_r(config, "-fsgusefiller") &&
        !fsg_model_has_sil(fsg))
        fsg_search_add_silences(fsgs, fsg);

    if (cmd_ln_boolean_r(config, "-fsgusealtpron") &&
        !fsg_model_has_alt(fsg))
        fsg_search_add_altpron(fsgs, fsg);

    if (fsg_search_reinit(ps_search_base(fsgs),
                          ps_search_dict(fsgs),
                          ps_search_dict2pid(fsgs)) < 0)
    {
        ps_search_free(ps_search_base(fsgs));
        return NULL;
    }
        
    return ps_search_base(fsgs);
}

void
fsg_search_free(ps_search_t *search)
{
    fsg_search_t *fsgs = (fsg_search_t *)search;

    ps_search_deinit(search);
    fsg_lextree_free(fsgs->lextree);
    if (fsgs->history) {
        fsg_history_reset(fsgs->history);
        fsg_history_set_fsg(fsgs->history, NULL, NULL);
        fsg_history_free(fsgs->history);
    }
    hmm_context_free(fsgs->hmmctx);
    fsg_model_free(fsgs->fsg);
    ckd_free(fsgs);
}

int
fsg_search_reinit(ps_search_t *search, dict_t *dict, dict2pid_t *d2p)
{
    fsg_search_t *fsgs = (fsg_search_t *)search;

    
    if (fsgs->lextree)
        fsg_lextree_free(fsgs->lextree);

    
    ps_search_base_reinit(search, dict, d2p);
    
    
    search->n_words = dict_size(dict);

    
    fsgs->lextree = fsg_lextree_init(fsgs->fsg, dict, d2p,
                                     ps_search_acmod(fsgs)->mdef,
                                     fsgs->hmmctx, fsgs->wip, fsgs->pip);

    
    fsg_history_set_fsg(fsgs->history, fsgs->fsg, dict);

    return 0;
}


static void
fsg_search_sen_active(fsg_search_t *fsgs)
{
    gnode_t *gn;
    fsg_pnode_t *pnode;
    hmm_t *hmm;

    acmod_clear_active(ps_search_acmod(fsgs));

    for (gn = fsgs->pnode_active; gn; gn = gnode_next(gn)) {
        pnode = (fsg_pnode_t *) gnode_ptr(gn);
        hmm = fsg_pnode_hmmptr(pnode);
        assert(hmm_frame(hmm) == fsgs->frame);
        acmod_activate_hmm(ps_search_acmod(fsgs), hmm);
    }
}






static void
fsg_search_hmm_eval(fsg_search_t *fsgs)
{
    gnode_t *gn;
    fsg_pnode_t *pnode;
    hmm_t *hmm;
    int32 bestscore;
    int32 n, maxhmmpf;

    bestscore = WORST_SCORE;

    if (!fsgs->pnode_active) {
        E_ERROR("Frame %d: No active HMM!!\n", fsgs->frame);
        return;
    }

    for (n = 0, gn = fsgs->pnode_active; gn; gn = gnode_next(gn), n++) {
        int32 score;

        pnode = (fsg_pnode_t *) gnode_ptr(gn);
        hmm = fsg_pnode_hmmptr(pnode);
        assert(hmm_frame(hmm) == fsgs->frame);

#if __FSG_DBG__
        E_INFO("pnode(%08x) active @frm %5d\n", (int32) pnode,
               fsgs->frame);
        hmm_dump(hmm, stdout);
#endif
        score = hmm_vit_eval(hmm);
#if __FSG_DBG_CHAN__
        E_INFO("pnode(%08x) after eval @frm %5d\n",
               (int32) pnode, fsgs->frame);
        hmm_dump(hmm, stdout);
#endif

        if (score BETTER_THAN bestscore)
            bestscore = score;
    }

#if __FSG_DBG__
    E_INFO("[%5d] %6d HMM; bestscr: %11d\n", fsgs->frame, n, bestscore);
#endif
    fsgs->n_hmm_eval += n;

    
    maxhmmpf = cmd_ln_int32_r(ps_search_config(fsgs), "-maxhmmpf");
    if (maxhmmpf != -1 && n > maxhmmpf) {
        



        if (fsgs->beam_factor > 0.1) {        
            fsgs->beam_factor *= 0.9f;        
            fsgs->beam =
                (int32) (fsgs->beam_orig * fsgs->beam_factor);
            fsgs->pbeam =
                (int32) (fsgs->pbeam_orig * fsgs->beam_factor);
            fsgs->wbeam =
                (int32) (fsgs->wbeam_orig * fsgs->beam_factor);
        }
    }
    else {
        fsgs->beam_factor = 1.0f;
        fsgs->beam = fsgs->beam_orig;
        fsgs->pbeam = fsgs->pbeam_orig;
        fsgs->wbeam = fsgs->wbeam_orig;
    }

    if (n > fsg_lextree_n_pnode(fsgs->lextree))
        E_FATAL("PANIC! Frame %d: #HMM evaluated(%d) > #PNodes(%d)\n",
                fsgs->frame, n, fsg_lextree_n_pnode(fsgs->lextree));

    fsgs->bestscore = bestscore;
}


static void
fsg_search_pnode_trans(fsg_search_t *fsgs, fsg_pnode_t * pnode)
{
    fsg_pnode_t *child;
    hmm_t *hmm;
    int32 newscore, thresh, nf;

    assert(pnode);
    assert(!fsg_pnode_leaf(pnode));

    nf = fsgs->frame + 1;
    thresh = fsgs->bestscore + fsgs->beam;

    hmm = fsg_pnode_hmmptr(pnode);

    for (child = fsg_pnode_succ(pnode);
         child; child = fsg_pnode_sibling(child)) {
        newscore = hmm_out_score(hmm) + child->logs2prob;

        if ((newscore BETTER_THAN thresh)
            && (newscore BETTER_THAN hmm_in_score(&child->hmm))) {
            
            if (hmm_frame(&child->hmm) < nf) {
                
                fsgs->pnode_active_next =
                    glist_add_ptr(fsgs->pnode_active_next,
                                  (void *) child);
            }

            hmm_enter(&child->hmm, newscore, hmm_out_history(hmm), nf);
        }
    }
}


static void
fsg_search_pnode_exit(fsg_search_t *fsgs, fsg_pnode_t * pnode)
{
    hmm_t *hmm;
    fsg_link_t *fl;
    int32 wid;
    fsg_pnode_ctxt_t ctxt;

    assert(pnode);
    assert(fsg_pnode_leaf(pnode));

    hmm = fsg_pnode_hmmptr(pnode);
    fl = fsg_pnode_fsglink(pnode);
    assert(fl);

    wid = fsg_link_wid(fl);
    assert(wid >= 0);

#if __FSG_DBG__
    E_INFO("[%5d] Exit(%08x) %10d(score) %5d(pred)\n",
           fsgs->frame, (int32) pnode,
           hmm_out_score(hmm), hmm_out_history(hmm));
#endif

    



    if (fsg_model_is_filler(fsgs->fsg, wid)
        
        || (dict_is_single_phone(ps_search_dict(fsgs),
                                   dict_wordid(ps_search_dict(fsgs),
                                               fsg_model_word_str(fsgs->fsg, wid))))) {
        
        fsg_pnode_add_all_ctxt(&ctxt);

        
        fsg_history_entry_add(fsgs->history,
                              fl,
                              fsgs->frame,
                              hmm_out_score(hmm),
                              hmm_out_history(hmm),
                              pnode->ci_ext, ctxt);

    }
    else {
        
        fsg_history_entry_add(fsgs->history,
                              fl,
                              fsgs->frame,
                              hmm_out_score(hmm),
                              hmm_out_history(hmm),
                              pnode->ci_ext, pnode->ctxt);
    }
}








static void
fsg_search_hmm_prune_prop(fsg_search_t *fsgs)
{
    gnode_t *gn;
    fsg_pnode_t *pnode;
    hmm_t *hmm;
    int32 thresh, word_thresh, phone_thresh;

    assert(fsgs->pnode_active_next == NULL);

    thresh = fsgs->bestscore + fsgs->beam;
    phone_thresh = fsgs->bestscore + fsgs->pbeam;
    word_thresh = fsgs->bestscore + fsgs->wbeam;

    for (gn = fsgs->pnode_active; gn; gn = gnode_next(gn)) {
        pnode = (fsg_pnode_t *) gnode_ptr(gn);
        hmm = fsg_pnode_hmmptr(pnode);

        if (hmm_bestscore(hmm) >= thresh) {
            
            if (hmm_frame(hmm) == fsgs->frame) {
                hmm_frame(hmm) = fsgs->frame + 1;
                fsgs->pnode_active_next =
                    glist_add_ptr(fsgs->pnode_active_next,
                                  (void *) pnode);
            }
            else {
                assert(hmm_frame(hmm) == fsgs->frame + 1);
            }

            if (!fsg_pnode_leaf(pnode)) {
                if (hmm_out_score(hmm) >= phone_thresh) {
                    
                    fsg_search_pnode_trans(fsgs, pnode);
                }
            }
            else {
                if (hmm_out_score(hmm) >= word_thresh) {
                    
                    fsg_search_pnode_exit(fsgs, pnode);
                }
            }
        }
    }
}





static void
fsg_search_null_prop(fsg_search_t *fsgs)
{
    int32 bpidx, n_entries, thresh, newscore;
    fsg_hist_entry_t *hist_entry;
    fsg_link_t *l;
    int32 s;
    fsg_model_t *fsg;

    fsg = fsgs->fsg;
    thresh = fsgs->bestscore + fsgs->wbeam; 

    n_entries = fsg_history_n_entries(fsgs->history);

    for (bpidx = fsgs->bpidx_start; bpidx < n_entries; bpidx++) {
        fsg_arciter_t *itor;
        hist_entry = fsg_history_entry_get(fsgs->history, bpidx);

        l = fsg_hist_entry_fsglink(hist_entry);

        
        s = l ? fsg_link_to_state(l) : fsg_model_start_state(fsg);

        




        
        for (itor = fsg_model_arcs(fsg, s); itor;
             itor = fsg_arciter_next(itor)) {
            fsg_link_t *l = fsg_arciter_get(itor);

            
            if (fsg_link_wid(l) != -1)
                continue;
            newscore =
                fsg_hist_entry_score(hist_entry) +
                (fsg_link_logs2prob(l) >> SENSCR_SHIFT);

            if (newscore >= thresh) {
                fsg_history_entry_add(fsgs->history, l,
                                      fsg_hist_entry_frame(hist_entry),
                                      newscore,
                                      bpidx,
                                      fsg_hist_entry_lc(hist_entry),
                                      fsg_hist_entry_rc(hist_entry));
            }
        }
    }
}






static void
fsg_search_word_trans(fsg_search_t *fsgs)
{
    int32 bpidx, n_entries;
    fsg_hist_entry_t *hist_entry;
    fsg_link_t *l;
    int32 score, newscore, thresh, nf, d;
    fsg_pnode_t *root;
    int32 lc, rc;

    n_entries = fsg_history_n_entries(fsgs->history);

    thresh = fsgs->bestscore + fsgs->beam;
    nf = fsgs->frame + 1;

    for (bpidx = fsgs->bpidx_start; bpidx < n_entries; bpidx++) {
        hist_entry = fsg_history_entry_get(fsgs->history, bpidx);
        assert(hist_entry);
        score = fsg_hist_entry_score(hist_entry);
        assert(fsgs->frame == fsg_hist_entry_frame(hist_entry));

        l = fsg_hist_entry_fsglink(hist_entry);

        
        d = l ? fsg_link_to_state(l) : fsg_model_start_state(fsgs->
                                                                fsg);

        lc = fsg_hist_entry_lc(hist_entry);

        
        for (root = fsg_lextree_root(fsgs->lextree, d);
             root; root = root->sibling) {
            rc = root->ci_ext;

            if ((root->ctxt.bv[lc >> 5] & (1 << (lc & 0x001f))) &&
                (hist_entry->rc.bv[rc >> 5] & (1 << (rc & 0x001f)))) {
                






                newscore = score + root->logs2prob;

                if ((newscore BETTER_THAN thresh)
                    && (newscore BETTER_THAN hmm_in_score(&root->hmm))) {
                    if (hmm_frame(&root->hmm) < nf) {
                        
                        fsgs->pnode_active_next =
                            glist_add_ptr(fsgs->pnode_active_next,
                                          (void *) root);
#if __FSG_DBG__
                        E_INFO
                            ("[%5d] WordTrans bpidx[%d] -> pnode[%08x] (activated)\n",
                             fsgs->frame, bpidx, (int32) root);
#endif
                    }
                    else {
#if __FSG_DBG__
                        E_INFO
                            ("[%5d] WordTrans bpidx[%d] -> pnode[%08x]\n",
                             fsgs->frame, bpidx, (int32) root);
#endif
                    }

                    hmm_enter(&root->hmm, newscore, bpidx, nf);
                }
            }
        }
    }
}


int
fsg_search_step(ps_search_t *search, int frame_idx)
{
    fsg_search_t *fsgs = (fsg_search_t *)search;
    int16 const *senscr;
    acmod_t *acmod = search->acmod;
    gnode_t *gn;
    fsg_pnode_t *pnode;
    hmm_t *hmm;

    
    if (!acmod->compallsen)
        fsg_search_sen_active(fsgs);
    
    senscr = acmod_score(acmod, &frame_idx);
    fsgs->n_sen_eval += acmod->n_senone_active;
    hmm_context_set_senscore(fsgs->hmmctx, senscr);

    
    fsgs->bpidx_start = fsg_history_n_entries(fsgs->history);

    
    fsg_search_hmm_eval(fsgs);

    




    fsg_search_hmm_prune_prop(fsgs);
    fsg_history_end_frame(fsgs->history);

    



    fsg_search_null_prop(fsgs);
    fsg_history_end_frame(fsgs->history);

    



    fsg_search_word_trans(fsgs);

    





    for (gn = fsgs->pnode_active; gn; gn = gnode_next(gn)) {
        pnode = (fsg_pnode_t *) gnode_ptr(gn);
        hmm = fsg_pnode_hmmptr(pnode);

        if (hmm_frame(hmm) == fsgs->frame) {
            
            fsg_psubtree_pnode_deactivate(pnode);
        }
        else {
            assert(hmm_frame(hmm) == (fsgs->frame + 1));
        }
    }

    
    glist_free(fsgs->pnode_active);

    
    fsgs->pnode_active = fsgs->pnode_active_next;
    fsgs->pnode_active_next = NULL;

    
    ++fsgs->frame;

    return 1;
}







int
fsg_search_start(ps_search_t *search)
{
    fsg_search_t *fsgs = (fsg_search_t *)search;
    int32 silcipid;
    fsg_pnode_ctxt_t ctxt;

    
    fsgs->beam_factor = 1.0f;
    fsgs->beam = fsgs->beam_orig;
    fsgs->pbeam = fsgs->pbeam_orig;
    fsgs->wbeam = fsgs->wbeam_orig;

    silcipid = bin_mdef_ciphone_id(ps_search_acmod(fsgs)->mdef, "SIL");

    
    assert(fsgs->pnode_active == NULL);
    assert(fsgs->pnode_active_next == NULL);

    fsg_history_reset(fsgs->history);
    fsg_history_utt_start(fsgs->history);
    fsgs->final = FALSE;

    
    fsg_pnode_add_all_ctxt(&ctxt);

    
    fsgs->frame = -1;
    fsgs->bestscore = 0;
    fsg_history_entry_add(fsgs->history,
                          NULL, -1, 0, -1, silcipid, ctxt);
    fsgs->bpidx_start = 0;

    
    fsg_search_null_prop(fsgs);

    
    fsg_search_word_trans(fsgs);

    
    fsgs->pnode_active = fsgs->pnode_active_next;
    fsgs->pnode_active_next = NULL;

    ++fsgs->frame;

    fsgs->n_hmm_eval = 0;
    fsgs->n_sen_eval = 0;

    return 0;
}




int
fsg_search_finish(ps_search_t *search)
{
    fsg_search_t *fsgs = (fsg_search_t *)search;
    gnode_t *gn;
    fsg_pnode_t *pnode;
    int32 n_hist;

    
    for (gn = fsgs->pnode_active; gn; gn = gnode_next(gn)) {
        pnode = (fsg_pnode_t *) gnode_ptr(gn);
        fsg_psubtree_pnode_deactivate(pnode);
    }
    for (gn = fsgs->pnode_active_next; gn; gn = gnode_next(gn)) {
        pnode = (fsg_pnode_t *) gnode_ptr(gn);
        fsg_psubtree_pnode_deactivate(pnode);
    }

    glist_free(fsgs->pnode_active);
    fsgs->pnode_active = NULL;
    glist_free(fsgs->pnode_active_next);
    fsgs->pnode_active_next = NULL;

    fsgs->final = TRUE;

    n_hist = fsg_history_n_entries(fsgs->history);
    E_INFO
        ("%d frames, %d HMMs (%d/fr), %d senones (%d/fr), %d history entries (%d/fr)\n\n",
         fsgs->frame, fsgs->n_hmm_eval,
         (fsgs->frame > 0) ? fsgs->n_hmm_eval / fsgs->frame : 0,
         fsgs->n_sen_eval,
         (fsgs->frame > 0) ? fsgs->n_sen_eval / fsgs->frame : 0,
         n_hist, (fsgs->frame > 0) ? n_hist / fsgs->frame : 0);

    return 0;
}

static int
fsg_search_find_exit(fsg_search_t *fsgs, int frame_idx, int final, int32 *out_score, int32* out_is_final)
{
    fsg_hist_entry_t *hist_entry = NULL;
    fsg_model_t *fsg;
    int bpidx, frm, last_frm, besthist;
    int32 bestscore;

    if (out_is_final)
	*out_is_final = FALSE;

    if (frame_idx == -1)
        frame_idx = fsgs->frame - 1;
    last_frm = frm = frame_idx;

    
    bpidx = fsg_history_n_entries(fsgs->history) - 1;
    while (bpidx > 0) {
        hist_entry = fsg_history_entry_get(fsgs->history, bpidx);
        if (fsg_hist_entry_frame(hist_entry) <= frame_idx) {
            frm = last_frm = fsg_hist_entry_frame(hist_entry);
            break;
        }
        bpidx--;
    }

    
    if (bpidx <= 0)
        return bpidx;

    
    bestscore = INT_MIN;
    besthist = -1;
    fsg = fsgs->fsg;
    while (frm == last_frm) {
        fsg_link_t *fl;
        int32 score;

        fl = fsg_hist_entry_fsglink(hist_entry);
        score = fsg_hist_entry_score(hist_entry);
        
        if (fl == NULL)
	    break;

	
	if (score == bestscore && fsg_link_to_state(fl) == fsg_model_final_state(fsg)) {
    	    besthist = bpidx;
	} else if (score BETTER_THAN bestscore) {
            
            if ((!final)
                || fsg_link_to_state(fl) == fsg_model_final_state(fsg)) {
                bestscore = score;
                besthist = bpidx;
            }
        }
        
        --bpidx;
        if (bpidx < 0)
            break;
        hist_entry = fsg_history_entry_get(fsgs->history, bpidx);
        frm = fsg_hist_entry_frame(hist_entry);
    }

    
    if (besthist == -1) {
        E_ERROR("Final result does not match the grammar in frame %d\n", frame_idx);
        return -1;
    }

    
    if (out_score)
        *out_score = bestscore;
    if (out_is_final) {
	fsg_link_t *fl;
	hist_entry = fsg_history_entry_get(fsgs->history, besthist);
	fl = fsg_hist_entry_fsglink(hist_entry);
	*out_is_final = (fsg_link_to_state(fl) == fsg_model_final_state(fsg));
    }
    return besthist;
}


static ps_latlink_t *
fsg_search_bestpath(ps_search_t *search, int32 *out_score, int backward)
{
    fsg_search_t *fsgs = (fsg_search_t *)search;

    if (search->last_link == NULL) {
        search->last_link = ps_lattice_bestpath(search->dag, NULL,
                                                1.0, fsgs->ascale);
        if (search->last_link == NULL)
            return NULL;
        

        if (search->post == 0)
            search->post = ps_lattice_posterior(search->dag, NULL, fsgs->ascale);
    }
    if (out_score)
        *out_score = search->last_link->path_scr + search->dag->final_node_ascr;
    return search->last_link;
}

char const *
fsg_search_hyp(ps_search_t *search, int32 *out_score, int32 *out_is_final)
{
    fsg_search_t *fsgs = (fsg_search_t *)search;
    dict_t *dict = ps_search_dict(search);
    char *c;
    size_t len;
    int bp, bpidx;

    
    bpidx = fsg_search_find_exit(fsgs, fsgs->frame, fsgs->final, out_score, out_is_final);
    
    if (bpidx <= 0) {
        return NULL;
    }

    
    if (fsgs->bestpath && fsgs->final) {
        ps_lattice_t *dag;
        ps_latlink_t *link;

        if ((dag = fsg_search_lattice(search)) == NULL) {
    	    E_WARN("Failed to obtain the lattice while bestpath enabled\n");
            return NULL;
        }
        if ((link = fsg_search_bestpath(search, out_score, FALSE)) == NULL) {
    	    E_WARN("Failed to find the bestpath in a lattice\n");
            return NULL;
        }
        return ps_lattice_hyp(dag, link);
    }

    bp = bpidx;
    len = 0;
    while (bp > 0) {
        fsg_hist_entry_t *hist_entry = fsg_history_entry_get(fsgs->history, bp);
        fsg_link_t *fl = fsg_hist_entry_fsglink(hist_entry);
        char const *baseword;
        int32 wid;

        bp = fsg_hist_entry_pred(hist_entry);
        wid = fsg_link_wid(fl);
        if (wid < 0 || fsg_model_is_filler(fsgs->fsg, wid))
            continue;
        baseword = dict_basestr(dict,
                                dict_wordid(dict,
                                            fsg_model_word_str(fsgs->fsg, wid)));
        len += strlen(baseword) + 1;
    }
    
    ckd_free(search->hyp_str);
    if (len == 0) {
	search->hyp_str = NULL;
	return search->hyp_str;
    }
    search->hyp_str = ckd_calloc(1, len);

    bp = bpidx;
    c = search->hyp_str + len - 1;
    while (bp > 0) {
        fsg_hist_entry_t *hist_entry = fsg_history_entry_get(fsgs->history, bp);
        fsg_link_t *fl = fsg_hist_entry_fsglink(hist_entry);
        char const *baseword;
        int32 wid;

        bp = fsg_hist_entry_pred(hist_entry);
        wid = fsg_link_wid(fl);
        if (wid < 0 || fsg_model_is_filler(fsgs->fsg, wid))
            continue;
        baseword = dict_basestr(dict,
                                dict_wordid(dict,
                                            fsg_model_word_str(fsgs->fsg, wid)));
        len = strlen(baseword);
        c -= len;
        memcpy(c, baseword, len);
        if (c > search->hyp_str) {
            --c;
            *c = ' ';
        }
    }

    return search->hyp_str;
}

static void
fsg_seg_bp2itor(ps_seg_t *seg, fsg_hist_entry_t *hist_entry)
{
    fsg_search_t *fsgs = (fsg_search_t *)seg->search;
    fsg_hist_entry_t *ph = NULL;
    int32 bp;

    if ((bp = fsg_hist_entry_pred(hist_entry)) >= 0)
        ph = fsg_history_entry_get(fsgs->history, bp);
    seg->word = fsg_model_word_str(fsgs->fsg, hist_entry->fsglink->wid);
    seg->ef = fsg_hist_entry_frame(hist_entry);
    seg->sf = ph ? fsg_hist_entry_frame(ph) + 1 : 0;
    
    if (seg->sf > seg->ef) seg->sf = seg->ef;
    seg->prob = 0; 
    
    seg->lback = 1;
    seg->lscr = fsg_link_logs2prob(hist_entry->fsglink) >> SENSCR_SHIFT;
    if (ph) {
        
        seg->ascr = hist_entry->score - ph->score - seg->lscr;
    }
    else
        seg->ascr = hist_entry->score - seg->lscr;
}


static void
fsg_seg_free(ps_seg_t *seg)
{
    fsg_seg_t *itor = (fsg_seg_t *)seg;
    ckd_free(itor->hist);
    ckd_free(itor);
}

static ps_seg_t *
fsg_seg_next(ps_seg_t *seg)
{
    fsg_seg_t *itor = (fsg_seg_t *)seg;

    if (++itor->cur == itor->n_hist) {
        fsg_seg_free(seg);
        return NULL;
    }

    fsg_seg_bp2itor(seg, itor->hist[itor->cur]);
    return seg;
}

static ps_segfuncs_t fsg_segfuncs = {
     fsg_seg_next,
     fsg_seg_free
};

static ps_seg_t *
fsg_search_seg_iter(ps_search_t *search, int32 *out_score)
{
    fsg_search_t *fsgs = (fsg_search_t *)search;
    fsg_seg_t *itor;
    int bp, bpidx, cur;

    bpidx = fsg_search_find_exit(fsgs, fsgs->frame, fsgs->final, out_score, NULL);
    
    if (bpidx <= 0)
        return NULL;

    
    if (fsgs->bestpath && fsgs->final) {
        ps_lattice_t *dag;
        ps_latlink_t *link;

        if ((dag = fsg_search_lattice(search)) == NULL)
            return NULL;
        if ((link = fsg_search_bestpath(search, out_score, TRUE)) == NULL)
            return NULL;
        return ps_lattice_seg_iter(dag, link, 1.0);
    }

    



    itor = ckd_calloc(1, sizeof(*itor));
    itor->base.vt = &fsg_segfuncs;
    itor->base.search = search;
    itor->base.lwf = 1.0;
    itor->n_hist = 0;
    bp = bpidx;
    while (bp > 0) {
        fsg_hist_entry_t *hist_entry = fsg_history_entry_get(fsgs->history, bp);
        bp = fsg_hist_entry_pred(hist_entry);
        ++itor->n_hist;
    }
    if (itor->n_hist == 0) {
        ckd_free(itor);
        return NULL;
    }
    itor->hist = ckd_calloc(itor->n_hist, sizeof(*itor->hist));
    cur = itor->n_hist - 1;
    bp = bpidx;
    while (bp > 0) {
        fsg_hist_entry_t *hist_entry = fsg_history_entry_get(fsgs->history, bp);
        itor->hist[cur] = hist_entry;
        bp = fsg_hist_entry_pred(hist_entry);
        --cur;
    }

    
    fsg_seg_bp2itor((ps_seg_t *)itor, itor->hist[0]);
    
    return (ps_seg_t *)itor;
}

static int
fsg_search_prob(ps_search_t *search)
{
    fsg_search_t *fsgs = (fsg_search_t *)search;

    
    if (fsgs->bestpath && fsgs->final) {
        ps_lattice_t *dag;
        ps_latlink_t *link;

        if ((dag = fsg_search_lattice(search)) == NULL)
            return 0;
        if ((link = fsg_search_bestpath(search, NULL, TRUE)) == NULL)
            return 0;
        return search->post;
    }
    else {
        
        return 0;
    }
}

static ps_latnode_t *
find_node(ps_lattice_t *dag, fsg_model_t *fsg, int sf, int32 wid, int32 node_id)
{
    ps_latnode_t *node;

    for (node = dag->nodes; node; node = node->next)
        if ((node->sf == sf) && (node->wid == wid) && (node->node_id == node_id))
            break;
    return node;
}

static ps_latnode_t *
new_node(ps_lattice_t *dag, fsg_model_t *fsg, int sf, int ef, int32 wid, int32 node_id, int32 ascr)
{
    ps_latnode_t *node;

    node = find_node(dag, fsg, sf, wid, node_id);

    if (node) {
        
        if (node->lef == -1 || node->lef < ef)
            node->lef = ef;
        if (node->fef == -1 || node->fef > ef)
            node->fef = ef;
        
        if (ascr BETTER_THAN node->info.best_exit)
            node->info.best_exit = ascr;
    }
    else {
        
        node = listelem_malloc(dag->latnode_alloc);
        node->wid = wid;
        node->sf = sf;
        node->fef = node->lef = ef;
        node->reachable = FALSE;
        node->entries = NULL;
        node->exits = NULL;
        node->info.best_exit = ascr;
        node->node_id = node_id;

        node->next = dag->nodes;
        dag->nodes = node;
        ++dag->n_nodes;
    }

    return node;
}

static ps_latnode_t *
find_start_node(fsg_search_t *fsgs, ps_lattice_t *dag)
{
    ps_latnode_t *node;
    glist_t start = NULL;
    int nstart = 0;

    
    for (node = dag->nodes; node; node = node->next) {
        if (node->sf == 0 && node->exits) {
            E_INFO("Start node %s.%d:%d:%d\n",
                   fsg_model_word_str(fsgs->fsg, node->wid),
                   node->sf, node->fef, node->lef);
            start = glist_add_ptr(start, node);
            ++nstart;
        }
    }

    


    if (nstart == 1) {
        node = gnode_ptr(start);
    }
    else {
        gnode_t *st;
        int wid;

        wid = fsg_model_word_add(fsgs->fsg, "<s>");
        if (fsgs->fsg->silwords)
            bitvec_set(fsgs->fsg->silwords, wid);
        node = new_node(dag, fsgs->fsg, 0, 0, wid, -1, 0);
        for (st = start; st; st = gnode_next(st))
            ps_lattice_link(dag, node, gnode_ptr(st), 0, 0);
    }
    glist_free(start);
    return node;
}

static ps_latnode_t *
find_end_node(fsg_search_t *fsgs, ps_lattice_t *dag)
{
    ps_latnode_t *node;
    glist_t end = NULL;
    int nend = 0;

    
    for (node = dag->nodes; node; node = node->next) {
        if (node->lef == dag->n_frames - 1 && node->entries) {
            E_INFO("End node %s.%d:%d:%d (%d)\n",
                   fsg_model_word_str(fsgs->fsg, node->wid),
                   node->sf, node->fef, node->lef, node->info.best_exit);
            end = glist_add_ptr(end, node);
            ++nend;
        }
    }

    if (nend == 1) {
        node = gnode_ptr(end);
    }
    else if (nend == 0) {
        ps_latnode_t *last = NULL;
        int ef = 0;

        

        for (node = dag->nodes; node; node = node->next) {
            if (node->lef > ef && node->entries) {
                last = node;
                ef = node->lef;
            }
        }
        node = last;
        if (node)
            E_INFO("End node %s.%d:%d:%d (%d)\n",
                   fsg_model_word_str(fsgs->fsg, node->wid),
                   node->sf, node->fef, node->lef, node->info.best_exit);
    }    
    else {
        


        gnode_t *st;
        int wid;
        wid = fsg_model_word_add(fsgs->fsg, "</s>");
        if (fsgs->fsg->silwords)
            bitvec_set(fsgs->fsg->silwords, wid);
        node = new_node(dag, fsgs->fsg, fsgs->frame, fsgs->frame, wid, -1, 0);
        

        for (st = end; st; st = gnode_next(st)) {
            ps_latnode_t *src = gnode_ptr(st);
            ps_lattice_link(dag, src, node, src->info.best_exit, fsgs->frame);
        }
    }
    glist_free(end);
    return node;
}

static void
mark_reachable(ps_lattice_t *dag, ps_latnode_t *end)
{
    glist_t q;

    
    end->reachable = TRUE;
    q = glist_add_ptr(NULL, end);
    while (q) {
        ps_latnode_t *node = gnode_ptr(q);
        latlink_list_t *x;

        
        q = gnode_free(q, NULL);
        
        for (x = node->entries; x; x = x->next) {
            ps_latnode_t *next = x->link->from;
            if (!next->reachable) {
                next->reachable = TRUE;
                q = glist_add_ptr(q, next);
            }
        }
    }
}









static ps_lattice_t *
fsg_search_lattice(ps_search_t *search)
{
    fsg_search_t *fsgs;
    fsg_model_t *fsg;
    ps_latnode_t *node;
    ps_lattice_t *dag;
    int32 i, n;

    fsgs = (fsg_search_t *)search;

    

    if (search->dag && search->dag->n_frames == fsgs->frame)
        return search->dag;

    
    ps_lattice_free(search->dag);
    search->dag = NULL;
    dag = ps_lattice_init_search(search, fsgs->frame);
    fsg = fsgs->fsg;

    





    n = fsg_history_n_entries(fsgs->history);
    for (i = 0; i < n; ++i) {
        fsg_hist_entry_t *fh = fsg_history_entry_get(fsgs->history, i);
        int32 ascr;
        int sf;

        
        if (fh->fsglink == NULL || fh->fsglink->wid == -1)
            continue;

        
        if (fh->pred) {
            fsg_hist_entry_t *pfh = fsg_history_entry_get(fsgs->history, fh->pred);
            






            ascr = fh->score - pfh->score;
            sf = pfh->frame + 1;
        }
        else {
            ascr = fh->score;
            sf = 0;
        }

        





        new_node(dag, fsg, sf, fh->frame, fh->fsglink->wid, fsg_link_to_state(fh->fsglink), ascr);
    }

    


    n = fsg_history_n_entries(fsgs->history);
    for (i = 0; i < n; ++i) {
        fsg_hist_entry_t *fh = fsg_history_entry_get(fsgs->history, i);
        fsg_arciter_t *itor;
        ps_latnode_t *src, *dest;
        int32 ascr;
        int sf;

        
        if (fh->fsglink == NULL || fh->fsglink->wid == -1)
            continue;

        
        if (fh->pred) {
            fsg_hist_entry_t *pfh = fsg_history_entry_get(fsgs->history, fh->pred);
            sf = pfh->frame + 1;
            ascr = fh->score - pfh->score;
        }
        else {
            ascr = fh->score;
            sf = 0;
        }
        src = find_node(dag, fsg, sf, fh->fsglink->wid, fsg_link_to_state(fh->fsglink));
        sf = fh->frame + 1;

        for (itor = fsg_model_arcs(fsg, fsg_link_to_state(fh->fsglink));
             itor; itor = fsg_arciter_next(itor)) {
            fsg_link_t *link = fsg_arciter_get(itor);
            
            
            if (link->wid >= 0) {
                



                if ((dest = find_node(dag, fsg, sf, link->wid, fsg_link_to_state(link))) != NULL)
            	    ps_lattice_link(dag, src, dest, ascr, fh->frame);
            }
            else {
                



                fsg_arciter_t *itor2;
                
                
                for (itor2 = fsg_model_arcs(fsg, fsg_link_to_state(link));
                     itor2; itor2 = fsg_arciter_next(itor2)) {
                    fsg_link_t *link = fsg_arciter_get(itor2);

                    if (link->wid == -1)
                        continue;
                    
                    if ((dest = find_node(dag, fsg, sf, link->wid, fsg_link_to_state(link))) != NULL) {
                        ps_lattice_link(dag, src, dest, ascr, fh->frame);
                    }
                }
            }
        }
    }


    
    if ((dag->start = find_start_node(fsgs, dag)) == NULL) {
	E_WARN("Failed to find the start node\n");
        goto error_out;
    }
    if ((dag->end = find_end_node(fsgs, dag)) == NULL) {
	E_WARN("Failed to find the end node\n");
        goto error_out;
    }


    E_INFO("lattice start node %s.%d end node %s.%d\n",
           fsg_model_word_str(fsg, dag->start->wid), dag->start->sf,
           fsg_model_word_str(fsg, dag->end->wid), dag->end->sf);
    

    


    for (node = dag->nodes; node; node = node->next) {
        node->wid = dict_wordid(dag->search->dict,
                                fsg_model_word_str(fsg, node->wid));
        node->basewid = dict_basewid(dag->search->dict, node->wid);
    }

    





    mark_reachable(dag, dag->end);

    ps_lattice_delete_unreachable(dag);
    {
        int32 silpen, fillpen;

        silpen = (int32)(logmath_log(fsg->lmath,
                                     cmd_ln_float32_r(ps_search_config(fsgs), "-silprob"))
                         * fsg->lw)
            >> SENSCR_SHIFT;
        fillpen = (int32)(logmath_log(fsg->lmath,
                                      cmd_ln_float32_r(ps_search_config(fsgs), "-fillprob"))
                          * fsg->lw)
            >> SENSCR_SHIFT;
	
	ps_lattice_penalize_fillers(dag, silpen, fillpen);
    }
    search->dag = dag;

    return dag;


error_out:
    ps_lattice_free(dag);
    return NULL;

}

