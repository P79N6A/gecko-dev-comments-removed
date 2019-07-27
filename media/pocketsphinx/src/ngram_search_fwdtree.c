









































#include <string.h>
#include <assert.h>


#include <sphinxbase/ckd_alloc.h>
#include <sphinxbase/listelem_alloc.h>
#include <sphinxbase/err.h>


#include "ngram_search_fwdtree.h"
#include "phone_loop_search.h"


#define __CHAN_DUMP__		0
#if __CHAN_DUMP__
#define chan_v_eval(chan) hmm_dump_vit_eval(&(chan)->hmm, stderr)
#else
#define chan_v_eval(chan) hmm_vit_eval(&(chan)->hmm)
#endif





static void
init_search_tree(ngram_search_t *ngs)
{
    int32 w, ndiph, i, n_words, n_ci;
    dict_t *dict = ps_search_dict(ngs);
    bitvec_t *dimap;

    n_words = ps_search_n_words(ngs);
    ngs->homophone_set = ckd_calloc(n_words, sizeof(*ngs->homophone_set));

    
    ndiph = 0;
    ngs->n_1ph_words = 0;
    n_ci = bin_mdef_n_ciphone(ps_search_acmod(ngs)->mdef);
    
    dimap = bitvec_alloc(n_ci * n_ci);
    for (w = 0; w < n_words; w++) {
        if (!dict_real_word(dict, w))
            continue;
        if (dict_is_single_phone(dict, w))
            ++ngs->n_1ph_words;
        else {
            int ph0, ph1;
            ph0 = dict_first_phone(dict, w);
            ph1 = dict_second_phone(dict, w);
            
            if (bitvec_is_clear(dimap, ph0 * n_ci + ph1)) {
                bitvec_set(dimap, ph0 * n_ci + ph1);
                ++ndiph;
            }
        }
    }
    E_INFO("%d unique initial diphones\n", ndiph);
    bitvec_free(dimap);

    
    ngs->n_1ph_words += dict_num_fillers(dict) + 2;
    ngs->n_root_chan_alloc = ndiph + 1;
    

    for (w = 0; w < n_words; ++w) {
        if (dict_real_word(dict, w))
            continue;
        if (!dict_is_single_phone(dict, w)) {
            E_WARN("Filler word %d = %s has more than one phone, ignoring it.\n",
                   w, dict_wordstr(dict, w));
            --ngs->n_1ph_words;
        }
    }

    
    ngs->root_chan =
        ckd_calloc(ngs->n_root_chan_alloc, sizeof(*ngs->root_chan));
    for (i = 0; i < ngs->n_root_chan_alloc; i++) {
        hmm_init(ngs->hmmctx, &ngs->root_chan[i].hmm, TRUE, -1, -1);
        ngs->root_chan[i].penult_phn_wid = -1;
        ngs->root_chan[i].next = NULL;
    }

    

    ngs->rhmm_1ph = ckd_calloc(ngs->n_1ph_words, sizeof(*ngs->rhmm_1ph));
    i = 0;
    for (w = 0; w < n_words; w++) {
        if (!dict_is_single_phone(dict, w))
            continue;
        
        ngs->rhmm_1ph[i].ci2phone = bin_mdef_silphone(ps_search_acmod(ngs)->mdef);
        ngs->rhmm_1ph[i].ciphone = dict_first_phone(dict, w);
        hmm_init(ngs->hmmctx, &ngs->rhmm_1ph[i].hmm, TRUE,
                 bin_mdef_pid2ssid(ps_search_acmod(ngs)->mdef, ngs->rhmm_1ph[i].ciphone),
                 bin_mdef_pid2tmatid(ps_search_acmod(ngs)->mdef, ngs->rhmm_1ph[i].ciphone));
        ngs->rhmm_1ph[i].next = NULL;

        ngs->word_chan[w] = (chan_t *) &(ngs->rhmm_1ph[i]);
        i++;
    }

    ngs->single_phone_wid = ckd_calloc(ngs->n_1ph_words,
                                       sizeof(*ngs->single_phone_wid));
    E_INFO("%d root, %d non-root channels, %d single-phone words\n",
           ngs->n_root_chan, ngs->n_nonroot_chan, ngs->n_1ph_words);
}




static void
init_nonroot_chan(ngram_search_t *ngs, chan_t * hmm, int32 ph, int32 ci, int32 tmatid)
{
    hmm->next = NULL;
    hmm->alt = NULL;
    hmm->info.penult_phn_wid = -1;
    hmm->ciphone = ci;
    hmm_init(ngs->hmmctx, &hmm->hmm, FALSE, ph, tmatid);
}











