









































#include <string.h>
#include <assert.h>


#include <sphinxbase/ckd_alloc.h>
#include <sphinxbase/listelem_alloc.h>
#include <sphinxbase/err.h>


#include "pocketsphinx_internal.h"
#include "ps_lattice_internal.h"
#include "ngram_search.h"
#include "ngram_search_fwdtree.h"
#include "ngram_search_fwdflat.h"

static int ngram_search_start(ps_search_t *search);
static int ngram_search_step(ps_search_t *search, int frame_idx);
static int ngram_search_finish(ps_search_t *search);
static int ngram_search_reinit(ps_search_t *search, dict_t *dict, dict2pid_t *d2p);
static char const *ngram_search_hyp(ps_search_t *search, int32 *out_score, int32 *out_is_final);
static int32 ngram_search_prob(ps_search_t *search);
static ps_seg_t *ngram_search_seg_iter(ps_search_t *search, int32 *out_score);

static ps_searchfuncs_t ngram_funcs = {
       "ngram",
      ngram_search_start,
       ngram_search_step,
     ngram_search_finish,
     ngram_search_reinit,
       ngram_search_free,
      ngram_search_lattice,
          ngram_search_hyp,
         ngram_search_prob,
     ngram_search_seg_iter,
};

static ngram_model_t *default_lm;

static void
ngram_search_update_widmap(ngram_search_t *ngs)
{
    char const **words;
    int32 i, n_words;

    
    n_words = ps_search_n_words(ngs);
    words = (char const**)ckd_calloc(n_words, sizeof(*words));
    
    for (i = 0; i < n_words; ++i)
        words[i] = dict_wordstr(ps_search_dict(ngs), i);
    ngram_model_set_map_words(ngs->lmset, words, n_words);
    ckd_free(words);
}

static void
ngram_search_calc_beams(ngram_search_t *ngs)
{
    cmd_ln_t *config;
    acmod_t *acmod;

    config = ps_search_config(ngs);
    acmod = ps_search_acmod(ngs);

    
    ngs->beam = logmath_log(acmod->lmath, cmd_ln_float64_r(config, "-beam"))>>SENSCR_SHIFT;
    ngs->wbeam = logmath_log(acmod->lmath, cmd_ln_float64_r(config, "-wbeam"))>>SENSCR_SHIFT;
    ngs->pbeam = logmath_log(acmod->lmath, cmd_ln_float64_r(config, "-pbeam"))>>SENSCR_SHIFT;
    ngs->lpbeam = logmath_log(acmod->lmath, cmd_ln_float64_r(config, "-lpbeam"))>>SENSCR_SHIFT;
    ngs->lponlybeam = logmath_log(acmod->lmath, cmd_ln_float64_r(config, "-lponlybeam"))>>SENSCR_SHIFT;
    ngs->fwdflatbeam = logmath_log(acmod->lmath, cmd_ln_float64_r(config, "-fwdflatbeam"))>>SENSCR_SHIFT;
    ngs->fwdflatwbeam = logmath_log(acmod->lmath, cmd_ln_float64_r(config, "-fwdflatwbeam"))>>SENSCR_SHIFT;

    
    ngs->maxwpf = cmd_ln_int32_r(config, "-maxwpf");
    ngs->maxhmmpf = cmd_ln_int32_r(config, "-maxhmmpf");

    
    ngs->wip = logmath_log(acmod->lmath, cmd_ln_float32_r(config, "-wip")) >>SENSCR_SHIFT;
    ngs->nwpen = logmath_log(acmod->lmath, cmd_ln_float32_r(config, "-nwpen")) >>SENSCR_SHIFT;
    ngs->pip = logmath_log(acmod->lmath, cmd_ln_float32_r(config, "-pip")) >>SENSCR_SHIFT;
    ngs->silpen = ngs->pip
        + (logmath_log(acmod->lmath, cmd_ln_float32_r(config, "-silprob"))>>SENSCR_SHIFT);
    ngs->fillpen = ngs->pip
        + (logmath_log(acmod->lmath, cmd_ln_float32_r(config, "-fillprob"))>>SENSCR_SHIFT);

    
    ngs->fwdflat_fwdtree_lw_ratio =
        cmd_ln_float32_r(config, "-fwdflatlw")
        / cmd_ln_float32_r(config, "-lw");
    ngs->bestpath_fwdtree_lw_ratio =
        cmd_ln_float32_r(config, "-bestpathlw")
        / cmd_ln_float32_r(config, "-lw");

    
    ngs->ascale = 1.0 / cmd_ln_float32_r(config, "-ascale");
}

