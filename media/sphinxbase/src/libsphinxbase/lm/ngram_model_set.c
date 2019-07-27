








































#include <string.h>
#include <stdlib.h>

#include "sphinxbase/err.h"
#include "sphinxbase/ckd_alloc.h"
#include "sphinxbase/strfuncs.h"
#include "sphinxbase/filename.h"

#include "ngram_model_set.h"

static ngram_funcs_t ngram_model_set_funcs;

static int
my_compare(const void *a, const void *b)
{
    
    if (strcmp(*(char * const *)a, "<UNK>") == 0)
        return -1;
    else if (strcmp(*(char * const *)b, "<UNK>") == 0)
        return 1;
    else
        return strcmp(*(char * const *)a, *(char * const *)b);
}

static void
build_widmap(ngram_model_t *base, logmath_t *lmath, int32 n)
{
    ngram_model_set_t *set = (ngram_model_set_t *)base;
    ngram_model_t **models = set->lms;
    hash_table_t *vocab;
    glist_t hlist;
    gnode_t *gn;
    int32 i;

    
    vocab = hash_table_new(models[0]->n_words, FALSE);
    
    for (i = 0; i < set->n_models; ++i) {
        int32 j;
        for (j = 0; j < models[i]->n_words; ++j) {
            
            (void)hash_table_enter_int32(vocab, models[i]->word_str[j], j);
        }
    }
    
    if (hash_table_lookup(vocab, "<UNK>", NULL) != 0)
        (void)hash_table_enter_int32(vocab, "<UNK>", 0);
    
    ngram_model_init(base, &ngram_model_set_funcs, lmath, n, hash_table_inuse(vocab));
    base->writable = FALSE; 
    i = 0;
    hlist = hash_table_tolist(vocab, NULL);
    for (gn = hlist; gn; gn = gnode_next(gn)) {
        hash_entry_t *ent = gnode_ptr(gn);
        base->word_str[i++] = (char *)ent->key;
    }
    glist_free(hlist);
    qsort(base->word_str, base->n_words, sizeof(*base->word_str), my_compare);

    
    if (set->widmap)
        ckd_free_2d((void **)set->widmap);
    set->widmap = (int32 **) ckd_calloc_2d(base->n_words, set->n_models,
                                           sizeof(**set->widmap));
    for (i = 0; i < base->n_words; ++i) {
        int32 j;
        
        (void)hash_table_enter_int32(base->wid, base->word_str[i], i);
        
        for (j = 0; j < set->n_models; ++j) {
            set->widmap[i][j] = ngram_wid(models[j], base->word_str[i]);
            
        }
        
    }
    hash_table_free(vocab);
}

ngram_model_t *
ngram_model_set_init(cmd_ln_t *config,
                     ngram_model_t **models,
                     char **names,
                     const float32 *weights,
                     int32 n_models)
{
    ngram_model_set_t *model;
    ngram_model_t *base;
    logmath_t *lmath;
    int32 i, n;

    if (n_models == 0) 
        return NULL;

    

    lmath = models[0]->lmath;
    for (i = 1; i < n_models; ++i) {
        if (logmath_get_base(models[i]->lmath) != logmath_get_base(lmath)
            || logmath_get_shift(models[i]->lmath) != logmath_get_shift(lmath)) {
            E_ERROR("Log-math parameters don't match, will not create LM set\n");
            return NULL;
        }
    }

    
    model = ckd_calloc(1, sizeof(*model));
    base = &model->base;
    model->n_models = n_models;
    model->lms = ckd_calloc(n_models, sizeof(*model->lms));
    model->names = ckd_calloc(n_models, sizeof(*model->names));
    
    model->lweights = ckd_calloc(n_models, sizeof(*model->lweights));
    {
        int32 uniform = logmath_log(lmath, 1.0/n_models);
        for (i = 0; i < n_models; ++i)
            model->lweights[i] = uniform;
    }
    
    if (weights)
        model->cur = -1;

    n = 0;
    for (i = 0; i < n_models; ++i) {
        model->lms[i] = ngram_model_retain(models[i]);
        model->names[i] = ckd_salloc(names[i]);
        if (weights)
            model->lweights[i] = logmath_log(lmath, weights[i]);
        
        if (models[i]->n > n)
            n = models[i]->n;
    }
    
    model->maphist = ckd_calloc(n - 1, sizeof(*model->maphist));

    
    build_widmap(base, lmath, n);
    return base;
}