static void
create_search_tree(ngram_search_t *ngs)
{
    chan_t *hmm;
    root_chan_t *rhmm;
    int32 w, i, j, p, ph, tmatid;
    int32 n_words;
    dict_t *dict = ps_search_dict(ngs);
    dict2pid_t *d2p = ps_search_dict2pid(ngs);

    n_words = ps_search_n_words(ngs);

    E_INFO("Creating search tree\n");

    for (w = 0; w < n_words; w++)
        ngs->homophone_set[w] = -1;

    E_INFO("before: %d root, %d non-root channels, %d single-phone words\n",
           ngs->n_root_chan, ngs->n_nonroot_chan, ngs->n_1ph_words);

    ngs->n_1ph_LMwords = 0;
    ngs->n_root_chan = 0;
    ngs->n_nonroot_chan = 0;

    for (w = 0; w < n_words; w++) {
        int ciphone, ci2phone;

        
        if (!ngram_model_set_known_wid(ngs->lmset, dict_basewid(dict, w)))
            continue;

        
        if (dict_is_single_phone(dict, w)) {
            E_DEBUG(1,("single_phone_wid[%d] = %s\n",
                       ngs->n_1ph_LMwords, dict_wordstr(dict, w)));
            ngs->single_phone_wid[ngs->n_1ph_LMwords++] = w;
            continue;
        }

        

        ciphone = dict_first_phone(dict, w);
        ci2phone = dict_second_phone(dict, w);
        for (i = 0; i < ngs->n_root_chan; ++i) {
            if (ngs->root_chan[i].ciphone == ciphone
                && ngs->root_chan[i].ci2phone == ci2phone)
                break;
        }
        if (i == ngs->n_root_chan) {
            rhmm = &(ngs->root_chan[ngs->n_root_chan]);
            rhmm->hmm.tmatid = bin_mdef_pid2tmatid(ps_search_acmod(ngs)->mdef, ciphone);
            
            hmm_mpx_ssid(&rhmm->hmm, 0) =
                bin_mdef_pid2ssid(ps_search_acmod(ngs)->mdef, ciphone);
            rhmm->ciphone = ciphone;
            rhmm->ci2phone = ci2phone;
            ngs->n_root_chan++;
        }
        else
            rhmm = &(ngs->root_chan[i]);

        E_DEBUG(3,("word %s rhmm %d\n", dict_wordstr(dict, w), rhmm - ngs->root_chan));
        
        if (dict_pronlen(dict, w) == 2) {
            
            if ((j = rhmm->penult_phn_wid) < 0)
                rhmm->penult_phn_wid = w;
            else {
                for (; ngs->homophone_set[j] >= 0; j = ngs->homophone_set[j]);
                ngs->homophone_set[j] = w;
            }
        }
        else {
            
            ph = dict2pid_internal(d2p, w, 1);
            tmatid = bin_mdef_pid2tmatid(ps_search_acmod(ngs)->mdef, dict_pron(dict, w, 1));
            hmm = rhmm->next;
            if (hmm == NULL) {
                rhmm->next = hmm = listelem_malloc(ngs->chan_alloc);
                init_nonroot_chan(ngs, hmm, ph, dict_pron(dict, w, 1), tmatid);
                ngs->n_nonroot_chan++;
            }
            else {
                chan_t *prev_hmm = NULL;

                for (; hmm && (hmm_nonmpx_ssid(&hmm->hmm) != ph); hmm = hmm->alt)
                    prev_hmm = hmm;
                if (!hmm) {     
                    prev_hmm->alt = hmm = listelem_malloc(ngs->chan_alloc);
                    init_nonroot_chan(ngs, hmm, ph, dict_pron(dict, w, 1), tmatid);
                    ngs->n_nonroot_chan++;
                }
            }
            E_DEBUG(3,("phone %s = %d\n",
                       bin_mdef_ciphone_str(ps_search_acmod(ngs)->mdef,
                                            dict_second_phone(dict, w)), ph));
            for (p = 2; p < dict_pronlen(dict, w) - 1; p++) {
                ph = dict2pid_internal(d2p, w, p);
                tmatid = bin_mdef_pid2tmatid(ps_search_acmod(ngs)->mdef, dict_pron(dict, w, p));
                if (!hmm->next) {
                    hmm->next = listelem_malloc(ngs->chan_alloc);
                    hmm = hmm->next;
                    init_nonroot_chan(ngs, hmm, ph, dict_pron(dict, w, p), tmatid);
                    ngs->n_nonroot_chan++;
                }
                else {
                    chan_t *prev_hmm = NULL;

                    for (hmm = hmm->next; hmm && (hmm_nonmpx_ssid(&hmm->hmm) != ph);
                         hmm = hmm->alt)
                        prev_hmm = hmm;
                    if (!hmm) { 
                        prev_hmm->alt = hmm = listelem_malloc(ngs->chan_alloc);
                        init_nonroot_chan(ngs, hmm, ph, dict_pron(dict, w, p), tmatid);
                        ngs->n_nonroot_chan++;
                    }
                }
                E_DEBUG(3,("phone %s = %d\n",
                           bin_mdef_ciphone_str(ps_search_acmod(ngs)->mdef,
                                                dict_pron(dict, w, p)), ph));
            }

            
            if ((j = hmm->info.penult_phn_wid) < 0)
                hmm->info.penult_phn_wid = w;
            else {
                for (; ngs->homophone_set[j] >= 0; j = ngs->homophone_set[j]);
                ngs->homophone_set[j] = w;
            }
        }
    }

    ngs->n_1ph_words = ngs->n_1ph_LMwords;

    
    for (w = 0; w < n_words; ++w) {
        
        if (!dict_is_single_phone(dict, w))
            continue;
        
        if (dict_real_word(dict, w))
            continue;
        if (ngram_model_set_known_wid(ngs->lmset, dict_basewid(dict, w)))
            continue;
        E_DEBUG(1,("single_phone_wid[%d] = %s\n",
                   ngs->n_1ph_words, dict_wordstr(dict, w)));
        ngs->single_phone_wid[ngs->n_1ph_words++] = w;
    }

    if (ngs->n_nonroot_chan >= ngs->max_nonroot_chan) {
        
        ngs->max_nonroot_chan = ngs->n_nonroot_chan + 128;
        E_INFO("after: max nonroot chan increased to %d\n", ngs->max_nonroot_chan);

        
        if (ngs->active_chan_list)
            ckd_free_2d(ngs->active_chan_list);
        ngs->active_chan_list = ckd_calloc_2d(2, ngs->max_nonroot_chan,
                                              sizeof(**ngs->active_chan_list));
    }

    if (!ngs->n_root_chan)
	E_ERROR("No word from the language model has pronunciation in the dictionary\n");

    E_INFO("after: %d root, %d non-root channels, %d single-phone words\n",
           ngs->n_root_chan, ngs->n_nonroot_chan, ngs->n_1ph_words);
}

static void
reinit_search_subtree(ngram_search_t *ngs, chan_t * hmm)
{
    chan_t *child, *sibling;

    
    for (child = hmm->next; child; child = sibling) {
        sibling = child->alt;
        reinit_search_subtree(ngs, child);
    }

    
    hmm_deinit(&hmm->hmm);
    listelem_free(ngs->chan_alloc, hmm);
}





static void
reinit_search_tree(ngram_search_t *ngs)
{
    int32 i;
    chan_t *hmm, *sibling;

    for (i = 0; i < ngs->n_root_chan; i++) {
        hmm = ngs->root_chan[i].next;

        while (hmm) {
            sibling = hmm->alt;
            reinit_search_subtree(ngs, hmm);
            hmm = sibling;
        }

        ngs->root_chan[i].penult_phn_wid = -1;
        ngs->root_chan[i].next = NULL;
    }
    ngs->n_nonroot_chan = 0;
}

void
ngram_fwdtree_init(ngram_search_t *ngs)
{
    
    ngs->bestbp_rc = ckd_calloc(bin_mdef_n_ciphone(ps_search_acmod(ngs)->mdef),
                                sizeof(*ngs->bestbp_rc));
    ngs->lastphn_cand = ckd_calloc(ps_search_n_words(ngs),
                                   sizeof(*ngs->lastphn_cand));
    init_search_tree(ngs);
    create_search_tree(ngs);
}

static void
deinit_search_tree(ngram_search_t *ngs)
{
    int i, w, n_words;

    n_words = ps_search_n_words(ngs);
    for (i = 0; i < ngs->n_root_chan_alloc; i++) {
        hmm_deinit(&ngs->root_chan[i].hmm);
    }
    if (ngs->rhmm_1ph) {
        for (i = w = 0; w < n_words; ++w) {
            if (!dict_is_single_phone(ps_search_dict(ngs), w))
                continue;
            hmm_deinit(&ngs->rhmm_1ph[i].hmm);
            ++i;
        }
        ckd_free(ngs->rhmm_1ph);
        ngs->rhmm_1ph = NULL;
    }
    ngs->n_root_chan = 0;
    ngs->n_root_chan_alloc = 0;
    ckd_free(ngs->root_chan);
    ngs->root_chan = NULL;
    ckd_free(ngs->single_phone_wid);
    ngs->single_phone_wid = NULL;
    ckd_free(ngs->homophone_set);
    ngs->homophone_set = NULL;
}

