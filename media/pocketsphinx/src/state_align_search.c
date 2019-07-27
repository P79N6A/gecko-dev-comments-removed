








































#include "state_align_search.h"

static int
state_align_search_start(ps_search_t *search)
{
    state_align_search_t *sas = (state_align_search_t *)search;

    
    hmm_enter(sas->hmms, 0, 0, 0);

    return 0;
}

static void
renormalize_hmms(state_align_search_t *sas, int frame_idx, int32 norm)
{
    int i;
    for (i = 0; i < sas->n_phones; ++i)
        hmm_normalize(sas->hmms + i, norm);
}

static int32
evaluate_hmms(state_align_search_t *sas, int16 const *senscr, int frame_idx)
{
    int32 bs = WORST_SCORE;
    int i;

    hmm_context_set_senscore(sas->hmmctx, senscr);

    for (i = 0; i < sas->n_phones; ++i) {
        hmm_t *hmm = sas->hmms + i;
        int32 score;

        if (hmm_frame(hmm) < frame_idx)
            continue;
        score = hmm_vit_eval(hmm);
        if (score BETTER_THAN bs) {
            bs = score;
        }
    }
    return bs;
}

static void
prune_hmms(state_align_search_t *sas, int frame_idx)
{
    int nf = frame_idx + 1;
    int i;

    
    for (i = 0; i < sas->n_phones; ++i) {
        hmm_t *hmm = sas->hmms + i;
        if (hmm_frame(hmm) < frame_idx)
            continue;
        hmm_frame(hmm) = nf;
    }
}

static void
phone_transition(state_align_search_t *sas, int frame_idx)
{
    int nf = frame_idx + 1;
    int i;

    for (i = 0; i < sas->n_phones - 1; ++i) {
        hmm_t *hmm, *nhmm;
        int32 newphone_score;

        hmm = sas->hmms + i;
        if (hmm_frame(hmm) != nf)
            continue;

        newphone_score = hmm_out_score(hmm);
        
        nhmm = hmm + 1;
        if (hmm_frame(nhmm) < frame_idx
            || newphone_score BETTER_THAN hmm_in_score(nhmm)) {
            hmm_enter(nhmm, newphone_score, hmm_out_history(hmm), nf);
        }
    }
}

#define TOKEN_STEP 20
static void
extend_tokenstack(state_align_search_t *sas, int frame_idx)
{
    if (frame_idx >= sas->n_fr_alloc) {
        sas->n_fr_alloc = frame_idx + TOKEN_STEP + 1;
        sas->tokens = ckd_realloc(sas->tokens,
                                  sas->n_emit_state * sas->n_fr_alloc
                                  * sizeof(*sas->tokens));
    }
    memset(sas->tokens + frame_idx * sas->n_emit_state, 0xff,
           sas->n_emit_state * sizeof(*sas->tokens));
}

static void
record_transitions(state_align_search_t *sas, int frame_idx)
{
    uint16 *tokens;
    int i;

    
    extend_tokenstack(sas, frame_idx);
    tokens = sas->tokens + frame_idx * sas->n_emit_state;

    
    for (i = 0; i < sas->n_phones; ++i) {
        hmm_t *hmm = sas->hmms + i;
        int j;

        if (hmm_frame(hmm) < frame_idx)
            continue;
        for (j = 0; j < sas->hmmctx->n_emit_state; ++j) {
            int state_idx = i * sas->hmmctx->n_emit_state + j;
            
            tokens[state_idx] = hmm_history(hmm, j);
            
            hmm_history(hmm, j) = state_idx;
        }
    }
}

static int
state_align_search_step(ps_search_t *search, int frame_idx)
{
    state_align_search_t *sas = (state_align_search_t *)search;
    acmod_t *acmod = ps_search_acmod(search);
    int16 const *senscr;
    int i;

    
    for (i = 0; i < sas->n_phones; ++i)
        acmod_activate_hmm(acmod, sas->hmms + i);
    senscr = acmod_score(acmod, &frame_idx);

    
    
    if ((sas->best_score - 0x300000) WORSE_THAN WORST_SCORE) {
        E_INFO("Renormalizing Scores at frame %d, best score %d\n",
               frame_idx, sas->best_score);
        renormalize_hmms(sas, frame_idx, sas->best_score);
    }
    
    
    sas->best_score = evaluate_hmms(sas, senscr, frame_idx);
    prune_hmms(sas, frame_idx);

    
    phone_transition(sas, frame_idx);

    
    record_transitions(sas, frame_idx);

    
    sas->frame = frame_idx;

    return 0;
}