ngram_model_t *
ngram_model_set_read(cmd_ln_t *config,
                     const char *lmctlfile,
                     logmath_t *lmath)
{
    FILE *ctlfp;
    glist_t lms = NULL;
    glist_t lmnames = NULL;
    __BIGSTACKVARIABLE__ char str[1024];
    ngram_model_t *set = NULL;
    hash_table_t *classes;
    char *basedir, *c;

    

    classes = hash_table_new(0, FALSE);
    if ((ctlfp = fopen(lmctlfile, "r")) == NULL) {
        E_ERROR_SYSTEM("Failed to open %s", lmctlfile);
        return NULL;
    }

    

    if ((c = strrchr(lmctlfile, '/')) || (c = strrchr(lmctlfile, '\\'))) {
        
        basedir = ckd_calloc(c - lmctlfile + 2, 1);
        memcpy(basedir, lmctlfile, c - lmctlfile + 1);
    }
    else {
        basedir = NULL;
    }
    E_INFO("Reading LM control file '%s'\n", lmctlfile);
    if (basedir)
        E_INFO("Will prepend '%s' to unqualified paths\n", basedir);

    if (fscanf(ctlfp, "%1023s", str) == 1) {
        if (strcmp(str, "{") == 0) {
            
            while ((fscanf(ctlfp, "%1023s", str) == 1)
                   && (strcmp(str, "}") != 0)) {
                char *deffile;
                if (basedir && !path_is_absolute(str))
                    deffile = string_join(basedir, str, NULL);
                else
                    deffile = ckd_salloc(str);
                E_INFO("Reading classdef from '%s'\n", deffile);
                if (read_classdef_file(classes, deffile) < 0) {
                    ckd_free(deffile);
                    goto error_out;
                }
                ckd_free(deffile);
            }

            if (strcmp(str, "}") != 0) {
                E_ERROR("Unexpected EOF in %s\n", lmctlfile);
                goto error_out;
            }

            
            if (fscanf(ctlfp, "%1023s", str) != 1)
                str[0] = '\0';
        }
    }
    else
        str[0] = '\0';

    
    while (str[0] != '\0') {
        char *lmfile;
        ngram_model_t *lm;

        if (basedir && str[0] != '/' && str[0] != '\\')
            lmfile = string_join(basedir, str, NULL);
        else
            lmfile = ckd_salloc(str);
        E_INFO("Reading lm from '%s'\n", lmfile);
        lm = ngram_model_read(config, lmfile, NGRAM_AUTO, lmath);
        if (lm == NULL) {
            ckd_free(lmfile);
            goto error_out;
        }
        if (fscanf(ctlfp, "%1023s", str) != 1) {
            E_ERROR("LMname missing after LMFileName '%s'\n", lmfile);
            ckd_free(lmfile);
            goto error_out;
        }
        ckd_free(lmfile);
        lms = glist_add_ptr(lms, lm);
        lmnames = glist_add_ptr(lmnames, ckd_salloc(str));

        if (fscanf(ctlfp, "%1023s", str) == 1) {
            if (strcmp(str, "{") == 0) {
                
                while ((fscanf(ctlfp, "%1023s", str) == 1) &&
                       (strcmp(str, "}") != 0)) {
                    void *val;
                    classdef_t *classdef;

                    if (hash_table_lookup(classes, str, &val) == -1) {
                        E_ERROR("Unknown class %s in control file\n", str);
                        goto error_out;
                    }
                    classdef = val;
                    if (ngram_model_add_class(lm, str, 1.0,
                                              classdef->words, classdef->weights,
                                              classdef->n_words) < 0) {
                        goto error_out;
                    }
                    E_INFO("Added class %s containing %d words\n",
                           str, classdef->n_words);
                }
                if (strcmp(str, "}") != 0) {
                    E_ERROR("Unexpected EOF in %s\n", lmctlfile);
                    goto error_out;
                }
                if (fscanf(ctlfp, "%1023s", str) != 1)
                    str[0] = '\0';
            }
        }
        else
            str[0] = '\0';
    }
    fclose(ctlfp);

    

    lms = glist_reverse(lms);
    lmnames = glist_reverse(lmnames);
    {
        int32 n_models;
        ngram_model_t **lm_array;
        char **name_array;
        gnode_t *lm_node, *name_node;
        int32 i;

        n_models = glist_count(lms);
        lm_array = ckd_calloc(n_models, sizeof(*lm_array));
        name_array = ckd_calloc(n_models, sizeof(*name_array));
        lm_node = lms;
        name_node = lmnames;
        for (i = 0; i < n_models; ++i) {
            lm_array[i] = gnode_ptr(lm_node);
            name_array[i] = gnode_ptr(name_node);
            lm_node = gnode_next(lm_node);
            name_node = gnode_next(name_node);
        }
        set = ngram_model_set_init(config, lm_array, name_array,
                                   NULL, n_models);
        ckd_free(lm_array);
        ckd_free(name_array);
    }
error_out:
    {
        gnode_t *gn;
        glist_t hlist;

        if (set == NULL) {
            for (gn = lms; gn; gn = gnode_next(gn)) {
                ngram_model_free(gnode_ptr(gn));
            }
        }
        glist_free(lms);
        for (gn = lmnames; gn; gn = gnode_next(gn)) {
            ckd_free(gnode_ptr(gn));
        }
        glist_free(lmnames);
        hlist = hash_table_tolist(classes, NULL);
        for (gn = hlist; gn; gn = gnode_next(gn)) {
            hash_entry_t *he = gnode_ptr(gn);
            ckd_free((char *)he->key);
            classdef_free(he->val);
        }
        glist_free(hlist);
        hash_table_free(classes);
        ckd_free(basedir);
    }
    return set;
}