void
ngram_fwdtree_deinit(ngram_search_t *ngs)
{
    double n_speech = (double)ngs->n_tot_frame
            / cmd_ln_int32_r(ps_search_config(ngs), "-frate");

    E_INFO("TOTAL fwdtree %.2f CPU %.3f xRT\n",
           ngs->fwdtree_perf.t_tot_cpu,
           ngs->fwdtree_perf.t_tot_cpu / n_speech);
    E_INFO("TOTAL fwdtree %.2f wall %.3f xRT\n",
           ngs->fwdtree_perf.t_tot_elapsed,
           ngs->fwdtree_perf.t_tot_elapsed / n_speech);

    
    reinit_search_tree(ngs);
    
    deinit_search_tree(ngs);
    
    ngs->max_nonroot_chan = 0;
    ckd_free_2d(ngs->active_chan_list);
    ngs->active_chan_list = NULL;
    ckd_free(ngs->cand_sf);
    ngs->cand_sf = NULL;
    ckd_free(ngs->bestbp_rc);
    ngs->bestbp_rc = NULL;
    ckd_free(ngs->lastphn_cand);
    ngs->lastphn_cand = NULL;
}

int
ngram_fwdtree_reinit(ngram_search_t *ngs)
{
    
    reinit_search_tree(ngs);
    
    deinit_search_tree(ngs);
    
    ckd_free(ngs->lastphn_cand);
    ngs->lastphn_cand = ckd_calloc(ps_search_n_words(ngs),
                                   sizeof(*ngs->lastphn_cand));
    ckd_free(ngs->word_chan);
    ngs->word_chan = ckd_calloc(ps_search_n_words(ngs),
                                sizeof(*ngs->word_chan));
    
    init_search_tree(ngs);
    create_search_tree(ngs);
    return 0;
}

void
ngram_fwdtree_start(ngram_search_t *ngs)
{
    ps_search_t *base = (ps_search_t *)ngs;
    int32 i, w, n_words;
    root_chan_t *rhmm;

    n_words = ps_search_n_words(ngs);

    
    memset(&ngs->st, 0, sizeof(ngs->st));
    ptmr_reset(&ngs->fwdtree_perf);
    ptmr_start(&ngs->fwdtree_perf);

    
    ngs->bpidx = 0;
    ngs->bss_head = 0;

    
    for (i = 0; i < n_words; ++i)
        ngs->word_lat_idx[i] = NO_BP;

    
    ngs->n_active_chan[0] = ngs->n_active_chan[1] = 0;
    ngs->n_active_word[0] = ngs->n_active_word[1] = 0;

    
    ngs->best_score = 0;
    ngs->renormalized = 0;

    
    for (i = 0; i < n_words; i++)
        ngs->last_ltrans[i].sf = -1;
    ngs->n_frame = 0;

    
    ckd_free(base->hyp_str);
    base->hyp_str = NULL;

    

    for (i = 0; i < ngs->n_1ph_words; i++) {
        w = ngs->single_phone_wid[i];
        rhmm = (root_chan_t *) ngs->word_chan[w];
        hmm_clear(&rhmm->hmm);
    }

    
    rhmm = (root_chan_t *) ngs->word_chan[dict_startwid(ps_search_dict(ngs))];
    hmm_clear(&rhmm->hmm);
    hmm_enter(&rhmm->hmm, 0, NO_BP, 0);
}





static void
compute_sen_active(ngram_search_t *ngs, int frame_idx)
{
    root_chan_t *rhmm;
    chan_t *hmm, **acl;
    int32 i, w, *awl;

    acmod_clear_active(ps_search_acmod(ngs));

    
    for (i = ngs->n_root_chan, rhmm = ngs->root_chan; i > 0; --i, rhmm++) {
        if (hmm_frame(&rhmm->hmm) == frame_idx)
            acmod_activate_hmm(ps_search_acmod(ngs), &rhmm->hmm);
    }

    
    i = ngs->n_active_chan[frame_idx & 0x1];
    acl = ngs->active_chan_list[frame_idx & 0x1];
    for (hmm = *(acl++); i > 0; --i, hmm = *(acl++)) {
        acmod_activate_hmm(ps_search_acmod(ngs), &hmm->hmm);
    }

    
    i = ngs->n_active_word[frame_idx & 0x1];
    awl = ngs->active_word_list[frame_idx & 0x1];
    for (w = *(awl++); i > 0; --i, w = *(awl++)) {
        for (hmm = ngs->word_chan[w]; hmm; hmm = hmm->next) {
            acmod_activate_hmm(ps_search_acmod(ngs), &hmm->hmm);
        }
    }
    for (i = 0; i < ngs->n_1ph_words; i++) {
        w = ngs->single_phone_wid[i];
        rhmm = (root_chan_t *) ngs->word_chan[w];

        if (hmm_frame(&rhmm->hmm) == frame_idx)
            acmod_activate_hmm(ps_search_acmod(ngs), &rhmm->hmm);
    }
}

static void
renormalize_scores(ngram_search_t *ngs, int frame_idx, int32 norm)
{
    root_chan_t *rhmm;
    chan_t *hmm, **acl;
    int32 i, w, *awl;

    
    for (i = ngs->n_root_chan, rhmm = ngs->root_chan; i > 0; --i, rhmm++) {
        if (hmm_frame(&rhmm->hmm) == frame_idx) {
            hmm_normalize(&rhmm->hmm, norm);
        }
    }

    
    i = ngs->n_active_chan[frame_idx & 0x1];
    acl = ngs->active_chan_list[frame_idx & 0x1];
    for (hmm = *(acl++); i > 0; --i, hmm = *(acl++)) {
        hmm_normalize(&hmm->hmm, norm);
    }

    
    i = ngs->n_active_word[frame_idx & 0x1];
    awl = ngs->active_word_list[frame_idx & 0x1];
    for (w = *(awl++); i > 0; --i, w = *(awl++)) {
        for (hmm = ngs->word_chan[w]; hmm; hmm = hmm->next) {
            hmm_normalize(&hmm->hmm, norm);
        }
    }
    for (i = 0; i < ngs->n_1ph_words; i++) {
        w = ngs->single_phone_wid[i];
        rhmm = (root_chan_t *) ngs->word_chan[w];
        if (hmm_frame(&rhmm->hmm) == frame_idx) {
            hmm_normalize(&rhmm->hmm, norm);
        }
    }

    ngs->renormalized = TRUE;
}

static int32
eval_root_chan(ngram_search_t *ngs, int frame_idx)
{
    root_chan_t *rhmm;
    int32 i, bestscore;

    bestscore = WORST_SCORE;
    for (i = ngs->n_root_chan, rhmm = ngs->root_chan; i > 0; --i, rhmm++) {
        if (hmm_frame(&rhmm->hmm) == frame_idx) {
            int32 score = chan_v_eval(rhmm);
            if (score BETTER_THAN bestscore)
                bestscore = score;
            ++ngs->st.n_root_chan_eval;
        }
    }
    return (bestscore);
}