ps_search_t *
ngram_search_init(ngram_model_t *lm,
                  cmd_ln_t *config,
                  acmod_t *acmod,
                  dict_t *dict,
                  dict2pid_t *d2p)
{
    ngram_search_t *ngs;
    static char *lmname = "default";

    

    acmod_set_grow(acmod, cmd_ln_boolean_r(config, "-fwdflat") &&
                          cmd_ln_boolean_r(config, "-fwdtree"));

    ngs = ckd_calloc(1, sizeof(*ngs));
    ps_search_init(&ngs->base, &ngram_funcs, config, acmod, dict, d2p);
    ngs->hmmctx = hmm_context_init(bin_mdef_n_emit_state(acmod->mdef),
                                   acmod->tmat->tp, NULL, acmod->mdef->sseq);
    if (ngs->hmmctx == NULL) {
        ps_search_free(ps_search_base(ngs));
        return NULL;
    }
    ngs->chan_alloc = listelem_alloc_init(sizeof(chan_t));
    ngs->root_chan_alloc = listelem_alloc_init(sizeof(root_chan_t));
    ngs->latnode_alloc = listelem_alloc_init(sizeof(ps_latnode_t));

    
    ngram_search_calc_beams(ngs);

    
    ngs->word_chan = ckd_calloc(dict_size(dict),
                                sizeof(*ngs->word_chan));
    ngs->word_lat_idx = ckd_calloc(dict_size(dict),
                                   sizeof(*ngs->word_lat_idx));
    ngs->word_active = bitvec_alloc(dict_size(dict));
    ngs->last_ltrans = ckd_calloc(dict_size(dict),
                                  sizeof(*ngs->last_ltrans));

    

    ngs->bp_table_size = cmd_ln_int32_r(config, "-latsize");
    ngs->bp_table = ckd_calloc(ngs->bp_table_size,
                               sizeof(*ngs->bp_table));
    
    ngs->bscore_stack_size = ngs->bp_table_size * 20;
    ngs->bscore_stack = ckd_calloc(ngs->bscore_stack_size,
                                   sizeof(*ngs->bscore_stack));
    ngs->n_frame_alloc = 256;
    ngs->bp_table_idx = ckd_calloc(ngs->n_frame_alloc + 1,
                                   sizeof(*ngs->bp_table_idx));
    ++ngs->bp_table_idx; 

    
    ngs->active_word_list = ckd_calloc_2d(2, dict_size(dict),
                                          sizeof(**ngs->active_word_list));

    ngs->lmset = ngram_model_set_init(config, &lm, &lmname, NULL, 1);
    if (!ngs->lmset)
        goto error_out;

    if (ngram_wid(ngs->lmset, S3_FINISH_WORD) ==
        ngram_unknown_wid(ngs->lmset))
    {
        E_ERROR("Language model/set does not contain </s>, "
                "recognition will fail\n");
        goto error_out;
    }

    
    ngram_search_update_widmap(ngs);

    
    if (cmd_ln_boolean_r(config, "-fwdtree")) {
        ngram_fwdtree_init(ngs);
        ngs->fwdtree = TRUE;
        ngs->fwdtree_perf.name = "fwdtree";
        ptmr_init(&ngs->fwdtree_perf);
    }
    if (cmd_ln_boolean_r(config, "-fwdflat")) {
        ngram_fwdflat_init(ngs);
        ngs->fwdflat = TRUE;
        ngs->fwdflat_perf.name = "fwdflat";
        ptmr_init(&ngs->fwdflat_perf);
    }
    if (cmd_ln_boolean_r(config, "-bestpath")) {
        ngs->bestpath = TRUE;
        ngs->bestpath_perf.name = "bestpath";
        ptmr_init(&ngs->bestpath_perf);
    }

    return (ps_search_t *)ngs;

error_out:
    ngram_search_free((ps_search_t *)ngs);
    return NULL;
}

static int
ngram_search_reinit(ps_search_t *search, dict_t *dict, dict2pid_t *d2p)
{
    ngram_search_t *ngs = (ngram_search_t *)search;
    int old_n_words;
    int rv = 0;

    
    old_n_words = search->n_words;
    if (old_n_words != dict_size(dict)) {
        search->n_words = dict_size(dict);
        
        ckd_free(ngs->word_lat_idx);
        ckd_free(ngs->word_active);
        ckd_free(ngs->last_ltrans);
        ckd_free_2d(ngs->active_word_list);
        ngs->word_lat_idx = ckd_calloc(search->n_words, sizeof(*ngs->word_lat_idx));
        ngs->word_active = bitvec_alloc(search->n_words);
        ngs->last_ltrans = ckd_calloc(search->n_words, sizeof(*ngs->last_ltrans));
        ngs->active_word_list
            = ckd_calloc_2d(2, search->n_words,
                            sizeof(**ngs->active_word_list));
    }

    
    ps_search_base_reinit(search, dict, d2p);

    if (ngs->lmset == NULL)
        return 0;

    
    ngram_search_calc_beams(ngs);

    
    ngram_search_update_widmap(ngs);

    
    if (ngs->fwdtree) {
        if ((rv = ngram_fwdtree_reinit(ngs)) < 0)
            return rv;
    }
    if (ngs->fwdflat) {
        if ((rv = ngram_fwdflat_reinit(ngs)) < 0)
            return rv;
    }

    return rv;
}

void
ngram_search_free(ps_search_t *search)
{
    ngram_search_t *ngs = (ngram_search_t *)search;

    ps_search_deinit(search);
    if (ngs->fwdtree)
        ngram_fwdtree_deinit(ngs);
    if (ngs->fwdflat)
        ngram_fwdflat_deinit(ngs);
    if (ngs->bestpath) {
        double n_speech = (double)ngs->n_tot_frame
            / cmd_ln_int32_r(ps_search_config(ngs), "-frate");

        E_INFO("TOTAL bestpath %.2f CPU %.3f xRT\n",
               ngs->bestpath_perf.t_tot_cpu,
               ngs->bestpath_perf.t_tot_cpu / n_speech);
        E_INFO("TOTAL bestpath %.2f wall %.3f xRT\n",
               ngs->bestpath_perf.t_tot_elapsed,
               ngs->bestpath_perf.t_tot_elapsed / n_speech);
    }

    hmm_context_free(ngs->hmmctx);
    listelem_alloc_free(ngs->chan_alloc);
    listelem_alloc_free(ngs->root_chan_alloc);
    listelem_alloc_free(ngs->latnode_alloc);
    ngram_model_free(ngs->lmset);

    ckd_free(ngs->word_chan);
    ckd_free(ngs->word_lat_idx);
    bitvec_free(ngs->word_active);
    ckd_free(ngs->bp_table);
    ckd_free(ngs->bscore_stack);
    if (ngs->bp_table_idx != NULL)
        ckd_free(ngs->bp_table_idx - 1);
    ckd_free_2d(ngs->active_word_list);
    ckd_free(ngs->last_ltrans);
    ckd_free(ngs);
}