int32
ngram_model_set_count(ngram_model_t *base)
{
    ngram_model_set_t *set = (ngram_model_set_t *)base;
    return set->n_models;
}

ngram_model_set_iter_t *
ngram_model_set_iter(ngram_model_t *base)
{
    ngram_model_set_t *set = (ngram_model_set_t *)base;
    ngram_model_set_iter_t *itor;

    if (set == NULL || set->n_models == 0)
        return NULL;
    itor = ckd_calloc(1, sizeof(*itor));
    itor->set = set;
    return itor;
}

ngram_model_set_iter_t *
ngram_model_set_iter_next(ngram_model_set_iter_t *itor)
{
    if (++itor->cur == itor->set->n_models) {
        ngram_model_set_iter_free(itor);
        return NULL;
    }
    return itor;
}

void
ngram_model_set_iter_free(ngram_model_set_iter_t *itor)
{
    ckd_free(itor);
}

ngram_model_t *
ngram_model_set_iter_model(ngram_model_set_iter_t *itor,
                           char const **lmname)
{
    if (lmname) *lmname = itor->set->names[itor->cur];
    return itor->set->lms[itor->cur];
}

ngram_model_t *
ngram_model_set_lookup(ngram_model_t *base,
                       const char *name)
{
    ngram_model_set_t *set = (ngram_model_set_t *)base;
    int32 i;

    if (name == NULL) {
        if (set->cur == -1)
            return NULL;
        else
            return set->lms[set->cur];
    }

    
    for (i = 0; i < set->n_models; ++i)
        if (0 == strcmp(set->names[i], name))
            break;
    if (i == set->n_models)
        return NULL;
    return set->lms[i];
}

ngram_model_t *
ngram_model_set_select(ngram_model_t *base,
                       const char *name)
{
    ngram_model_set_t *set = (ngram_model_set_t *)base;
    int32 i;

    
    for (i = 0; i < set->n_models; ++i)
        if (0 == strcmp(set->names[i], name))
            break;
    if (i == set->n_models)
        return NULL;
    set->cur = i;
    return set->lms[set->cur];
}

const char *
ngram_model_set_current(ngram_model_t *base)
{
    ngram_model_set_t *set = (ngram_model_set_t *)base;

    if (set->cur == -1)
        return NULL;
    else
        return set->names[set->cur];
}