static int
state_align_search_finish(ps_search_t *search)
{
    state_align_search_t *sas = (state_align_search_t *)search;
    hmm_t *final_phone = sas->hmms + sas->n_phones - 1;
    ps_alignment_iter_t *itor;
    ps_alignment_entry_t *ent;
    int next_state, next_start, state, frame;

    
    next_state = state = hmm_out_history(final_phone);
    if (state == 0xffff) {
        E_ERROR("Failed to reach final state in alignment\n");
        return -1;
    }
    itor = ps_alignment_states(sas->al);
    next_start = sas->frame + 1;
    for (frame = sas->frame - 1; frame >= 0; --frame) {
        state = sas->tokens[frame * sas->n_emit_state + state];
        
        if (state != next_state) {
            itor = ps_alignment_iter_goto(itor, next_state);
            assert(itor != NULL);
            ent = ps_alignment_iter_get(itor);
            ent->start = frame + 1;
            ent->duration = next_start - ent->start;
            E_DEBUG(1,("state %d start %d end %d\n", next_state,
                       ent->start, next_start));
            next_state = state;
            next_start = frame + 1;
        }
    }
    
    itor = ps_alignment_iter_goto(itor, 0);
    assert(itor != NULL);
    ent = ps_alignment_iter_get(itor);
    ent->start = 0;
    ent->duration = next_start;
    E_DEBUG(1,("state %d start %d end %d\n", 0,
               ent->start, next_start));
    ps_alignment_iter_free(itor);
    ps_alignment_propagate(sas->al);

    return 0;
}

static int
state_align_search_reinit(ps_search_t *search, dict_t *dict, dict2pid_t *d2p)
{
    
    return 0;
}

static void
state_align_search_free(ps_search_t *search)
{
    state_align_search_t *sas = (state_align_search_t *)search;
    ps_search_deinit(search);
    ckd_free(sas->hmms);
    ckd_free(sas->tokens);
    hmm_context_free(sas->hmmctx);
    ckd_free(sas);
}

static ps_searchfuncs_t state_align_search_funcs = {
       "state_align",
      state_align_search_start,
       state_align_search_step,
     state_align_search_finish,
     state_align_search_reinit,
       state_align_search_free,
      NULL,
          NULL,
         NULL,
     NULL,
};

ps_search_t *
state_align_search_init(cmd_ln_t *config,
                        acmod_t *acmod,
                        ps_alignment_t *al)
{
    state_align_search_t *sas;
    ps_alignment_iter_t *itor;
    hmm_t *hmm;

    sas = ckd_calloc(1, sizeof(*sas));
    ps_search_init(ps_search_base(sas), &state_align_search_funcs,
                   config, acmod, al->d2p->dict, al->d2p);
    sas->hmmctx = hmm_context_init(bin_mdef_n_emit_state(acmod->mdef),
                                   acmod->tmat->tp, NULL, acmod->mdef->sseq);
    if (sas->hmmctx == NULL) {
        ckd_free(sas);
        return NULL;
    }
    sas->al = al;

    
    sas->n_phones = ps_alignment_n_phones(al);
    sas->n_emit_state = ps_alignment_n_states(al);
    sas->hmms = ckd_calloc(sas->n_phones, sizeof(*sas->hmms));
    for (hmm = sas->hmms, itor = ps_alignment_phones(al); itor;
         ++hmm, itor = ps_alignment_iter_next(itor)) {
        ps_alignment_entry_t *ent = ps_alignment_iter_get(itor);
        hmm_init(sas->hmmctx, hmm, FALSE,
                 ent->id.pid.ssid, ent->id.pid.tmatid);
    }
    return ps_search_base(sas);
}