int
ngram_search_mark_bptable(ngram_search_t *ngs, int frame_idx)
{
    if (frame_idx >= ngs->n_frame_alloc) {
        ngs->n_frame_alloc *= 2;
        ngs->bp_table_idx = ckd_realloc(ngs->bp_table_idx - 1,
                                        (ngs->n_frame_alloc + 1)
                                        * sizeof(*ngs->bp_table_idx));
        if (ngs->frm_wordlist) {
            ngs->frm_wordlist = ckd_realloc(ngs->frm_wordlist,
                                            ngs->n_frame_alloc
                                            * sizeof(*ngs->frm_wordlist));
        }
        ++ngs->bp_table_idx; 
    }
    ngs->bp_table_idx[frame_idx] = ngs->bpidx;
    return ngs->bpidx;
}

static void
set_real_wid(ngram_search_t *ngs, int32 bp)
{
    bptbl_t *ent, *prev;

    assert(bp != NO_BP);
    ent = ngs->bp_table + bp;
    if (ent->bp == NO_BP)
        prev = NULL;
    else
        prev = ngs->bp_table + ent->bp;

    
    if (dict_filler_word(ps_search_dict(ngs), ent->wid)) {
        if (prev != NULL) {
            ent->real_wid = prev->real_wid;
            ent->prev_real_wid = prev->prev_real_wid;
        }
        else {
            ent->real_wid = dict_basewid(ps_search_dict(ngs),
                                         ent->wid);
            ent->prev_real_wid = BAD_S3WID;
        }
    }
    else {
        ent->real_wid = dict_basewid(ps_search_dict(ngs), ent->wid);
        if (prev != NULL)
            ent->prev_real_wid = prev->real_wid;
        else
            ent->prev_real_wid = BAD_S3WID;
    }
}

#define NGRAM_HISTORY_LONG_WORD 2000 /* 20s */

void
ngram_search_save_bp(ngram_search_t *ngs, int frame_idx,
                     int32 w, int32 score, int32 path, int32 rc)
{
    int32 bp;

    


    bp = ngs->word_lat_idx[w];
    if (bp != NO_BP) {

        if (frame_idx - ngs->bp_table[path].frame > NGRAM_HISTORY_LONG_WORD) {
    	    E_WARN("Word '%s' survived for %d frames, potential overpruning\n", dict_wordstr(ps_search_dict(ngs), w),
	    	    frame_idx - ngs->bp_table[path].frame);
	}

        


        if (ngs->bp_table[bp].score WORSE_THAN score) {
            assert(path != bp); 
            if (ngs->bp_table[bp].bp != path) {
                int32 bplh[2], newlh[2];
                


                E_DEBUG(2,("Updating path history %d => %d frame %d\n",
                           ngs->bp_table[bp].bp, path, frame_idx));
                bplh[0] = ngs->bp_table[bp].bp == -1
                    ? -1 : ngs->bp_table[ngs->bp_table[bp].bp].prev_real_wid;
                bplh[1] = ngs->bp_table[bp].bp == -1
                    ? -1 : ngs->bp_table[ngs->bp_table[bp].bp].real_wid;
                newlh[0] = path == -1
                    ? -1 : ngs->bp_table[path].prev_real_wid;
                newlh[1] = path == -1
                    ? -1 : ngs->bp_table[path].real_wid;
                

                if (bplh[0] != newlh[0] || bplh[1] != newlh[1]) {
                    


                    E_DEBUG(1, ("Updating language model state %s,%s => %s,%s frame %d\n",
                                dict_wordstr(ps_search_dict(ngs), bplh[0]),
                                dict_wordstr(ps_search_dict(ngs), bplh[1]),
                                dict_wordstr(ps_search_dict(ngs), newlh[0]),
                                dict_wordstr(ps_search_dict(ngs), newlh[1]),
                                frame_idx));
                    set_real_wid(ngs, bp);
                }
                ngs->bp_table[bp].bp = path;
            }
            ngs->bp_table[bp].score = score;
        }
        


        if (ngs->bp_table[bp].s_idx != -1)
            ngs->bscore_stack[ngs->bp_table[bp].s_idx + rc] = score;
    }
    else {
        int32 i, rcsize;
        bptbl_t *be;

        
        if (ngs->bpidx == NO_BP) {
            E_ERROR("No entries in backpointer table!");
            return;
        }

        
        if (ngs->bpidx >= ngs->bp_table_size) {
            ngs->bp_table_size *= 2;
            ngs->bp_table = ckd_realloc(ngs->bp_table,
                                        ngs->bp_table_size
                                        * sizeof(*ngs->bp_table));
            E_INFO("Resized backpointer table to %d entries\n", ngs->bp_table_size);
        }
        if (ngs->bss_head >= ngs->bscore_stack_size
            - bin_mdef_n_ciphone(ps_search_acmod(ngs)->mdef)) {
            ngs->bscore_stack_size *= 2;
            ngs->bscore_stack = ckd_realloc(ngs->bscore_stack,
                                            ngs->bscore_stack_size
                                            * sizeof(*ngs->bscore_stack));
            E_INFO("Resized score stack to %d entries\n", ngs->bscore_stack_size);
        }

        ngs->word_lat_idx[w] = ngs->bpidx;
        be = &(ngs->bp_table[ngs->bpidx]);
        be->wid = w;
        be->frame = frame_idx;
        be->bp = path;
        be->score = score;
        be->s_idx = ngs->bss_head;
        be->valid = TRUE;
        assert(path != ngs->bpidx);

        
        
        be->last_phone = dict_last_phone(ps_search_dict(ngs),w);
        if (dict_is_single_phone(ps_search_dict(ngs), w)) {
            be->last2_phone = -1;
            be->s_idx = -1;
            rcsize = 0;
        }
        else {
            be->last2_phone = dict_second_last_phone(ps_search_dict(ngs),w);
            rcsize = dict2pid_rssid(ps_search_dict2pid(ngs),
                                    be->last_phone, be->last2_phone)->n_ssid;
        }
        
        for (i = 0; i < rcsize; ++i)
            ngs->bscore_stack[ngs->bss_head + i] = WORST_SCORE;
        if (rcsize)
            ngs->bscore_stack[ngs->bss_head + rc] = score;
        set_real_wid(ngs, ngs->bpidx);

        ngs->bpidx++;
        ngs->bss_head += rcsize;
    }
}