int32
ngram_model_set_current_wid(ngram_model_t *base,
                            int32 set_wid)
{
    ngram_model_set_t *set = (ngram_model_set_t *)base;

    if (set->cur == -1 || set_wid >= base->n_words)
        return NGRAM_INVALID_WID;
    else
        return set->widmap[set_wid][set->cur];
}

int32
ngram_model_set_known_wid(ngram_model_t *base,
                          int32 set_wid)
{
    ngram_model_set_t *set = (ngram_model_set_t *)base;

    if (set_wid >= base->n_words)
        return FALSE;
    else if (set->cur == -1) {
        int32 i;
        for (i = 0; i < set->n_models; ++i) {
            if (set->widmap[set_wid][i] != ngram_unknown_wid(set->lms[i]))
                return TRUE;
        }
        return FALSE;
    }
    else
        return (set->widmap[set_wid][set->cur]
                != ngram_unknown_wid(set->lms[set->cur]));
}

ngram_model_t *
ngram_model_set_interp(ngram_model_t *base,
                       const char **names,
                       const float32 *weights)
{
    ngram_model_set_t *set = (ngram_model_set_t *)base;

    
    if (names && weights) {
        int32 i, j;

        
        for (i = 0; i < set->n_models; ++i) {
            for (j = 0; j < set->n_models; ++j)
                if (0 == strcmp(names[i], set->names[j]))
                    break;
            if (j == set->n_models) {
                E_ERROR("Unknown LM name %s\n", names[i]);
                return NULL;
            }
            set->lweights[j] = logmath_log(base->lmath, weights[i]);
        }
    }
    else if (weights) {
        memcpy(set->lweights, weights, set->n_models * sizeof(*set->lweights));
    }
    
    set->cur = -1;
    return base;
}

ngram_model_t *
ngram_model_set_add(ngram_model_t *base,
                    ngram_model_t *model,
                    const char *name,
                    float32 weight,
                    int reuse_widmap)
                    
{
    ngram_model_set_t *set = (ngram_model_set_t *)base;
    float32 fprob;
    int32 scale, i;

    
    ++set->n_models;
    set->lms = ckd_realloc(set->lms, set->n_models * sizeof(*set->lms));
    set->lms[set->n_models - 1] = model;
    set->names = ckd_realloc(set->names, set->n_models * sizeof(*set->names));
    set->names[set->n_models - 1] = ckd_salloc(name);
    
    if (model->n > base->n) {
        base->n = model->n;
        set->maphist = ckd_realloc(set->maphist,
                                   (model->n - 1) * sizeof(*set->maphist));
    }

    
    fprob = weight * 1.0 / set->n_models;
    set->lweights = ckd_realloc(set->lweights,
                                set->n_models * sizeof(*set->lweights));
    set->lweights[set->n_models - 1] = logmath_log(base->lmath, fprob);
    


    scale = logmath_log(base->lmath, 1.0 - fprob);
    for (i = 0; i < set->n_models - 1; ++i)
        set->lweights[i] += scale;

    
    if (reuse_widmap) {
        int32 **new_widmap;

        
        new_widmap = (int32 **)ckd_calloc_2d(base->n_words, set->n_models,
                                             sizeof (**new_widmap));
        for (i = 0; i < base->n_words; ++i) {
            
            memcpy(new_widmap[i], set->widmap[i],
                   (set->n_models - 1) * sizeof(**new_widmap));
            
            new_widmap[i][set->n_models-1] = ngram_wid(model, base->word_str[i]);
        }
        ckd_free_2d((void **)set->widmap);
        set->widmap = new_widmap;
    }
    else {
        build_widmap(base, base->lmath, base->n);
    }
    return model;
}