static int32
eval_nonroot_chan(ngram_search_t *ngs, int frame_idx)
{
    chan_t *hmm, **acl;
    int32 i, bestscore;

    i = ngs->n_active_chan[frame_idx & 0x1];
    acl = ngs->active_chan_list[frame_idx & 0x1];
    bestscore = WORST_SCORE;
    ngs->st.n_nonroot_chan_eval += i;

    for (hmm = *(acl++); i > 0; --i, hmm = *(acl++)) {
        int32 score = chan_v_eval(hmm);
        assert(hmm_frame(&hmm->hmm) == frame_idx);
        if (score BETTER_THAN bestscore)
            bestscore = score;
    }

    return bestscore;
}

static int32
eval_word_chan(ngram_search_t *ngs, int frame_idx)
{
    root_chan_t *rhmm;
    chan_t *hmm;
    int32 i, w, bestscore, *awl, j, k;

    k = 0;
    bestscore = WORST_SCORE;
    awl = ngs->active_word_list[frame_idx & 0x1];

    i = ngs->n_active_word[frame_idx & 0x1];
    for (w = *(awl++); i > 0; --i, w = *(awl++)) {
        assert(bitvec_is_set(ngs->word_active, w));
        bitvec_clear(ngs->word_active, w);
        assert(ngs->word_chan[w] != NULL);

        for (hmm = ngs->word_chan[w]; hmm; hmm = hmm->next) {
            int32 score;

            assert(hmm_frame(&hmm->hmm) == frame_idx);
            score = chan_v_eval(hmm);
            

            if (score BETTER_THAN bestscore)
                bestscore = score;

            k++;
        }
    }

    
    j = 0;
    for (i = 0; i < ngs->n_1ph_words; i++) {
        int32 score;

        w = ngs->single_phone_wid[i];
        rhmm = (root_chan_t *) ngs->word_chan[w];
        if (hmm_frame(&rhmm->hmm) < frame_idx)
            continue;

        score = chan_v_eval(rhmm);
        
        if (score BETTER_THAN bestscore && w != ps_search_finish_wid(ngs))
            bestscore = score;

        j++;
    }

    ngs->st.n_last_chan_eval += k + j;
    ngs->st.n_nonroot_chan_eval += k + j;
    ngs->st.n_word_lastchan_eval +=
        ngs->n_active_word[frame_idx & 0x1] + j;

    return bestscore;
}

static int32
evaluate_channels(ngram_search_t *ngs, int16 const *senone_scores, int frame_idx)
{
    int32 bs;

    hmm_context_set_senscore(ngs->hmmctx, senone_scores);
    ngs->best_score = eval_root_chan(ngs, frame_idx);
    if ((bs = eval_nonroot_chan(ngs, frame_idx)) BETTER_THAN ngs->best_score)
        ngs->best_score = bs;
    if ((bs = eval_word_chan(ngs, frame_idx)) BETTER_THAN ngs->best_score)
        ngs->best_score = bs;
    ngs->last_phone_best_score = bs;

    return ngs->best_score;
}






static void
prune_root_chan(ngram_search_t *ngs, int frame_idx)
{
    root_chan_t *rhmm;
    chan_t *hmm;
    int32 i, nf, w;
    int32 thresh, newphone_thresh, lastphn_thresh, newphone_score;
    chan_t **nacl;              
    lastphn_cand_t *candp;
    phone_loop_search_t *pls;

    nf = frame_idx + 1;
    thresh = ngs->best_score + ngs->dynamic_beam;
    newphone_thresh = ngs->best_score + ngs->pbeam;
    lastphn_thresh = ngs->best_score + ngs->lpbeam;
    nacl = ngs->active_chan_list[nf & 0x1];
    pls = (phone_loop_search_t *)ps_search_lookahead(ngs);

    for (i = 0, rhmm = ngs->root_chan; i < ngs->n_root_chan; i++, rhmm++) {
        E_DEBUG(3,("Root channel %d frame %d score %d thresh %d\n",
                   i, hmm_frame(&rhmm->hmm), hmm_bestscore(&rhmm->hmm), thresh));
        
        if (hmm_frame(&rhmm->hmm) < frame_idx)
            continue;

        if (hmm_bestscore(&rhmm->hmm) BETTER_THAN thresh) {
            hmm_frame(&rhmm->hmm) = nf;  
            E_DEBUG(3,("Preserving root channel %d score %d\n", i, hmm_bestscore(&rhmm->hmm)));
            
            
            newphone_score = hmm_out_score(&rhmm->hmm) + ngs->pip;
            if (pls != NULL || newphone_score BETTER_THAN newphone_thresh) {
                for (hmm = rhmm->next; hmm; hmm = hmm->alt) {
                    int32 pl_newphone_score = newphone_score
                        + phone_loop_search_score(pls, hmm->ciphone);
                    if (pl_newphone_score BETTER_THAN newphone_thresh) {
                        if ((hmm_frame(&hmm->hmm) < frame_idx)
                            || (newphone_score BETTER_THAN hmm_in_score(&hmm->hmm))) {
                            hmm_enter(&hmm->hmm, newphone_score,
                                      hmm_out_history(&rhmm->hmm), nf);
                            *(nacl++) = hmm;
                        }
                    }
                }
            }

            




            if (pls != NULL || newphone_score BETTER_THAN lastphn_thresh) {
                for (w = rhmm->penult_phn_wid; w >= 0;
                     w = ngs->homophone_set[w]) {
                    int32 pl_newphone_score = newphone_score
                        + phone_loop_search_score
                        (pls, dict_last_phone(ps_search_dict(ngs),w));
                    E_DEBUG(3,("word %s newphone_score %d\n", dict_wordstr(ps_search_dict(ngs), w), newphone_score));
                    if (pl_newphone_score BETTER_THAN lastphn_thresh) {
                        candp = ngs->lastphn_cand + ngs->n_lastphn_cand;
                        ngs->n_lastphn_cand++;
                        candp->wid = w;
                        candp->score =
                            newphone_score - ngs->nwpen;
                        candp->bp = hmm_out_history(&rhmm->hmm);
                    }
                }
            }
        }
    }
    ngs->n_active_chan[nf & 0x1] = (int)(nacl - ngs->active_chan_list[nf & 0x1]);
}