int
ngram_search_find_exit(ngram_search_t *ngs, int frame_idx, int32 *out_best_score, int32 *out_is_final)
{
    
    int end_bpidx;
    int best_exit, bp;
    int32 best_score;

    
    if (ngs->n_frame == 0)
        return NO_BP;

    if (frame_idx == -1 || frame_idx >= ngs->n_frame)
        frame_idx = ngs->n_frame - 1;
    end_bpidx = ngs->bp_table_idx[frame_idx];

    best_score = WORST_SCORE;
    best_exit = NO_BP;

    
    while (frame_idx >= 0 && ngs->bp_table_idx[frame_idx] == end_bpidx)
        --frame_idx;
    
    if (frame_idx < 0)
        return NO_BP;

    
    assert(end_bpidx < ngs->bp_table_size);
    for (bp = ngs->bp_table_idx[frame_idx]; bp < end_bpidx; ++bp) {
        if (ngs->bp_table[bp].wid == ps_search_finish_wid(ngs)
            || ngs->bp_table[bp].score BETTER_THAN best_score) {
            best_score = ngs->bp_table[bp].score;
            best_exit = bp;
        }
        if (ngs->bp_table[bp].wid == ps_search_finish_wid(ngs))
            break;
    }

    if (out_best_score) {
	*out_best_score = best_score;
    }
    if (out_is_final) {
	*out_is_final = (ngs->bp_table[bp].wid == ps_search_finish_wid(ngs));
    }
    return best_exit;
}

char const *
ngram_search_bp_hyp(ngram_search_t *ngs, int bpidx)
{
    ps_search_t *base = ps_search_base(ngs);
    char *c;
    size_t len;
    int bp;

    if (bpidx == NO_BP)
        return NULL;

    bp = bpidx;
    len = 0;
    while (bp != NO_BP) {
        bptbl_t *be = &ngs->bp_table[bp];
        bp = be->bp;
        if (dict_real_word(ps_search_dict(ngs), be->wid))
            len += strlen(dict_basestr(ps_search_dict(ngs), be->wid)) + 1;
    }

    ckd_free(base->hyp_str);
    if (len == 0) {
	base->hyp_str = NULL;
	return base->hyp_str;
    }
    base->hyp_str = ckd_calloc(1, len);

    bp = bpidx;
    c = base->hyp_str + len - 1;
    while (bp != NO_BP) {
        bptbl_t *be = &ngs->bp_table[bp];
        size_t len;

        bp = be->bp;
        if (dict_real_word(ps_search_dict(ngs), be->wid)) {
            len = strlen(dict_basestr(ps_search_dict(ngs), be->wid));
            c -= len;
            memcpy(c, dict_basestr(ps_search_dict(ngs), be->wid), len);
            if (c > base->hyp_str) {
                --c;
                *c = ' ';
            }
        }
    }

    return base->hyp_str;
}

void
ngram_search_alloc_all_rc(ngram_search_t *ngs, int32 w)
{
    chan_t *hmm, *thmm;
    xwdssid_t *rssid;
    int32 i, tmatid, ciphone;

    
    
    assert(!dict_is_single_phone(ps_search_dict(ngs), w));
    ciphone = dict_last_phone(ps_search_dict(ngs),w);
    rssid = dict2pid_rssid(ps_search_dict2pid(ngs),
                           ciphone,
                           dict_second_last_phone(ps_search_dict(ngs),w));
    tmatid = bin_mdef_pid2tmatid(ps_search_acmod(ngs)->mdef, ciphone);
    hmm = ngs->word_chan[w];
    if ((hmm == NULL) || (hmm_nonmpx_ssid(&hmm->hmm) != rssid->ssid[0])) {
        hmm = listelem_malloc(ngs->chan_alloc);
        hmm->next = ngs->word_chan[w];
        ngs->word_chan[w] = hmm;

        hmm->info.rc_id = 0;
        hmm->ciphone = ciphone;
        hmm_init(ngs->hmmctx, &hmm->hmm, FALSE, rssid->ssid[0], tmatid);
        E_DEBUG(3,("allocated rc_id 0 ssid %d ciphone %d lc %d word %s\n",
                   rssid->ssid[0], hmm->ciphone,
                   dict_second_last_phone(ps_search_dict(ngs),w),
                   dict_wordstr(ps_search_dict(ngs),w)));
    }
    for (i = 1; i < rssid->n_ssid; ++i) {
        if ((hmm->next == NULL) || (hmm_nonmpx_ssid(&hmm->next->hmm) != rssid->ssid[i])) {
            thmm = listelem_malloc(ngs->chan_alloc);
            thmm->next = hmm->next;
            hmm->next = thmm;
            hmm = thmm;

            hmm->info.rc_id = i;
            hmm->ciphone = ciphone;
            hmm_init(ngs->hmmctx, &hmm->hmm, FALSE, rssid->ssid[i], tmatid);
            E_DEBUG(3,("allocated rc_id %d ssid %d ciphone %d lc %d word %s\n",
                       i, rssid->ssid[i], hmm->ciphone,
                       dict_second_last_phone(ps_search_dict(ngs),w),
                       dict_wordstr(ps_search_dict(ngs),w)));
        }
        else
            hmm = hmm->next;
    }
}