ngram_model_t *
ngram_model_set_remove(ngram_model_t *base,
                       const char *name,
                       int reuse_widmap)
{
    ngram_model_set_t *set = (ngram_model_set_t *)base;
    ngram_model_t *submodel;
    int32 lmidx, scale, n, i;
    float32 fprob;

    for (lmidx = 0; lmidx < set->n_models; ++lmidx)
        if (0 == strcmp(name, set->names[lmidx]))
            break;
    if (lmidx == set->n_models)
        return NULL;
    submodel = set->lms[lmidx];

    

    fprob = logmath_exp(base->lmath, set->lweights[lmidx]);
    scale = logmath_log(base->lmath, 1.0 - fprob);

    

    --set->n_models;
    n = 0;
    ckd_free(set->names[lmidx]);
    set->names[lmidx] = NULL;
    for (i = 0; i < set->n_models; ++i) {
        if (i >= lmidx) {
            set->lms[i] = set->lms[i+1];
            set->names[i] = set->names[i+1];
            set->lweights[i] = set->lweights[i+1];
        }
        set->lweights[i] -= scale;
        if (set->lms[i]->n > n)
            n = set->lms[i]->n;
    }
    
    set->lms[set->n_models] = NULL;
    set->lweights[set->n_models] = base->log_zero;
    

    
    if (reuse_widmap) {
        
        for (i = 0; i < base->n_words; ++i) {
            memmove(set->widmap[i] + lmidx, set->widmap[i] + lmidx + 1,
                    (set->n_models - lmidx) * sizeof(**set->widmap));
        }
    }
    else {
        build_widmap(base, base->lmath, n);
    }
    return submodel;
}

void
ngram_model_set_map_words(ngram_model_t *base,
                          const char **words,
                          int32 n_words)
{
    ngram_model_set_t *set = (ngram_model_set_t *)base;
    int32 i;

    
    if (base->writable) {
        for (i = 0; i < base->n_words; ++i) {
            ckd_free(base->word_str[i]);
        }
    }
    ckd_free(base->word_str);
    ckd_free_2d((void **)set->widmap);
    base->writable = TRUE;
    base->n_words = base->n_1g_alloc = n_words;
    base->word_str = ckd_calloc(n_words, sizeof(*base->word_str));
    set->widmap = (int32 **)ckd_calloc_2d(n_words, set->n_models, sizeof(**set->widmap));
    hash_table_empty(base->wid);
    for (i = 0; i < n_words; ++i) {
        int32 j;
        base->word_str[i] = ckd_salloc(words[i]);
        (void)hash_table_enter_int32(base->wid, base->word_str[i], i);
        for (j = 0; j < set->n_models; ++j) {
            set->widmap[i][j] = ngram_wid(set->lms[j], base->word_str[i]);
        }
    }
}

static int
ngram_model_set_apply_weights(ngram_model_t *base, float32 lw,
                              float32 wip, float32 uw)
{
    ngram_model_set_t *set = (ngram_model_set_t *)base;
    int32 i;

    
    for (i = 0; i < set->n_models; ++i)
        ngram_model_apply_weights(set->lms[i], lw, wip, uw);
    return 0;
}

static int32
ngram_model_set_score(ngram_model_t *base, int32 wid,
                      int32 *history, int32 n_hist,
                      int32 *n_used)
{
    ngram_model_set_t *set = (ngram_model_set_t *)base;
    int32 mapwid;
    int32 score;
    int32 i;

    
    if (n_hist > base->n - 1)
        n_hist = base->n - 1;

    
    if (set->cur == -1) {
        score = base->log_zero;
        for (i = 0; i < set->n_models; ++i) {
            int32 j;
            
            mapwid = set->widmap[wid][i];
            for (j = 0; j < n_hist; ++j) {
                if (history[j] == NGRAM_INVALID_WID)
                    set->maphist[j] = NGRAM_INVALID_WID;
                else
                    set->maphist[j] = set->widmap[history[j]][i];
            }
            score = logmath_add(base->lmath, score,
                                set->lweights[i] + 
                                ngram_ng_score(set->lms[i],
                                               mapwid, set->maphist, n_hist, n_used));
        }
    }
    else {
        int32 j;
        
        mapwid = set->widmap[wid][set->cur];
        for (j = 0; j < n_hist; ++j) {
            if (history[j] == NGRAM_INVALID_WID)
                set->maphist[j] = NGRAM_INVALID_WID;
            else
                set->maphist[j] = set->widmap[history[j]][set->cur];
        }
        score = ngram_ng_score(set->lms[set->cur],
                               mapwid, set->maphist, n_hist, n_used);
    }

    return score;
}