static void
prune_nonroot_chan(ngram_search_t *ngs, int frame_idx)
{
    chan_t *hmm, *nexthmm;
    int32 nf, w, i;
    int32 thresh, newphone_thresh, lastphn_thresh, newphone_score;
    chan_t **acl, **nacl;       
    lastphn_cand_t *candp;
    phone_loop_search_t *pls;

    nf = frame_idx + 1;

    thresh = ngs->best_score + ngs->dynamic_beam;
    newphone_thresh = ngs->best_score + ngs->pbeam;
    lastphn_thresh = ngs->best_score + ngs->lpbeam;
    pls = (phone_loop_search_t *)ps_search_lookahead(ngs);

    acl = ngs->active_chan_list[frame_idx & 0x1];   
    nacl = ngs->active_chan_list[nf & 0x1] + ngs->n_active_chan[nf & 0x1];

    for (i = ngs->n_active_chan[frame_idx & 0x1], hmm = *(acl++); i > 0;
         --i, hmm = *(acl++)) {
        assert(hmm_frame(&hmm->hmm) >= frame_idx);

        if (hmm_bestscore(&hmm->hmm) BETTER_THAN thresh) {
            
            if (hmm_frame(&hmm->hmm) != nf) {
                hmm_frame(&hmm->hmm) = nf;
                *(nacl++) = hmm;
            }

            
            newphone_score = hmm_out_score(&hmm->hmm) + ngs->pip;
            if (pls != NULL || newphone_score BETTER_THAN newphone_thresh) {
                for (nexthmm = hmm->next; nexthmm; nexthmm = nexthmm->alt) {
                    int32 pl_newphone_score = newphone_score
                        + phone_loop_search_score(pls, nexthmm->ciphone);
                    if ((pl_newphone_score BETTER_THAN newphone_thresh)
                        && ((hmm_frame(&nexthmm->hmm) < frame_idx)
                            || (newphone_score
                                BETTER_THAN hmm_in_score(&nexthmm->hmm)))) {
                        if (hmm_frame(&nexthmm->hmm) != nf) {
                            
                            *(nacl++) = nexthmm;
                        }
                        hmm_enter(&nexthmm->hmm, newphone_score,
                                  hmm_out_history(&hmm->hmm), nf);
                    }
                }
            }

            




            if (pls != NULL || newphone_score BETTER_THAN lastphn_thresh) {
                for (w = hmm->info.penult_phn_wid; w >= 0;
                     w = ngs->homophone_set[w]) {
                    int32 pl_newphone_score = newphone_score
                        + phone_loop_search_score
                        (pls, dict_last_phone(ps_search_dict(ngs),w));
                    if (pl_newphone_score BETTER_THAN lastphn_thresh) {
                        candp = ngs->lastphn_cand + ngs->n_lastphn_cand;
                        ngs->n_lastphn_cand++;
                        candp->wid = w;
                        candp->score =
                            newphone_score - ngs->nwpen;
                        candp->bp = hmm_out_history(&hmm->hmm);
                    }
                }
            }
        }
        else if (hmm_frame(&hmm->hmm) != nf) {
            hmm_clear(&hmm->hmm);
        }
    }
    ngs->n_active_chan[nf & 0x1] = (int)(nacl - ngs->active_chan_list[nf & 0x1]);
}






static void
last_phone_transition(ngram_search_t *ngs, int frame_idx)
{
    int32 i, j, k, nf, bp, bpend, w;
    lastphn_cand_t *candp;
    int32 *nawl;
    int32 thresh;
    int32 bestscore, dscr;
    chan_t *hmm;
    bptbl_t *bpe;
    int32 n_cand_sf = 0;

    nf = frame_idx + 1;
    nawl = ngs->active_word_list[nf & 0x1];
    ngs->st.n_lastphn_cand_utt += ngs->n_lastphn_cand;

    
    
    for (i = 0, candp = ngs->lastphn_cand; i < ngs->n_lastphn_cand; i++, candp++) {
        int32 start_score;

        
        if (candp->bp == -1)
            continue;
        
        bpe = &(ngs->bp_table[candp->bp]);

        
        start_score = ngram_search_exit_score
            (ngs, bpe, dict_first_phone(ps_search_dict(ngs), candp->wid));
        assert(start_score BETTER_THAN WORST_SCORE);
        candp->score -= start_score;

        



        

        if (ngs->last_ltrans[candp->wid].sf != bpe->frame + 1) {
            

            for (j = 0; j < n_cand_sf; j++) {
                if (ngs->cand_sf[j].bp_ef == bpe->frame)
                    break;
            }
            
            if (j < n_cand_sf)
                candp->next = ngs->cand_sf[j].cand;
            else {
                
                if (n_cand_sf >= ngs->cand_sf_alloc) {
                    if (ngs->cand_sf_alloc == 0) {
                        ngs->cand_sf =
                            ckd_calloc(CAND_SF_ALLOCSIZE,
                                       sizeof(*ngs->cand_sf));
                        ngs->cand_sf_alloc = CAND_SF_ALLOCSIZE;
                    }
                    else {
                        ngs->cand_sf_alloc += CAND_SF_ALLOCSIZE;
                        ngs->cand_sf = ckd_realloc(ngs->cand_sf,
                                                   ngs->cand_sf_alloc
                                                   * sizeof(*ngs->cand_sf));
                        E_INFO("cand_sf[] increased to %d entries\n",
                               ngs->cand_sf_alloc);
                    }
                }

                
                j = n_cand_sf++;
                candp->next = -1; 
                ngs->cand_sf[j].bp_ef = bpe->frame;
            }
            
            ngs->cand_sf[j].cand = i;

            ngs->last_ltrans[candp->wid].dscr = WORST_SCORE;
            ngs->last_ltrans[candp->wid].sf = bpe->frame + 1;
        }
    }

    
    for (i = 0; i < n_cand_sf; i++) {
        
        bp = ngs->bp_table_idx[ngs->cand_sf[i].bp_ef];
        bpend = ngs->bp_table_idx[ngs->cand_sf[i].bp_ef + 1];
        for (bpe = &(ngs->bp_table[bp]); bp < bpend; bp++, bpe++) {
            if (!bpe->valid)
                continue;
            
            for (j = ngs->cand_sf[i].cand; j >= 0; j = candp->next) {
                int32 n_used;
                candp = &(ngs->lastphn_cand[j]);
                dscr = 
                    ngram_search_exit_score
                    (ngs, bpe, dict_first_phone(ps_search_dict(ngs), candp->wid));
                if (dscr BETTER_THAN WORST_SCORE) {
                    assert(!dict_filler_word(ps_search_dict(ngs), candp->wid));
                    dscr += ngram_tg_score(ngs->lmset,
                                           dict_basewid(ps_search_dict(ngs), candp->wid),
                                           bpe->real_wid,
                                           bpe->prev_real_wid,
                                           &n_used)>>SENSCR_SHIFT;
                }

                if (dscr BETTER_THAN ngs->last_ltrans[candp->wid].dscr) {
                    ngs->last_ltrans[candp->wid].dscr = dscr;
                    ngs->last_ltrans[candp->wid].bp = bp;
                }
            }
        }
    }

    
    bestscore = ngs->last_phone_best_score;
    for (i = 0, candp = ngs->lastphn_cand; i < ngs->n_lastphn_cand; i++, candp++) {
        candp->score += ngs->last_ltrans[candp->wid].dscr;
        candp->bp = ngs->last_ltrans[candp->wid].bp;

        if (candp->score BETTER_THAN bestscore)
            bestscore = candp->score;
    }
    ngs->last_phone_best_score = bestscore;

    
    thresh = bestscore + ngs->lponlybeam;
    for (i = ngs->n_lastphn_cand, candp = ngs->lastphn_cand; i > 0; --i, candp++) {
        if (candp->score BETTER_THAN thresh) {
            w = candp->wid;

            ngram_search_alloc_all_rc(ngs, w);

            k = 0;
            for (hmm = ngs->word_chan[w]; hmm; hmm = hmm->next) {
                if ((hmm_frame(&hmm->hmm) < frame_idx)
                    || (candp->score BETTER_THAN hmm_in_score(&hmm->hmm))) {
                    assert(hmm_frame(&hmm->hmm) != nf);
                    hmm_enter(&hmm->hmm,
                              candp->score, candp->bp, nf);
                    k++;
                }
            }
            if (k > 0) {
                assert(bitvec_is_clear(ngs->word_active, w));
                assert(!dict_is_single_phone(ps_search_dict(ngs), w));
                *(nawl++) = w;
                bitvec_set(ngs->word_active, w);
            }
        }
    }
    ngs->n_active_word[nf & 0x1] = (int)(nawl - ngs->active_word_list[nf & 0x1]);
}