void
ngram_search_free_all_rc(ngram_search_t *ngs, int32 w)
{
    chan_t *hmm, *thmm;

    for (hmm = ngs->word_chan[w]; hmm; hmm = thmm) {
        thmm = hmm->next;
        hmm_deinit(&hmm->hmm);
        listelem_free(ngs->chan_alloc, hmm);
    }
    ngs->word_chan[w] = NULL;
}

int32
ngram_search_exit_score(ngram_search_t *ngs, bptbl_t *pbe, int rcphone)
{
    
    

    if (pbe->last2_phone == -1) {
        
        return pbe->score;
    }
    else {
        xwdssid_t *rssid;
        

        rssid = dict2pid_rssid(ps_search_dict2pid(ngs),
                               pbe->last_phone, pbe->last2_phone);
        

        return ngs->bscore_stack[pbe->s_idx + rssid->cimap[rcphone]];
    }
}




void
ngram_compute_seg_score(ngram_search_t *ngs, bptbl_t *be, float32 lwf,
                        int32 *out_ascr, int32 *out_lscr)
{
    bptbl_t *pbe;
    int32 start_score;

    
    if (be->bp == NO_BP) {
        *out_ascr = be->score;
        *out_lscr = 0;
        return;
    }

    
    pbe = ngs->bp_table + be->bp;
    start_score = ngram_search_exit_score(ngs, pbe,
                                 dict_first_phone(ps_search_dict(ngs),be->wid));
    assert(start_score BETTER_THAN WORST_SCORE);

    



    if (be->wid == ps_search_silence_wid(ngs)) {
        *out_lscr = ngs->silpen;
    }
    else if (dict_filler_word(ps_search_dict(ngs), be->wid)) {
        *out_lscr = ngs->fillpen;
    }
    else {
        int32 n_used;
        *out_lscr = ngram_tg_score(ngs->lmset,
                                   be->real_wid,
                                   pbe->real_wid,
                                   pbe->prev_real_wid,
                                   &n_used)>>SENSCR_SHIFT;
        *out_lscr = *out_lscr * lwf;
    }
    *out_ascr = be->score - start_score - *out_lscr;
}

static int
ngram_search_start(ps_search_t *search)
{
    ngram_search_t *ngs = (ngram_search_t *)search;

    ngs->done = FALSE;
    ngram_model_flush(ngs->lmset);
    if (ngs->fwdtree)
        ngram_fwdtree_start(ngs);
    else if (ngs->fwdflat)
        ngram_fwdflat_start(ngs);
    else
        return -1;
    return 0;
}

static int
ngram_search_step(ps_search_t *search, int frame_idx)
{
    ngram_search_t *ngs = (ngram_search_t *)search;

    if (ngs->fwdtree)
        return ngram_fwdtree_search(ngs, frame_idx);
    else if (ngs->fwdflat)
        return ngram_fwdflat_search(ngs, frame_idx);
    else
        return -1;
}

void
dump_bptable(ngram_search_t *ngs)
{
    int i;
    E_INFO("Backpointer table (%d entries):\n", ngs->bpidx);
    for (i = 0; i < ngs->bpidx; ++i) {
        bptbl_t *bpe = ngs->bp_table + i;
        int j, rcsize;

        E_INFO_NOFN("%-5d %-10s start %-3d end %-3d score %-8d bp %-3d real_wid %-5d prev_real_wid %-5d",
                    i, dict_wordstr(ps_search_dict(ngs), bpe->wid),
                    (bpe->bp == -1
                     ? 0 : ngs->bp_table[bpe->bp].frame + 1),
                    bpe->frame, bpe->score, bpe->bp,
                    bpe->real_wid, bpe->prev_real_wid);

        if (bpe->last2_phone == -1)
            rcsize = 0;
        else
            rcsize = dict2pid_rssid(ps_search_dict2pid(ngs),
                                    bpe->last_phone, bpe->last2_phone)->n_ssid;
        if (rcsize) {
            E_INFOCONT("\tbss");
            for (j = 0; j < rcsize; ++j)
                if (ngs->bscore_stack[bpe->s_idx + j] != WORST_SCORE)
                    E_INFOCONT(" %d", bpe->score - ngs->bscore_stack[bpe->s_idx + j]);
        }
        E_INFOCONT("\n");
    }
}

static int
ngram_search_finish(ps_search_t *search)
{
    ngram_search_t *ngs = (ngram_search_t *)search;

    ngs->n_tot_frame += ngs->n_frame;
    if (ngs->fwdtree) {
        ngram_fwdtree_finish(ngs);
        

        
        if (ngs->fwdflat) {
            int i;
            
            if (acmod_rewind(ps_search_acmod(ngs)) < 0)
                return -1;
            
            ngram_fwdflat_start(ngs);
            i = 0;
            while (ps_search_acmod(ngs)->n_feat_frame > 0) {
                int nfr;
                if ((nfr = ngram_fwdflat_search(ngs, i)) < 0)
                    return nfr;
                acmod_advance(ps_search_acmod(ngs));
                ++i;
            }
            ngram_fwdflat_finish(ngs);
            
            
        }
    }
    else if (ngs->fwdflat) {
        ngram_fwdflat_finish(ngs);
    }

    
    ngs->done = TRUE;
    return 0;
}

