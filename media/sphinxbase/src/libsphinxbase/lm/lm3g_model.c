









































#include <string.h>
#include <assert.h>
#include <limits.h>

#include "sphinxbase/listelem_alloc.h"
#include "sphinxbase/ckd_alloc.h"
#include "sphinxbase/err.h"

#include "lm3g_model.h"

void
lm3g_tginfo_free(ngram_model_t *base, lm3g_model_t *lm3g)
{
	if (lm3g->tginfo == NULL)
		return;
        listelem_alloc_free(lm3g->le);
        ckd_free(lm3g->tginfo);
}

void
lm3g_tginfo_reset(ngram_model_t *base, lm3g_model_t *lm3g)
{
    if (lm3g->tginfo == NULL)
        return;
    listelem_alloc_free(lm3g->le);
    memset(lm3g->tginfo, 0, base->n_counts[0] * sizeof(tginfo_t *));
    lm3g->le = listelem_alloc_init(sizeof(tginfo_t));
}

void
lm3g_apply_weights(ngram_model_t *base,
		   lm3g_model_t *lm3g,
		   float32 lw, float32 wip, float32 uw)
{
    int32 log_wip, log_uw, log_uniform_weight;
    int i;

    
    log_wip = logmath_log(base->lmath, wip);
    log_uw = logmath_log(base->lmath, uw);
    log_uniform_weight = logmath_log(base->lmath, 1.0 - uw);

    for (i = 0; i < base->n_counts[0]; ++i) {
        int32 prob1, bo_wt, n_used;

        
        bo_wt = (int32)(lm3g->unigrams[i].bo_wt1.l / base->lw);
        

        prob1 = ngram_ng_prob(base, i, NULL, 0, &n_used);
        
        lm3g->unigrams[i].bo_wt1.l = (int32)(bo_wt * lw);
        if (strcmp(base->word_str[i], "<s>") == 0) { 
            
            lm3g->unigrams[i].prob1.l = (int32)(prob1 * lw) + log_wip;
        }
        else {
            
            prob1 += log_uw;
            prob1 = logmath_add(base->lmath, prob1, base->log_uniform + log_uniform_weight);
            
            lm3g->unigrams[i].prob1.l = (int32)(prob1 * lw) + log_wip;
        }
    }

    for (i = 0; i < lm3g->n_prob2; ++i) {
        int32 prob2;
        
        prob2 = (int32)((lm3g->prob2[i].l - base->log_wip) / base->lw);
        lm3g->prob2[i].l = (int32)(prob2 * lw) + log_wip;
    }

    if (base->n > 2) {
        for (i = 0; i < lm3g->n_bo_wt2; ++i) {
            lm3g->bo_wt2[i].l = (int32)(lm3g->bo_wt2[i].l  / base->lw * lw);
        }
        for (i = 0; i < lm3g->n_prob3; i++) {
            int32 prob3;
            
            prob3 = (int32)((lm3g->prob3[i].l - base->log_wip) / base->lw);
            lm3g->prob3[i].l = (int32)(prob3 * lw) + log_wip;
        }
    }

    
    base->log_wip = log_wip;
    base->log_uw = log_uw;
    base->log_uniform_weight = log_uniform_weight;
    base->lw = lw;
}

int32
lm3g_add_ug(ngram_model_t *base,
            lm3g_model_t *lm3g, int32 wid, int32 lweight)
{
    int32 score;

    
    assert(!NGRAM_IS_CLASSWID(wid));

    
    lm3g->unigrams = ckd_realloc(lm3g->unigrams,
                                 sizeof(*lm3g->unigrams) * base->n_1g_alloc);
    memset(lm3g->unigrams + base->n_counts[0], 0,
           (base->n_1g_alloc - base->n_counts[0]) * sizeof(*lm3g->unigrams));
    
    lm3g->tginfo = ckd_realloc(lm3g->tginfo,
                               sizeof(*lm3g->tginfo) * base->n_1g_alloc);
    memset(lm3g->tginfo + base->n_counts[0], 0,
           (base->n_1g_alloc - base->n_counts[0]) * sizeof(*lm3g->tginfo));
    



    
    score = lweight + base->log_uniform + base->log_uw;
    score = logmath_add(base->lmath, score,
                        base->log_uniform + base->log_uniform_weight);
    lm3g->unigrams[wid].prob1.l = score;
    

    lm3g->unigrams[wid].bo_wt1.l = 0;
    lm3g->unigrams[wid].bigrams = 0;
    
    ++base->n_counts[0];
    



    if (wid >= base->n_counts[0])
        base->n_counts[0] = wid + 1;

    return score;
}

#define INITIAL_SORTED_ENTRIES	MAX_UINT16

void
init_sorted_list(sorted_list_t * l)
{
    l->list = ckd_calloc(INITIAL_SORTED_ENTRIES, sizeof(sorted_entry_t));
    l->list[0].val.l = INT_MIN;
    l->list[0].lower = 0;
    l->list[0].higher = 0;
    l->free = 1;
    l->size = INITIAL_SORTED_ENTRIES;
}

void
free_sorted_list(sorted_list_t * l)
{
    free(l->list);
}

lmprob_t *
vals_in_sorted_list(sorted_list_t * l)
{
    lmprob_t *vals;
    int32 i;

    vals = ckd_calloc(l->free, sizeof(lmprob_t));
    for (i = 0; i < l->free; i++)
        vals[i] = l->list[i].val;
    return (vals);
}

int32
sorted_id(sorted_list_t * l, int32 *val)
{
    int32 i = 0;

    for (;;) {
        if (*val == l->list[i].val.l)
            return (i);
        if (*val < l->list[i].val.l) {
            if (l->list[i].lower == 0) {

                if (l->free >= l->size) {
            	    int newsize = l->size + INITIAL_SORTED_ENTRIES;
            	    l->list = ckd_realloc(l->list, sizeof(sorted_entry_t) * newsize);
            	    memset(l->list + l->size, 
            	           0, INITIAL_SORTED_ENTRIES * sizeof(sorted_entry_t));
            	    l->size = newsize;
                }

                l->list[i].lower = l->free;
                (l->free)++;
                i = l->list[i].lower;
                l->list[i].val.l = *val;
                return (i);
            }
            else
                i = l->list[i].lower;
        }
        else {
            if (l->list[i].higher == 0) {

                if (l->free >= l->size) {
            	    int newsize = l->size + INITIAL_SORTED_ENTRIES;
            	    l->list = ckd_realloc(l->list, sizeof(sorted_entry_t) * newsize);
            	    memset(l->list + l->size, 
            	           0, INITIAL_SORTED_ENTRIES * sizeof(sorted_entry_t));
            	    l->size = newsize;
                }

                l->list[i].higher = l->free;
                (l->free)++;
                i = l->list[i].higher;
                l->list[i].val.l = *val;
                return (i);
            }
            else
                i = l->list[i].higher;
        }
    }
}