static void
prune_word_chan(ngram_search_t *ngs, int frame_idx)
{
    root_chan_t *rhmm;
    chan_t *hmm, *thmm;
    chan_t **phmmp;             
    int32 nf, w, i, k;
    int32 newword_thresh, lastphn_thresh;
    int32 *awl, *nawl;

    nf = frame_idx + 1;
    newword_thresh = ngs->last_phone_best_score + ngs->wbeam;
    lastphn_thresh = ngs->last_phone_best_score + ngs->lponlybeam;

    awl = ngs->active_word_list[frame_idx & 0x1];
    nawl = ngs->active_word_list[nf & 0x1] + ngs->n_active_word[nf & 0x1];

    
    for (i = ngs->n_active_word[frame_idx & 0x1], w = *(awl++); i > 0;
         --i, w = *(awl++)) {
        k = 0;
        phmmp = &(ngs->word_chan[w]);
        for (hmm = ngs->word_chan[w]; hmm; hmm = thmm) {
            assert(hmm_frame(&hmm->hmm) >= frame_idx);

            thmm = hmm->next;
            if (hmm_bestscore(&hmm->hmm) BETTER_THAN lastphn_thresh) {
                
                hmm_frame(&hmm->hmm) = nf;
                k++;
                phmmp = &(hmm->next);

                
                if (hmm_out_score(&hmm->hmm) BETTER_THAN newword_thresh) {
                    
                    ngram_search_save_bp(ngs, frame_idx, w,
                                 hmm_out_score(&hmm->hmm),
                                 hmm_out_history(&hmm->hmm),
                                 hmm->info.rc_id);
                }
            }
            else if (hmm_frame(&hmm->hmm) == nf) {
                phmmp = &(hmm->next);
            }
            else {
                hmm_deinit(&hmm->hmm);
                listelem_free(ngs->chan_alloc, hmm);
                *phmmp = thmm;
            }
        }
        if ((k > 0) && (bitvec_is_clear(ngs->word_active, w))) {
            assert(!dict_is_single_phone(ps_search_dict(ngs), w));
            *(nawl++) = w;
            bitvec_set(ngs->word_active, w);
        }
    }
    ngs->n_active_word[nf & 0x1] = (int)(nawl - ngs->active_word_list[nf & 0x1]);

    



    for (i = 0; i < ngs->n_1ph_words; i++) {
        w = ngs->single_phone_wid[i];
        rhmm = (root_chan_t *) ngs->word_chan[w];
        E_DEBUG(3,("Single phone word %s frame %d score %d thresh %d outscore %d nwthresh %d\n",
                   dict_wordstr(ps_search_dict(ngs),w),
                   hmm_frame(&rhmm->hmm), hmm_bestscore(&rhmm->hmm),
                   lastphn_thresh, hmm_out_score(&rhmm->hmm), newword_thresh));
        if (hmm_frame(&rhmm->hmm) < frame_idx)
            continue;
        if (hmm_bestscore(&rhmm->hmm) BETTER_THAN lastphn_thresh) {
            hmm_frame(&rhmm->hmm) = nf;

            
            if (hmm_out_score(&rhmm->hmm) BETTER_THAN newword_thresh) {
                E_DEBUG(4,("Exiting single phone word %s with %d > %d, %d\n",
                           dict_wordstr(ps_search_dict(ngs),w),
                           hmm_out_score(&rhmm->hmm),
                           lastphn_thresh, newword_thresh));
                ngram_search_save_bp(ngs, frame_idx, w,
                             hmm_out_score(&rhmm->hmm),
                             hmm_out_history(&rhmm->hmm), 0);
            }
        }
    }
}

static void
prune_channels(ngram_search_t *ngs, int frame_idx)
{
    
    ngs->n_lastphn_cand = 0;
    
    ngs->dynamic_beam = ngs->beam;
    if (ngs->maxhmmpf != -1
        && ngs->st.n_root_chan_eval + ngs->st.n_nonroot_chan_eval > ngs->maxhmmpf) {
        
        int32 bins[256], bw, nhmms, i;
        root_chan_t *rhmm;
        chan_t **acl, *hmm;

        
        bw = -ngs->beam / 256;
        memset(bins, 0, sizeof(bins));
        
        for (i = 0, rhmm = ngs->root_chan; i < ngs->n_root_chan; i++, rhmm++) {
            int32 b;

            
            b = (ngs->best_score - hmm_bestscore(&rhmm->hmm)) / bw;
            if (b >= 256)
                b = 255;
            ++bins[b];
        }
        
        acl = ngs->active_chan_list[frame_idx & 0x1];       
        for (i = ngs->n_active_chan[frame_idx & 0x1], hmm = *(acl++);
             i > 0; --i, hmm = *(acl++)) {
            int32 b;

            
            b = (ngs->best_score - hmm_bestscore(&hmm->hmm)) / bw;
            if (b >= 256)
                b = 255;
            ++bins[b];
        }
        
        for (i = nhmms = 0; i < 256; ++i) {
            nhmms += bins[i];
            if (nhmms > ngs->maxhmmpf)
                break;
        }
        ngs->dynamic_beam = -(i * bw);
    }

    prune_root_chan(ngs, frame_idx);
    prune_nonroot_chan(ngs, frame_idx);
    last_phone_transition(ngs, frame_idx);
    prune_word_chan(ngs, frame_idx);
}