static ps_latlink_t *
ngram_search_bestpath(ps_search_t *search, int32 *out_score, int backward)
{
    ngram_search_t *ngs = (ngram_search_t *)search;

    if (search->last_link == NULL) {
        search->last_link = ps_lattice_bestpath(search->dag, ngs->lmset,
                                                ngs->bestpath_fwdtree_lw_ratio,
                                                ngs->ascale);
        if (search->last_link == NULL)
            return NULL;
        

        if (search->post == 0)
            search->post = ps_lattice_posterior(search->dag, ngs->lmset,
                                                ngs->ascale);
    }
    if (out_score)
        *out_score = search->last_link->path_scr + search->dag->final_node_ascr;
    return search->last_link;
}

static char const *
ngram_search_hyp(ps_search_t *search, int32 *out_score, int32 *out_is_final)
{
    ngram_search_t *ngs = (ngram_search_t *)search;

    
    if (ngs->bestpath && ngs->done) {
        ps_lattice_t *dag;
        ps_latlink_t *link;
        char const *hyp;
        double n_speech;

        ptmr_reset(&ngs->bestpath_perf);
        ptmr_start(&ngs->bestpath_perf);
        if ((dag = ngram_search_lattice(search)) == NULL)
            return NULL;
        if ((link = ngram_search_bestpath(search, out_score, FALSE)) == NULL)
            return NULL;
        hyp = ps_lattice_hyp(dag, link);
        ptmr_stop(&ngs->bestpath_perf);
        n_speech = (double)dag->n_frames
            / cmd_ln_int32_r(ps_search_config(ngs), "-frate");
        E_INFO("bestpath %.2f CPU %.3f xRT\n",
               ngs->bestpath_perf.t_cpu,
               ngs->bestpath_perf.t_cpu / n_speech);
        E_INFO("bestpath %.2f wall %.3f xRT\n",
               ngs->bestpath_perf.t_elapsed,
               ngs->bestpath_perf.t_elapsed / n_speech);
        return hyp;
    }
    else {
        int32 bpidx;

        
        bpidx = ngram_search_find_exit(ngs, -1, out_score, out_is_final);
        if (bpidx != NO_BP)
            return ngram_search_bp_hyp(ngs, bpidx);
    }

    return NULL;
}

static void
ngram_search_bp2itor(ps_seg_t *seg, int bp)
{
    ngram_search_t *ngs = (ngram_search_t *)seg->search;
    bptbl_t *be, *pbe;

    be = &ngs->bp_table[bp];
    pbe = be->bp == -1 ? NULL : &ngs->bp_table[be->bp];
    seg->word = dict_wordstr(ps_search_dict(ngs), be->wid);
    seg->ef = be->frame;
    seg->sf = pbe ? pbe->frame + 1 : 0;
    seg->prob = 0; 
    
    if (pbe == NULL) {
        seg->ascr = be->score;
        seg->lscr = 0;
        seg->lback = 0;
    }
    else {
        int32 start_score;

        
        start_score = ngram_search_exit_score(ngs, pbe,
                                     dict_first_phone(ps_search_dict(ngs), be->wid));
        assert(start_score BETTER_THAN WORST_SCORE);
        if (be->wid == ps_search_silence_wid(ngs)) {
            seg->lscr = ngs->silpen;
        }
        else if (dict_filler_word(ps_search_dict(ngs), be->wid)) {
            seg->lscr = ngs->fillpen;
        }
        else {
            seg->lscr = ngram_tg_score(ngs->lmset,
                                       be->real_wid,
                                       pbe->real_wid,
                                       pbe->prev_real_wid,
                                       &seg->lback)>>SENSCR_SHIFT;
            seg->lscr = (int32)(seg->lscr * seg->lwf);
        }
        seg->ascr = be->score - start_score - seg->lscr;
    }
}

static void
ngram_bp_seg_free(ps_seg_t *seg)
{
    bptbl_seg_t *itor = (bptbl_seg_t *)seg;

    ckd_free(itor->bpidx);
    ckd_free(itor);
}

static ps_seg_t *
ngram_bp_seg_next(ps_seg_t *seg)
{
    bptbl_seg_t *itor = (bptbl_seg_t *)seg;

    if (++itor->cur == itor->n_bpidx) {
        ngram_bp_seg_free(seg);
        return NULL;
    }

    ngram_search_bp2itor(seg, itor->bpidx[itor->cur]);
    return seg;
}

static ps_segfuncs_t ngram_bp_segfuncs = {
     ngram_bp_seg_next,
     ngram_bp_seg_free
};

static ps_seg_t *
ngram_search_bp_iter(ngram_search_t *ngs, int bpidx, float32 lwf)
{
    bptbl_seg_t *itor;
    int bp, cur;

    



    itor = ckd_calloc(1, sizeof(*itor));
    itor->base.vt = &ngram_bp_segfuncs;
    itor->base.search = ps_search_base(ngs);
    itor->base.lwf = lwf;
    itor->n_bpidx = 0;
    bp = bpidx;
    while (bp != NO_BP) {
        bptbl_t *be = &ngs->bp_table[bp];
        bp = be->bp;
        ++itor->n_bpidx;
    }
    if (itor->n_bpidx == 0) {
        ckd_free(itor);
        return NULL;
    }
    itor->bpidx = ckd_calloc(itor->n_bpidx, sizeof(*itor->bpidx));
    cur = itor->n_bpidx - 1;
    bp = bpidx;
    while (bp != NO_BP) {
        bptbl_t *be = &ngs->bp_table[bp];
        itor->bpidx[cur] = bp;
        bp = be->bp;
        --cur;
    }

    
    ngram_search_bp2itor((ps_seg_t *)itor, itor->bpidx[0]);

    return (ps_seg_t *)itor;
}

