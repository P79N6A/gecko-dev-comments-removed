








































#include <assert.h>


#define BINARY_SEARCH_THRESH	16
static int32
find_bg(bigram_t * bg, int32 n, int32 w)
{
    int32 i, b, e;

    
    b = 0;
    e = n;
    while (e - b > BINARY_SEARCH_THRESH) {
        i = (b + e) >> 1;
        if (bg[i].wid < w)
            b = i + 1;
        else if (bg[i].wid > w)
            e = i;
        else
            return i;
    }

    
    for (i = b; (i < e) && (bg[i].wid != w); i++);
    return ((i < e) ? i : -1);
}

static int32
lm3g_bg_score(NGRAM_MODEL_TYPE *model,
              int32 lw1, int32 lw2, int32 *n_used)
{
    int32 i, n, b, score;
    bigram_t *bg;

    if (lw1 < 0 || model->base.n < 2) {
        *n_used = 1;
        return model->lm3g.unigrams[lw2].prob1.l;
    }

    b = FIRST_BG(model, lw1);
    n = FIRST_BG(model, lw1 + 1) - b;
    bg = model->lm3g.bigrams + b;

    if ((i = find_bg(bg, n, lw2)) >= 0) {
        
        *n_used = 2;
        score = model->lm3g.prob2[bg[i].prob2].l;
    }
    else {
        
        *n_used = 1;
        score = model->lm3g.unigrams[lw1].bo_wt1.l + model->lm3g.unigrams[lw2].prob1.l;
    }

    return (score);
}

static void
load_tginfo(NGRAM_MODEL_TYPE *model, int32 lw1, int32 lw2)
{
    int32 i, n, b, t;
    bigram_t *bg;
    tginfo_t *tginfo;

    
    tginfo = (tginfo_t *) listelem_malloc(model->lm3g.le);
    tginfo->w1 = lw1;
    tginfo->tg = NULL;
    tginfo->next = model->lm3g.tginfo[lw2];
    model->lm3g.tginfo[lw2] = tginfo;

    
    b = model->lm3g.unigrams[lw1].bigrams;
    n = model->lm3g.unigrams[lw1 + 1].bigrams - b;
    bg = model->lm3g.bigrams + b;

    if ((n > 0) && ((i = find_bg(bg, n, lw2)) >= 0)) {
        tginfo->bowt = model->lm3g.bo_wt2[bg[i].bo_wt2].l;

        
        b += i;                 
        t = FIRST_TG(model, b);

        tginfo->tg = model->lm3g.trigrams + t;

        
        tginfo->n_tg = FIRST_TG(model, b + 1) - t;
    }
    else {                      
        tginfo->bowt = 0;
        tginfo->n_tg = 0;
    }
}


static int32
find_tg(trigram_t * tg, int32 n, uint32 w)
{
    int32 i, b, e;

    b = 0;
    e = n;
    while (e - b > BINARY_SEARCH_THRESH) {
        i = (b + e) >> 1;
        if (tg[i].wid < w)
            b = i + 1;
        else if (tg[i].wid > w)
            e = i;
        else
            return i;
    }

    for (i = b; (i < e) && (tg[i].wid != w); i++);
    return ((i < e) ? i : -1);
}

static int32
lm3g_tg_score(NGRAM_MODEL_TYPE *model, int32 lw1,
              int32 lw2, int32 lw3, int32 *n_used)
{
    ngram_model_t *base = &model->base;
    int32 i, n, score;
    trigram_t *tg;
    tginfo_t *tginfo, *prev_tginfo;

    if ((base->n < 3) || (lw1 < 0) || (lw2 < 0))
        return (lm3g_bg_score(model, lw2, lw3, n_used));

    prev_tginfo = NULL;
    for (tginfo = model->lm3g.tginfo[lw2]; tginfo; tginfo = tginfo->next) {
        if (tginfo->w1 == lw1)
            break;
        prev_tginfo = tginfo;
    }

    if (!tginfo) {
        load_tginfo(model, lw1, lw2);
        tginfo = model->lm3g.tginfo[lw2];
    }
    else if (prev_tginfo) {
        prev_tginfo->next = tginfo->next;
        tginfo->next = model->lm3g.tginfo[lw2];
        model->lm3g.tginfo[lw2] = tginfo;
    }

    tginfo->used = 1;

    
    n = tginfo->n_tg;
    tg = tginfo->tg;
    if ((i = find_tg(tg, n, lw3)) >= 0) {
        
        *n_used = 3;
        score = model->lm3g.prob3[tg[i].prob3].l;
    }
    else {
        score = tginfo->bowt + lm3g_bg_score(model, lw2, lw3, n_used);
    }

    return (score);
}