static void
bptable_maxwpf(ngram_search_t *ngs, int frame_idx)
{
    int32 bp, n;
    int32 bestscr, worstscr;
    bptbl_t *bpe, *bestbpe, *worstbpe;

    
    if (ngs->maxwpf == -1 || ngs->maxwpf == ps_search_n_words(ngs))
        return;

    
    bestscr = (int32) 0x80000000;
    bestbpe = NULL;
    n = 0;
    for (bp = ngs->bp_table_idx[frame_idx]; bp < ngs->bpidx; bp++) {
        bpe = &(ngs->bp_table[bp]);
        if (dict_filler_word(ps_search_dict(ngs), bpe->wid)) {
            if (bpe->score BETTER_THAN bestscr) {
                bestscr = bpe->score;
                bestbpe = bpe;
            }
            bpe->valid = FALSE;
            n++;                
        }
    }
    
    if (bestbpe != NULL) {
        bestbpe->valid = TRUE;
        --n;
    }

    
    n = (ngs->bpidx
         - ngs->bp_table_idx[frame_idx]) - n;  
    for (; n > ngs->maxwpf; --n) {
        
        worstscr = (int32) 0x7fffffff;
        worstbpe = NULL;
        for (bp = ngs->bp_table_idx[frame_idx]; (bp < ngs->bpidx); bp++) {
            bpe = &(ngs->bp_table[bp]);
            if (bpe->valid && (bpe->score WORSE_THAN worstscr)) {
                worstscr = bpe->score;
                worstbpe = bpe;
            }
        }
        
        if (worstbpe == NULL)
            E_FATAL("PANIC: No worst BPtable entry remaining\n");
        worstbpe->valid = FALSE;
    }
}

static void
word_transition(ngram_search_t *ngs, int frame_idx)
{
    int32 i, k, bp, w, nf;
    int32 rc;
    int32 thresh, newscore, pl_newscore;
    bptbl_t *bpe;
    root_chan_t *rhmm;
    struct bestbp_rc_s *bestbp_rc_ptr;
    phone_loop_search_t *pls;
    dict_t *dict = ps_search_dict(ngs);
    dict2pid_t *d2p = ps_search_dict2pid(ngs);

    




    for (i = bin_mdef_n_ciphone(ps_search_acmod(ngs)->mdef) - 1; i >= 0; --i)
        ngs->bestbp_rc[i].score = WORST_SCORE;
    k = 0;
    pls = (phone_loop_search_t *)ps_search_lookahead(ngs);
    

    for (bp = ngs->bp_table_idx[frame_idx]; bp < ngs->bpidx; bp++) {
        bpe = &(ngs->bp_table[bp]);
        ngs->word_lat_idx[bpe->wid] = NO_BP;

        if (bpe->wid == ps_search_finish_wid(ngs))
            continue;
        k++;

        
        


        if (bpe->last2_phone == -1) { 
            
            for (rc = 0; rc < bin_mdef_n_ciphone(ps_search_acmod(ngs)->mdef); ++rc) {
                if (bpe->score BETTER_THAN ngs->bestbp_rc[rc].score) {
                    E_DEBUG(4,("bestbp_rc[0] = %d lc %d\n",
                               bpe->score, bpe->last_phone));
                    ngs->bestbp_rc[rc].score = bpe->score;
                    ngs->bestbp_rc[rc].path = bp;
                    ngs->bestbp_rc[rc].lc = bpe->last_phone;
                }
            }
        }
        else {
            xwdssid_t *rssid = dict2pid_rssid(d2p, bpe->last_phone, bpe->last2_phone);
            int32 *rcss = &(ngs->bscore_stack[bpe->s_idx]);
            for (rc = 0; rc < bin_mdef_n_ciphone(ps_search_acmod(ngs)->mdef); ++rc) {
                if (rcss[rssid->cimap[rc]] BETTER_THAN ngs->bestbp_rc[rc].score) {
                    E_DEBUG(4,("bestbp_rc[%d] = %d lc %d\n",
                               rc, rcss[rssid->cimap[rc]], bpe->last_phone));
                    ngs->bestbp_rc[rc].score = rcss[rssid->cimap[rc]];
                    ngs->bestbp_rc[rc].path = bp;
                    ngs->bestbp_rc[rc].lc = bpe->last_phone;
                }
            }
        }
    }
    if (k == 0)
        return;

    nf = frame_idx + 1;
    thresh = ngs->best_score + ngs->dynamic_beam;
    



    for (i = ngs->n_root_chan, rhmm = ngs->root_chan; i > 0; --i, rhmm++) {
        bestbp_rc_ptr = &(ngs->bestbp_rc[rhmm->ciphone]);

        newscore = bestbp_rc_ptr->score + ngs->nwpen + ngs->pip;
        pl_newscore = newscore
            + phone_loop_search_score(pls, rhmm->ciphone);
        if (pl_newscore BETTER_THAN thresh) {
            if ((hmm_frame(&rhmm->hmm) < frame_idx)
                || (newscore BETTER_THAN hmm_in_score(&rhmm->hmm))) {
                hmm_enter(&rhmm->hmm, newscore,
                          bestbp_rc_ptr->path, nf);
                
                
                hmm_mpx_ssid(&rhmm->hmm, 0) =
                    dict2pid_ldiph_lc(d2p, rhmm->ciphone, rhmm->ci2phone, bestbp_rc_ptr->lc);
                assert(hmm_mpx_ssid(&rhmm->hmm, 0) != BAD_SSID);
            }
        }
    }

    



    for (i = 0; i < ngs->n_1ph_LMwords; i++) {
        w = ngs->single_phone_wid[i];
        ngs->last_ltrans[w].dscr = (int32) 0x80000000;
    }
    for (bp = ngs->bp_table_idx[frame_idx]; bp < ngs->bpidx; bp++) {
        bpe = &(ngs->bp_table[bp]);
        if (!bpe->valid)
            continue;

        for (i = 0; i < ngs->n_1ph_LMwords; i++) {
            int32 n_used;
            w = ngs->single_phone_wid[i];
            newscore = ngram_search_exit_score
                (ngs, bpe, dict_first_phone(dict, w));
            E_DEBUG(4, ("initial newscore for %s: %d\n",
                        dict_wordstr(dict, w), newscore));
            if (newscore != WORST_SCORE)
                newscore += ngram_tg_score(ngs->lmset,
                                           dict_basewid(dict, w),
                                           bpe->real_wid,
                                           bpe->prev_real_wid,
                                           &n_used)>>SENSCR_SHIFT;

            

            if (newscore BETTER_THAN ngs->last_ltrans[w].dscr) {
                ngs->last_ltrans[w].dscr = newscore;
                ngs->last_ltrans[w].bp = bp;
            }
        }
    }

    
    for (i = 0; i < ngs->n_1ph_LMwords; i++) {
        w = ngs->single_phone_wid[i];
        

        if (w == dict_startwid(ps_search_dict(ngs)))
            continue;
        rhmm = (root_chan_t *) ngs->word_chan[w];
        newscore = ngs->last_ltrans[w].dscr + ngs->pip;
	pl_newscore = newscore + phone_loop_search_score(pls, rhmm->ciphone);
        if (pl_newscore BETTER_THAN thresh) {
            bpe = ngs->bp_table + ngs->last_ltrans[w].bp;
            if ((hmm_frame(&rhmm->hmm) < frame_idx)
                || (newscore BETTER_THAN hmm_in_score(&rhmm->hmm))) {
                hmm_enter(&rhmm->hmm,
                          newscore, ngs->last_ltrans[w].bp, nf);
                
                
                hmm_mpx_ssid(&rhmm->hmm, 0) =
                    dict2pid_ldiph_lc(d2p, rhmm->ciphone, rhmm->ci2phone,
                                      dict_last_phone(dict, bpe->wid));
                assert(hmm_mpx_ssid(&rhmm->hmm, 0) != BAD_SSID);
            }
        }
    }

    
    w = ps_search_silence_wid(ngs);
    rhmm = (root_chan_t *) ngs->word_chan[w];
    bestbp_rc_ptr = &(ngs->bestbp_rc[ps_search_acmod(ngs)->mdef->sil]);
    newscore = bestbp_rc_ptr->score + ngs->silpen + ngs->pip;
    pl_newscore = newscore
        + phone_loop_search_score(pls, rhmm->ciphone);
    if (pl_newscore BETTER_THAN thresh) {
        if ((hmm_frame(&rhmm->hmm) < frame_idx)
            || (newscore BETTER_THAN hmm_in_score(&rhmm->hmm))) {
            hmm_enter(&rhmm->hmm,
                      newscore, bestbp_rc_ptr->path, nf);
        }
    }
    for (w = dict_filler_start(dict); w <= dict_filler_end(dict); w++) {
        if (w == ps_search_silence_wid(ngs))
            continue;
        

        if (w == dict_startwid(ps_search_dict(ngs)))
            continue;
        rhmm = (root_chan_t *) ngs->word_chan[w];
        
        if (rhmm == NULL)
            continue;
        newscore = bestbp_rc_ptr->score + ngs->fillpen + ngs->pip;
        pl_newscore = newscore
            + phone_loop_search_score(pls, rhmm->ciphone);
        if (pl_newscore BETTER_THAN thresh) {
            if ((hmm_frame(&rhmm->hmm) < frame_idx)
                || (newscore BETTER_THAN hmm_in_score(&rhmm->hmm))) {
                hmm_enter(&rhmm->hmm,
                          newscore, bestbp_rc_ptr->path, nf);
            }
        }
    }
}