static ps_seg_t *
ngram_search_seg_iter(ps_search_t *search, int32 *out_score)
{
    ngram_search_t *ngs = (ngram_search_t *)search;

    
    if (ngs->bestpath && ngs->done) {
        ps_lattice_t *dag;
        ps_latlink_t *link;
        double n_speech;
        ps_seg_t *itor;

        ptmr_reset(&ngs->bestpath_perf);
        ptmr_start(&ngs->bestpath_perf);
        if ((dag = ngram_search_lattice(search)) == NULL)
            return NULL;
        if ((link = ngram_search_bestpath(search, out_score, TRUE)) == NULL)
            return NULL;
        itor = ps_lattice_seg_iter(dag, link,
                                   ngs->bestpath_fwdtree_lw_ratio);
        ptmr_stop(&ngs->bestpath_perf);
        n_speech = (double)dag->n_frames
            / cmd_ln_int32_r(ps_search_config(ngs), "-frate");
        E_INFO("bestpath %.2f CPU %.3f xRT\n",
               ngs->bestpath_perf.t_cpu,
               ngs->bestpath_perf.t_cpu / n_speech);
        E_INFO("bestpath %.2f wall %.3f xRT\n",
               ngs->bestpath_perf.t_elapsed,
               ngs->bestpath_perf.t_elapsed / n_speech);
        return itor;
    }
    else {
        int32 bpidx;

        
        bpidx = ngram_search_find_exit(ngs, -1, out_score, NULL);
        return ngram_search_bp_iter(ngs, bpidx,
                                    
                                    (ngs->done && ngs->fwdflat)
                                    ? ngs->fwdflat_fwdtree_lw_ratio : 1.0);
    }

    return NULL;
}

static int32
ngram_search_prob(ps_search_t *search)
{
    ngram_search_t *ngs = (ngram_search_t *)search;

    
    if (ngs->bestpath && ngs->done) {
        ps_lattice_t *dag;
        ps_latlink_t *link;

        if ((dag = ngram_search_lattice(search)) == NULL)
            return 0;
        if ((link = ngram_search_bestpath(search, NULL, TRUE)) == NULL)
            return 0;
        return search->post;
    }
    else {
        
        return 0;
    }
}

static void
create_dag_nodes(ngram_search_t *ngs, ps_lattice_t *dag)
{
    bptbl_t *bp_ptr;
    int32 i;

    for (i = 0, bp_ptr = ngs->bp_table; i < ngs->bpidx; ++i, ++bp_ptr) {
        int32 sf, ef, wid;
        ps_latnode_t *node;

        
        if (!bp_ptr->valid)
            continue;

        sf = (bp_ptr->bp < 0) ? 0 : ngs->bp_table[bp_ptr->bp].frame + 1;
        ef = bp_ptr->frame;
        wid = bp_ptr->wid;

        assert(ef < dag->n_frames);
        
        if ((wid == ps_search_finish_wid(ngs)) && (ef < dag->n_frames - 1))
            continue;

        
        if ((!dict_filler_word(ps_search_dict(ngs), wid))
            && (!ngram_model_set_known_wid(ngs->lmset,
                                           dict_basewid(ps_search_dict(ngs), wid))))
            continue;

        
        for (node = dag->nodes; node; node = node->next) {
            if ((node->wid == wid) && (node->sf == sf))
                break;
        }

        
        if (node)
            node->lef = i;
        else {
            
            node = listelem_malloc(dag->latnode_alloc);
            node->wid = wid;
            node->sf = sf; 
            node->fef = node->lef = i; 
            node->reachable = FALSE;
            node->entries = NULL;
            node->exits = NULL;

            


            node->next = dag->nodes;
            dag->nodes = node;
            ++dag->n_nodes;
        }
    }
}

static ps_latnode_t *
find_start_node(ngram_search_t *ngs, ps_lattice_t *dag)
{
    ps_latnode_t *node;

    
    for (node = dag->nodes; node; node = node->next) {
        if ((node->wid == ps_search_start_wid(ngs)) && (node->sf == 0))
            break;
    }
    if (!node) {
        
        E_ERROR("Couldn't find <s> in first frame\n");
        return NULL;
    }
    return node;
}

static ps_latnode_t *
find_end_node(ngram_search_t *ngs, ps_lattice_t *dag, float32 lwf)
{
    ps_latnode_t *node;
    int32 ef, bestbp, bp, bestscore;

    
    for (node = dag->nodes; node; node = node->next) {
        int32 lef = ngs->bp_table[node->lef].frame;
        if ((node->wid == ps_search_finish_wid(ngs))
            && (lef == dag->n_frames - 1))
            break;
    }
    if (node != NULL)
        return node;

    

    
    for (ef = dag->n_frames - 1;
         ef >= 0 && ngs->bp_table_idx[ef] == ngs->bpidx;
         --ef);
    if (ef < 0) {
        E_ERROR("Empty backpointer table: can not build DAG.\n");
        return NULL;
    }

    
    bestscore = WORST_SCORE;
    bestbp = NO_BP;
    for (bp = ngs->bp_table_idx[ef]; bp < ngs->bp_table_idx[ef + 1]; ++bp) {
        int32 n_used, l_scr, wid, prev_wid;
        wid = ngs->bp_table[bp].real_wid;
        prev_wid = ngs->bp_table[bp].prev_real_wid;
        
        if (wid == ps_search_finish_wid(ngs)) {
            bestbp = bp;
            break;
        }
        l_scr = ngram_tg_score(ngs->lmset, ps_search_finish_wid(ngs),
                               wid, prev_wid, &n_used) >>SENSCR_SHIFT;
        l_scr = l_scr * lwf;
        if (ngs->bp_table[bp].score + l_scr BETTER_THAN bestscore) {
            bestscore = ngs->bp_table[bp].score + l_scr;
            bestbp = bp;
        }
    }
    if (bestbp == NO_BP) {
        E_ERROR("No word exits found in last frame (%d), assuming no recognition\n", ef);
        return NULL;
    }
    E_INFO("</s> not found in last frame, using %s.%d instead\n",
           dict_basestr(ps_search_dict(ngs), ngs->bp_table[bestbp].wid), ef);

    
    for (node = dag->nodes; node; node = node->next) {
        if (node->lef == bestbp)
            return node;
    }

    
    E_ERROR("Failed to find DAG node corresponding to %s\n",
           dict_basestr(ps_search_dict(ngs), ngs->bp_table[bestbp].wid));
    return NULL;
}