static int32
lm3g_template_score(ngram_model_t *base, int32 wid,
                      int32 *history, int32 n_hist,
                      int32 *n_used)
{
    NGRAM_MODEL_TYPE *model = (NGRAM_MODEL_TYPE *)base;
    switch (n_hist) {
    case 0:
        
        *n_used = 1;
        return model->lm3g.unigrams[wid].prob1.l;
    case 1:
        return lm3g_bg_score(model, history[0], wid, n_used);
    case 2:
    default:
        
        return lm3g_tg_score(model, history[1], history[0], wid, n_used);
    }
}

static int32
lm3g_template_raw_score(ngram_model_t *base, int32 wid,
                        int32 *history, int32 n_hist,
                          int32 *n_used)
{
    NGRAM_MODEL_TYPE *model = (NGRAM_MODEL_TYPE *)base;
    int32 score;

    switch (n_hist) {
    case 0:
        
        *n_used = 1;
        
        score = model->lm3g.unigrams[wid].prob1.l - base->log_wip;
        
        score = (int32)(score / base->lw);
        
        if (strcmp(base->word_str[wid], "<s>") != 0) { 
            

            if (base->log_uniform + base->log_uniform_weight > logmath_get_zero(base->lmath)) {
               score = logmath_log(base->lmath,
                            logmath_exp(base->lmath, score)
                            - logmath_exp(base->lmath, 
                                          base->log_uniform + base->log_uniform_weight));
            }
        }
        return score;
    case 1:
        score = lm3g_bg_score(model, history[0], wid, n_used);
        break;
    case 2:
    default:
        
        score = lm3g_tg_score(model, history[1], history[0], wid, n_used);
        break;
    }
    
    return (int32)((score - base->log_wip) / base->lw);
}

static int32
lm3g_template_add_ug(ngram_model_t *base,
                       int32 wid, int32 lweight)
{
    NGRAM_MODEL_TYPE *model = (NGRAM_MODEL_TYPE *)base;
    return lm3g_add_ug(base, &model->lm3g, wid, lweight);
}

static void
lm3g_template_flush(ngram_model_t *base)
{
    NGRAM_MODEL_TYPE *model = (NGRAM_MODEL_TYPE *)base;
    lm3g_tginfo_reset(base, &model->lm3g);
}

typedef struct lm3g_iter_s {
    ngram_iter_t base;
    unigram_t *ug;
    bigram_t *bg;
    trigram_t *tg;
} lm3g_iter_t;