static void
deactivate_channels(ngram_search_t *ngs, int frame_idx)
{
    root_chan_t *rhmm;
    int i;

    
    for (i = ngs->n_root_chan, rhmm = ngs->root_chan; i > 0; --i, rhmm++) {
        if (hmm_frame(&rhmm->hmm) == frame_idx) {
            hmm_clear(&rhmm->hmm);
        }
    }
    
    for (i = 0; i < ngs->n_1ph_words; i++) {
        int32 w = ngs->single_phone_wid[i];
        rhmm = (root_chan_t *) ngs->word_chan[w];
        if (hmm_frame(&rhmm->hmm) == frame_idx) {
            hmm_clear(&rhmm->hmm);
        }
    }
}

int
ngram_fwdtree_search(ngram_search_t *ngs, int frame_idx)
{
    int16 const *senscr;

    
    if (!ps_search_acmod(ngs)->compallsen)
        compute_sen_active(ngs, frame_idx);

    
    if ((senscr = acmod_score(ps_search_acmod(ngs), &frame_idx)) == NULL)
        return 0;
    ngs->st.n_senone_active_utt += ps_search_acmod(ngs)->n_senone_active;

    
    ngram_search_mark_bptable(ngs, frame_idx);

    

    if (ngs->best_score == WORST_SCORE || ngs->best_score WORSE_THAN WORST_SCORE)
        return 0;
    
    if (ngs->best_score + (2 * ngs->beam) WORSE_THAN WORST_SCORE) {
        E_INFO("Renormalizing Scores at frame %d, best score %d\n",
               frame_idx, ngs->best_score);
        renormalize_scores(ngs, frame_idx, ngs->best_score);
    }

    
    evaluate_channels(ngs, senscr, frame_idx);
    
    prune_channels(ngs, frame_idx);
    
    bptable_maxwpf(ngs, frame_idx);
    
    word_transition(ngs, frame_idx);
    
    deactivate_channels(ngs, frame_idx);

    ++ngs->n_frame;
    
    return 1;
}

void
ngram_fwdtree_finish(ngram_search_t *ngs)
{
    int32 i, w, cf, *awl;
    root_chan_t *rhmm;
    chan_t *hmm, **acl;

    
    cf = ps_search_acmod(ngs)->output_frame;
    
    ngram_search_mark_bptable(ngs, cf);

    
    
    for (i = ngs->n_root_chan, rhmm = ngs->root_chan; i > 0; --i, rhmm++) {
        hmm_clear(&rhmm->hmm);
    }

    
    i = ngs->n_active_chan[cf & 0x1];
    acl = ngs->active_chan_list[cf & 0x1];
    for (hmm = *(acl++); i > 0; --i, hmm = *(acl++)) {
        hmm_clear(&hmm->hmm);
    }

    
    i = ngs->n_active_word[cf & 0x1];
    awl = ngs->active_word_list[cf & 0x1];
    for (w = *(awl++); i > 0; --i, w = *(awl++)) {
        
        if (dict_is_single_phone(ps_search_dict(ngs), w))
            continue;
        bitvec_clear(ngs->word_active, w);
        if (ngs->word_chan[w] == NULL)
            continue;
        ngram_search_free_all_rc(ngs, w);
    }

    







    ptmr_stop(&ngs->fwdtree_perf);
    
    if (cf > 0) {
        double n_speech = (double)(cf + 1)
            / cmd_ln_int32_r(ps_search_config(ngs), "-frate");
        E_INFO("%8d words recognized (%d/fr)\n",
               ngs->bpidx, (ngs->bpidx + (cf >> 1)) / (cf + 1));
        E_INFO("%8d senones evaluated (%d/fr)\n", ngs->st.n_senone_active_utt,
               (ngs->st.n_senone_active_utt + (cf >> 1)) / (cf + 1));
        E_INFO("%8d channels searched (%d/fr), %d 1st, %d last\n",
               ngs->st.n_root_chan_eval + ngs->st.n_nonroot_chan_eval,
               (ngs->st.n_root_chan_eval + ngs->st.n_nonroot_chan_eval) / (cf + 1),
               ngs->st.n_root_chan_eval, ngs->st.n_last_chan_eval);
        E_INFO("%8d words for which last channels evaluated (%d/fr)\n",
               ngs->st.n_word_lastchan_eval,
               ngs->st.n_word_lastchan_eval / (cf + 1));
        E_INFO("%8d candidate words for entering last phone (%d/fr)\n",
               ngs->st.n_lastphn_cand_utt, ngs->st.n_lastphn_cand_utt / (cf + 1));
        E_INFO("fwdtree %.2f CPU %.3f xRT\n",
               ngs->fwdtree_perf.t_cpu,
               ngs->fwdtree_perf.t_cpu / n_speech);
        E_INFO("fwdtree %.2f wall %.3f xRT\n",
               ngs->fwdtree_perf.t_elapsed,
               ngs->fwdtree_perf.t_elapsed / n_speech);
    }
    
}