ps_lattice_t *
ngram_search_lattice(ps_search_t *search)
{
    int32 i, score, ascr, lscr;
    ps_latnode_t *node, *from, *to;
    ngram_search_t *ngs;
    ps_lattice_t *dag;
    int min_endfr, nlink;
    float lwf;

    ngs = (ngram_search_t *)search;
    min_endfr = cmd_ln_int32_r(ps_search_config(search), "-min_endfr");

    

    if (ngs->best_score == WORST_SCORE || ngs->best_score WORSE_THAN WORST_SCORE)
        return NULL;

    

    if (search->dag && search->dag->n_frames == ngs->n_frame)
        return search->dag;

    
    ps_lattice_free(search->dag);
    search->dag = NULL;
    dag = ps_lattice_init_search(search, ngs->n_frame);
    
    lwf = ngs->fwdflat ? ngs->fwdflat_fwdtree_lw_ratio : 1.0;
    create_dag_nodes(ngs, dag);
    if ((dag->start = find_start_node(ngs, dag)) == NULL)
        goto error_out;
    if ((dag->end = find_end_node(ngs, dag, ngs->bestpath_fwdtree_lw_ratio)) == NULL)
        goto error_out;
    E_INFO("lattice start node %s.%d end node %s.%d\n",
           dict_wordstr(search->dict, dag->start->wid), dag->start->sf,
           dict_wordstr(search->dict, dag->end->wid), dag->end->sf);

    ngram_compute_seg_score(ngs, ngs->bp_table + dag->end->lef, lwf,
                            &dag->final_node_ascr, &lscr);

    













    i = 0;
    while (dag->nodes && dag->nodes != dag->end) {
        ps_latnode_t *next = dag->nodes->next;
        listelem_free(dag->latnode_alloc, dag->nodes);
        dag->nodes = next;
        ++i;
    }
    E_INFO("Eliminated %d nodes before end node\n", i);
    dag->end->reachable = TRUE;
    nlink = 0;
    for (to = dag->end; to; to = to->next) {
        int fef, lef;

        
        if (!to->reachable)
            continue;

        

        fef = ngs->bp_table[to->fef].frame;
        lef = ngs->bp_table[to->lef].frame;
        if (to != dag->end && lef - fef < min_endfr) {
            to->reachable = FALSE;
            continue;
        }

        
        for (from = to->next; from; from = from->next) {
            bptbl_t *from_bpe;

            fef = ngs->bp_table[from->fef].frame;
            lef = ngs->bp_table[from->lef].frame;

            if ((to->sf <= fef) || (to->sf > lef + 1))
                continue;
            if (lef - fef < min_endfr) {
                assert(!from->reachable);
                continue;
            }

            
            i = from->fef;
            from_bpe = ngs->bp_table + i;
            for (; i <= from->lef; i++, from_bpe++) {
                if (from_bpe->wid != from->wid)
                    continue;
                if (from_bpe->frame >= to->sf - 1)
                    break;
            }

            if ((i > from->lef) || (from_bpe->frame != to->sf - 1))
                continue;

            
            
            ngram_compute_seg_score(ngs, from_bpe, lwf,
                                    &ascr, &lscr);
            


            score = ngram_search_exit_score(ngs, from_bpe,
                                            dict_first_phone(ps_search_dict(ngs), to->wid));
            
            if (score == WORST_SCORE)
                continue;
            
            else
                score = ascr + (score - from_bpe->score);
            if (score BETTER_THAN 0) {
                




                ps_lattice_link(dag, from, to, -424242, from_bpe->frame);
                ++nlink;
                from->reachable = TRUE;
            }
            else if (score BETTER_THAN WORST_SCORE) {
                ps_lattice_link(dag, from, to, score, from_bpe->frame);
                ++nlink;
                from->reachable = TRUE;
            }
        }
    }

    
    if (!dag->start->reachable) {
        E_ERROR("End node of lattice isolated; unreachable\n");
        goto error_out;
    }

    for (node = dag->nodes; node; node = node->next) {
        
        node->fef = ngs->bp_table[node->fef].frame;
        node->lef = ngs->bp_table[node->lef].frame;
        
        node->basewid = dict_basewid(search->dict, node->wid);
    }

    
    for (node = dag->nodes; node; node = node->next) {
        ps_latnode_t *alt;
        
        for (alt = node->next; alt && alt->sf == node->sf; alt = alt->next) {
            if (alt->basewid == node->basewid) {
                alt->alt = node->alt;
                node->alt = alt;
                break;
            }
        }
    }
    E_INFO("Lattice has %d nodes, %d links\n", dag->n_nodes, nlink);

    


    if (dict_filler_word(ps_search_dict(ngs), dag->end->wid))
        dag->end->basewid = ps_search_finish_wid(ngs);

    
    ps_lattice_delete_unreachable(dag);

    
    ps_lattice_penalize_fillers(dag, ngs->silpen, ngs->fillpen);

    search->dag = dag;
    return dag;

error_out:
    ps_lattice_free(dag);
    return NULL;
}

void ngram_search_set_lm(ngram_model_t *lm)
{
    default_lm = ngram_model_retain(lm);
}