static ngram_iter_t *
lm3g_template_iter(ngram_model_t *base, int32 wid,
                   int32 *history, int32 n_hist)
{
    NGRAM_MODEL_TYPE *model = (NGRAM_MODEL_TYPE *)base;
    lm3g_iter_t *itor = (lm3g_iter_t *)ckd_calloc(1, sizeof(*itor));

    ngram_iter_init((ngram_iter_t *)itor, base, n_hist, FALSE);

    if (n_hist == 0) {
        
        itor->ug = model->lm3g.unigrams + wid;
        return (ngram_iter_t *)itor;
    }
    else if (n_hist == 1) {
        int32 i, n, b;
        
        itor->ug = model->lm3g.unigrams + history[0];
        b = FIRST_BG(model, history[0]);
        n = FIRST_BG(model, history[0] + 1) - b;
        itor->bg = model->lm3g.bigrams + b;
        
        if ((i = find_bg(itor->bg, n, wid)) < 0) {
            ngram_iter_free((ngram_iter_t *)itor);
            return NULL;
        }
        itor->bg += i;
        return (ngram_iter_t *)itor;
    }
    else if (n_hist == 2) {
        int32 i, n;
        tginfo_t *tginfo, *prev_tginfo;
        
        itor->ug = model->lm3g.unigrams + history[1];
        prev_tginfo = NULL;
        for (tginfo = model->lm3g.tginfo[history[0]];
             tginfo; tginfo = tginfo->next) {
            if (tginfo->w1 == history[1])
                break;
            prev_tginfo = tginfo;
        }

        if (!tginfo) {
            load_tginfo(model, history[1], history[0]);
            tginfo = model->lm3g.tginfo[history[0]];
        }
        else if (prev_tginfo) {
            prev_tginfo->next = tginfo->next;
            tginfo->next = model->lm3g.tginfo[history[0]];
            model->lm3g.tginfo[history[0]] = tginfo;
        }

        tginfo->used = 1;

        
        n = tginfo->n_tg;
        itor->tg = tginfo->tg;
        if ((i = find_tg(itor->tg, n, wid)) >= 0) {
            itor->tg += i;
            


            itor->bg = model->lm3g.bigrams;
            while (FIRST_TG(model, (itor->bg - model->lm3g.bigrams + 1))
                   <= (itor->tg - model->lm3g.trigrams))
                ++itor->bg;
            return (ngram_iter_t *)itor;
        }
        else {
            ngram_iter_free((ngram_iter_t *)itor);
            return (ngram_iter_t *)NULL;
        }
    }
    else {
        
        assert(n_hist == 0); 
        ngram_iter_free((ngram_iter_t *)itor);
        return NULL;
    }
}

static ngram_iter_t *
lm3g_template_mgrams(ngram_model_t *base, int m)
{
    NGRAM_MODEL_TYPE *model = (NGRAM_MODEL_TYPE *)base;
    lm3g_iter_t *itor = (lm3g_iter_t *)ckd_calloc(1, sizeof(*itor));
    ngram_iter_init((ngram_iter_t *)itor, base, m, FALSE);

    itor->ug = model->lm3g.unigrams;
    itor->bg = model->lm3g.bigrams;
    itor->tg = model->lm3g.trigrams;

    
    if (m > 1 && base->n_counts[1] > 1)  {
        while (FIRST_TG(model, (itor->bg - model->lm3g.bigrams + 1))
               <= (itor->tg - model->lm3g.trigrams))
            ++itor->bg;
    }

    
    if (m > 0 && base->n_counts[0] > 1) {
        while (itor->ug[1].bigrams <= (itor->bg - model->lm3g.bigrams))
            ++itor->ug;
    }

    return (ngram_iter_t *)itor;
}

static ngram_iter_t *
lm3g_template_successors(ngram_iter_t *bitor)
{
    NGRAM_MODEL_TYPE *model = (NGRAM_MODEL_TYPE *)bitor->model;
    lm3g_iter_t *from = (lm3g_iter_t *)bitor;
    lm3g_iter_t *itor = (lm3g_iter_t *)ckd_calloc(1, sizeof(*itor));

    itor->ug = from->ug;
    switch (bitor->m) {
    case 0:
        

        if (((itor->ug + 1) - model->lm3g.unigrams < bitor->model->n_counts[0] &&
	    itor->ug->bigrams == (itor->ug + 1)->bigrams) || 
	    itor->ug->bigrams == bitor->model->n_counts[1])
	    goto done;
	    
        
        itor->bg = model->lm3g.bigrams + itor->ug->bigrams;
        break;
    case 1:
        itor->bg = from->bg;
        
        
        if (((itor->bg + 1) - model->lm3g.bigrams < bitor->model->n_counts[1] &&
    	    FIRST_TG (model, itor->bg - model->lm3g.bigrams) == 
    	    FIRST_TG (model, (itor->bg + 1) - model->lm3g.bigrams)) ||
	    FIRST_TG (model, itor->bg - model->lm3g.bigrams) == bitor->model->n_counts[2])
    	    goto done;
    	    
        
        itor->tg = (model->lm3g.trigrams 
                    + FIRST_TG(model, (itor->bg - model->lm3g.bigrams)));
#if 0
        printf("%s %s => %d (%s)\n",
               model->base.word_str[itor->ug - model->lm3g.unigrams],
               model->base.word_str[itor->bg->wid],
               FIRST_TG(model, (itor->bg - model->lm3g.bigrams)),
               model->base.word_str[itor->tg->wid]);
#endif
        break;
    case 2:
    default:
        
        goto done;
    }

    ngram_iter_init((ngram_iter_t *)itor, bitor->model, bitor->m + 1, TRUE);
    return (ngram_iter_t *)itor;
    done:
        ckd_free(itor);
        return NULL;
}