static int32
ngram_model_set_raw_score(ngram_model_t *base, int32 wid,
                          int32 *history, int32 n_hist,
                          int32 *n_used)
{
    ngram_model_set_t *set = (ngram_model_set_t *)base;
    int32 mapwid;
    int32 score;
    int32 i;

    
    if (n_hist > base->n - 1)
        n_hist = base->n - 1;

    
    if (set->cur == -1) {
        score = base->log_zero;
        for (i = 0; i < set->n_models; ++i) {
            int32 j;
            
            mapwid = set->widmap[wid][i];
            for (j = 0; j < n_hist; ++j) {
                if (history[j] == NGRAM_INVALID_WID)
                    set->maphist[j] = NGRAM_INVALID_WID;
                else
                    set->maphist[j] = set->widmap[history[j]][i];
            }
            score = logmath_add(base->lmath, score,
                                set->lweights[i] + 
                                ngram_ng_prob(set->lms[i],
                                              mapwid, set->maphist, n_hist, n_used));
        }
    }
    else {
        int32 j;
        
        mapwid = set->widmap[wid][set->cur];
        for (j = 0; j < n_hist; ++j) {
            if (history[j] == NGRAM_INVALID_WID)
                set->maphist[j] = NGRAM_INVALID_WID;
            else
                set->maphist[j] = set->widmap[history[j]][set->cur];
        }
        score = ngram_ng_prob(set->lms[set->cur],
                              mapwid, set->maphist, n_hist, n_used);
    }

    return score;
}

static int32
ngram_model_set_add_ug(ngram_model_t *base,
                       int32 wid, int32 lweight)
{
    ngram_model_set_t *set = (ngram_model_set_t *)base;
    int32 *newwid;
    int32 i, prob;

    


    newwid = ckd_calloc(set->n_models, sizeof(*newwid));
    prob = base->log_zero;
    for (i = 0; i < set->n_models; ++i) {
        int32 wprob, n_hist;

        
        if (set->cur == -1 || set->cur == i) {
            
            newwid[i] = ngram_wid(set->lms[i], base->word_str[wid]);
            if (newwid[i] == NGRAM_INVALID_WID) {
                
                newwid[i] = ngram_model_add_word(set->lms[i], base->word_str[wid],
                                                 logmath_exp(base->lmath, lweight));
                if (newwid[i] == NGRAM_INVALID_WID) {
                    ckd_free(newwid);
                    return base->log_zero;
                }
            }
            

            wprob = ngram_ng_prob(set->lms[i], newwid[i], NULL, 0, &n_hist);
            if (set->cur == i)
                prob = wprob;
            else if (set->cur == -1)
                prob = logmath_add(base->lmath, prob, set->lweights[i] + wprob);
        }
        else {
            newwid[i] = NGRAM_INVALID_WID;
        }
    }
    


    set->widmap = ckd_realloc(set->widmap, base->n_words * sizeof(*set->widmap));
    set->widmap[0] = ckd_realloc(set->widmap[0],
                                 base->n_words
                                 * set->n_models
                                 * sizeof(**set->widmap));
    for (i = 0; i < base->n_words; ++i)
        set->widmap[i] = set->widmap[0] + i * set->n_models;
    memcpy(set->widmap[wid], newwid, set->n_models * sizeof(*newwid));
    ckd_free(newwid);
    return prob;
}

static void
ngram_model_set_free(ngram_model_t *base)
{
    ngram_model_set_t *set = (ngram_model_set_t *)base;
    int32 i;

    for (i = 0; i < set->n_models; ++i)
        ngram_model_free(set->lms[i]);
    ckd_free(set->lms);
    for (i = 0; i < set->n_models; ++i)
        ckd_free(set->names[i]);
    ckd_free(set->names);
    ckd_free(set->lweights);
    ckd_free(set->maphist);
    ckd_free_2d((void **)set->widmap);
}

static void
ngram_model_set_flush(ngram_model_t *base)
{
    ngram_model_set_t *set = (ngram_model_set_t *)base;
    int32 i;

    for (i = 0; i < set->n_models; ++i)
        ngram_model_flush(set->lms[i]);
}

static ngram_funcs_t ngram_model_set_funcs = {
    ngram_model_set_free,          
    ngram_model_set_apply_weights, 
    ngram_model_set_score,         
    ngram_model_set_raw_score,     
    ngram_model_set_add_ug,        
    ngram_model_set_flush          
};