static int32 const *
lm3g_template_iter_get(ngram_iter_t *base,
                       int32 *out_score, int32 *out_bowt)
{
    NGRAM_MODEL_TYPE *model = (NGRAM_MODEL_TYPE *)base->model;
    lm3g_iter_t *itor = (lm3g_iter_t *)base;

    base->wids[0] = itor->ug - model->lm3g.unigrams;
    if (itor->bg) base->wids[1] = itor->bg->wid;
    if (itor->tg) base->wids[2] = itor->tg->wid;
#if 0
    printf("itor_get: %d %d %d\n", base->wids[0], base->wids[1], base->wids[2]);
#endif

    switch (base->m) {
    case 0:
        *out_score = itor->ug->prob1.l;
        *out_bowt = itor->ug->bo_wt1.l;
        break;
    case 1:
        *out_score = model->lm3g.prob2[itor->bg->prob2].l;
        if (model->lm3g.bo_wt2)
            *out_bowt = model->lm3g.bo_wt2[itor->bg->bo_wt2].l;
        else
            *out_bowt = 0;
        break;
    case 2:
        *out_score = model->lm3g.prob3[itor->tg->prob3].l;
        *out_bowt = 0;
        break;
    default: 
        return NULL;
    }
    return base->wids;
}

static ngram_iter_t *
lm3g_template_iter_next(ngram_iter_t *base)
{
    NGRAM_MODEL_TYPE *model = (NGRAM_MODEL_TYPE *)base->model;
    lm3g_iter_t *itor = (lm3g_iter_t *)base;

    switch (base->m) {
    case 0:
        ++itor->ug;
        
        if (itor->ug - model->lm3g.unigrams >= base->model->n_counts[0])
            goto done;
        break;
    case 1:
        ++itor->bg;
        
        if (itor->bg - model->lm3g.bigrams >= base->model->n_counts[1])
            goto done;
        

        while (itor->bg - model->lm3g.bigrams >= itor->ug[1].bigrams) {
            

            if (base->successor)
                goto done;
            ++itor->ug;
            if (itor->ug == model->lm3g.unigrams + base->model->n_counts[0]) {
                E_ERROR("Bigram %d has no valid unigram parent\n",
                        itor->bg - model->lm3g.bigrams);
                goto done;
            }
        }
        break;
    case 2:
        ++itor->tg;
        
        if (itor->tg - model->lm3g.trigrams >= base->model->n_counts[2])
            goto done;
        
        while (itor->tg - model->lm3g.trigrams >=
            FIRST_TG(model, (itor->bg - model->lm3g.bigrams + 1))) {
            if (base->successor)
                goto done;
            ++itor->bg;
            if (itor->bg == model->lm3g.bigrams + base->model->n_counts[1]) {
                E_ERROR("Trigram %d has no valid bigram parent\n",
                        itor->tg - model->lm3g.trigrams);

               goto done;
            }
        }
        
        while (itor->bg - model->lm3g.bigrams >= itor->ug[1].bigrams) {
            ++itor->ug;
            if (itor->ug == model->lm3g.unigrams + base->model->n_counts[0]) {
                E_ERROR("Trigram %d has no valid unigram parent\n",
                        itor->tg - model->lm3g.trigrams);
                goto done;
            }
        }
        break;
    default: 
        goto done;
    }

    return (ngram_iter_t *)itor;
done:
    ngram_iter_free(base);
    return NULL;
}

static void
lm3g_template_iter_free(ngram_iter_t *base)
{
    ckd_free(base);
}
