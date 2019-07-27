









































#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <limits.h>

#include "sphinxbase/ckd_alloc.h"
#include "sphinxbase/pio.h"
#include "sphinxbase/err.h"
#include "sphinxbase/byteorder.h"
#include "sphinxbase/listelem_alloc.h"

#include "ngram_model_dmp.h"

static const char darpa_hdr[] = "Darpa Trigram LM";
static ngram_funcs_t ngram_model_dmp_funcs;

#define TSEG_BASE(m,b)		((m)->lm3g.tseg_base[(b)>>LOG_BG_SEG_SZ])
#define FIRST_BG(m,u)		((m)->lm3g.unigrams[u].bigrams)
#define FIRST_TG(m,b)		(TSEG_BASE((m),(b))+((m)->lm3g.bigrams[b].trigrams))

static unigram_t *
new_unigram_table(int32 n_ug)
{
    unigram_t *table;
    int32 i;

    table = ckd_calloc(n_ug, sizeof(unigram_t));
    for (i = 0; i < n_ug; i++) {
        table[i].prob1.f = -99.0;
        table[i].bo_wt1.f = -99.0;
    }
    return table;
}

ngram_model_t *
ngram_model_dmp_read(cmd_ln_t *config,
                     const char *file_name,
                     logmath_t *lmath)
{
    ngram_model_t *base;
    ngram_model_dmp_t *model;
    FILE *fp;
    int do_mmap, do_swap;
    int32 is_pipe;
    int32 i, j, k, vn, n, ts;
    int32 n_unigram;
    int32 n_bigram;
    int32 n_trigram;
    char str[1024];
    unigram_t *ugptr;
    bigram_t *bgptr;
    trigram_t *tgptr;
    char *tmp_word_str;
    char *map_base = NULL;
    size_t offset = 0;

    base = NULL;
    do_mmap = FALSE;
    if (config)
        do_mmap = cmd_ln_boolean_r(config, "-mmap");

    if ((fp = fopen_comp(file_name, "rb", &is_pipe)) == NULL) {
        E_ERROR("Dump file %s not found\n", file_name);
        goto error_out;
    }

    if (is_pipe && do_mmap) {
        E_WARN("Dump file is compressed, will not use memory-mapped I/O\n");
        do_mmap = 0;
    }

    do_swap = FALSE;
    if (fread(&k, sizeof(k), 1, fp) != 1)
        goto error_out;
    if (k != strlen(darpa_hdr)+1) {
        SWAP_INT32(&k);
        if (k != strlen(darpa_hdr)+1) {
            E_ERROR("Wrong magic header size number %x: %s is not a dump file\n", k, file_name);
            goto error_out;
        }
        do_swap = 1;
    }
    if (fread(str, 1, k, fp) != (size_t) k) {
        E_ERROR("Cannot read header\n");
        goto error_out;
    }
    if (strncmp(str, darpa_hdr, k) != 0) {
        E_ERROR("Wrong header %s: %s is not a dump file\n", darpa_hdr);
        goto error_out;
    }

    if (do_mmap) {
        if (do_swap) {
            E_INFO
                ("Byteswapping required, will not use memory-mapped I/O for LM file\n");
            do_mmap = 0;
        }
        else {
            E_INFO("Will use memory-mapped I/O for LM file\n");
#ifdef __ADSPBLACKFIN__ 
            E_FATAL("memory mapping is not supported at the moment.");
#else
#endif
        }
    }

    if (fread(&k, sizeof(k), 1, fp) != 1)
        goto error_out;
    if (do_swap) SWAP_INT32(&k);
    if (fread(str, 1, k, fp) != (size_t) k) {
        E_ERROR("Cannot read LM filename in header\n");
        goto error_out;
    }

    
    if (fread(&vn, sizeof(vn), 1, fp) != 1)
        goto error_out;
    if (do_swap) SWAP_INT32(&vn);
    if (vn <= 0) {
        
        if (fread(&ts, sizeof(ts), 1, fp) != 1)
            goto error_out;
        if (do_swap) SWAP_INT32(&ts);

        
        for (;;) {
            if (fread(&k, sizeof(k), 1, fp) != 1)
                goto error_out;
            if (do_swap) SWAP_INT32(&k);
            if (k == 0)
                break;
            if (fread(str, 1, k, fp) != (size_t) k) {
                E_ERROR("Failed to read word\n");
                goto error_out;
            }
        }
        
        if (fread(&n_unigram, sizeof(n_unigram), 1, fp) != 1)
            goto error_out;
        if (do_swap) SWAP_INT32(&n_unigram);
    }
    else {
        n_unigram = vn;
    }

    
    if (fread(&n_bigram, sizeof(n_bigram), 1, fp) != 1)
        goto error_out;
    if (do_swap) SWAP_INT32(&n_bigram);
    if (fread(&n_trigram, sizeof(n_trigram), 1, fp) != 1)
        goto error_out;
    if (do_swap) SWAP_INT32(&n_trigram);
    E_INFO("ngrams 1=%d, 2=%d, 3=%d\n", n_unigram, n_bigram, n_trigram);

    
    model = ckd_calloc(1, sizeof(*model));
    base = &model->base;
    if (n_trigram > 0)
        n = 3;
    else if (n_bigram > 0)
        n = 2;
    else
        n = 1;
    ngram_model_init(base, &ngram_model_dmp_funcs, lmath, n, n_unigram);
    base->n_counts[0] = n_unigram;
    base->n_counts[1] = n_bigram;
    base->n_counts[2] = n_trigram;

    

    model->lm3g.unigrams = new_unigram_table(n_unigram + 1);
    ugptr = model->lm3g.unigrams;
    for (i = 0; i <= n_unigram; ++i) {
        
        if (fread(ugptr, sizeof(int32), 1, fp) != 1) {
            E_ERROR("Failed to read maping id %d\n", i);
            goto error_out;
        }
        
        if (fread(ugptr, sizeof(unigram_t), 1, fp) != 1)  {
            E_ERROR("Failed to read unigrams data\n");
            ngram_model_free(base);
            fclose_comp(fp, is_pipe);
            return NULL;
        }
        
        if (do_swap) {
            SWAP_INT32(&ugptr->prob1.l);
            SWAP_INT32(&ugptr->bo_wt1.l);
            SWAP_INT32(&ugptr->bigrams);
        }
        
        ugptr->prob1.l = logmath_log10_to_log(lmath, ugptr->prob1.f);
        ugptr->bo_wt1.l = logmath_log10_to_log(lmath, ugptr->bo_wt1.f);
        E_DEBUG(2, ("ug %d: prob %d bo %d bigrams %d\n",
                    i, ugptr->prob1.l, ugptr->bo_wt1.l, ugptr->bigrams));
        ++ugptr;
    }
    E_INFO("%8d = LM.unigrams(+trailer) read\n", n_unigram);

    
    if (do_mmap) {
        offset = ftell(fp);

        
        if (offset & 0x3) {
            E_WARN("-mmap specified, but trigram index is not word-aligned.  Will not memory-map.\n");
            do_mmap = FALSE;
        }
        else {
            model->dump_mmap = mmio_file_read(file_name);
            if (model->dump_mmap == NULL) {
                do_mmap = FALSE;
            }
            else {
                map_base = mmio_file_ptr(model->dump_mmap);
            }
        }
    }
    
    if (n_bigram > 0) {
        
	if (do_mmap) {
    	    model->lm3g.bigrams = (bigram_t *) (map_base + offset);
    	    offset += (n_bigram + 1) * sizeof(bigram_t);
	}
	else {
    	    model->lm3g.bigrams =
        	ckd_calloc(n_bigram + 1, sizeof(bigram_t));
    	    if (fread(model->lm3g.bigrams, sizeof(bigram_t), n_bigram + 1, fp)
        	!= (size_t) n_bigram + 1) {
    		E_ERROR("Failed to read bigrams data\n");
        	goto error_out;
    	    }
    	    if (do_swap) {
        	for (i = 0, bgptr = model->lm3g.bigrams; i <= n_bigram;
            	     i++, bgptr++) {
            	    SWAP_INT16(&bgptr->wid);
            	    SWAP_INT16(&bgptr->prob2);
            	    SWAP_INT16(&bgptr->bo_wt2);
            	    SWAP_INT16(&bgptr->trigrams);
        	}
    	    }
	}
	E_INFO("%8d = LM.bigrams(+trailer) read\n", n_bigram);
    }

    
    if (n_trigram > 0) {
        if (do_mmap) {
            model->lm3g.trigrams = (trigram_t *) (map_base + offset);
            offset += n_trigram * sizeof(trigram_t);
        }
        else {
            model->lm3g.trigrams =
                ckd_calloc(n_trigram, sizeof(trigram_t));
            if (fread
                (model->lm3g.trigrams, sizeof(trigram_t), n_trigram, fp)
                != (size_t) n_trigram) {
                E_ERROR("Failed to read trigrams data\n");
                goto error_out;
            }
            if (do_swap) {
                for (i = 0, tgptr = model->lm3g.trigrams; i < n_trigram;
                     i++, tgptr++) {
                    SWAP_INT16(&tgptr->wid);
                    SWAP_INT16(&tgptr->prob3);
                }
            }
        }
        E_INFO("%8d = LM.trigrams read\n", n_trigram);
        
        model->lm3g.tginfo = ckd_calloc(n_unigram, sizeof(tginfo_t *));
        model->lm3g.le = listelem_alloc_init(sizeof(tginfo_t));
    }

    if (n_bigram > 0) {
        
	if (do_mmap)
    	    fseek(fp, offset, SEEK_SET);
        if (fread(&k, sizeof(k), 1, fp) != 1)
	    goto error_out;
        if (do_swap) SWAP_INT32(&k);
	model->lm3g.n_prob2 = k;
        model->lm3g.prob2 = ckd_calloc(k, sizeof(*model->lm3g.prob2));
	if (fread(model->lm3g.prob2, sizeof(*model->lm3g.prob2), k, fp) != (size_t) k) {
    	    E_ERROR("fread(prob2) failed\n");
    	    goto error_out;
	}
	for (i = 0; i < k; i++) {
    	    if (do_swap)
        	SWAP_INT32(&model->lm3g.prob2[i].l);
    	    
    	    model->lm3g.prob2[i].l = logmath_log10_to_log(lmath, model->lm3g.prob2[i].f);
	}
	E_INFO("%8d = LM.prob2 entries read\n", k);
    }

    
    if (base->n > 2) {
        if (fread(&k, sizeof(k), 1, fp) != 1)
            goto error_out;
        if (do_swap) SWAP_INT32(&k);
        model->lm3g.n_bo_wt2 = k;
        model->lm3g.bo_wt2 = ckd_calloc(k, sizeof(*model->lm3g.bo_wt2));
        if (fread(model->lm3g.bo_wt2, sizeof(*model->lm3g.bo_wt2), k, fp) != (size_t) k) {
            E_ERROR("Failed to read backoff weights\n");
            goto error_out;
        }
        for (i = 0; i < k; i++) {
            if (do_swap)
                SWAP_INT32(&model->lm3g.bo_wt2[i].l);
            
            model->lm3g.bo_wt2[i].l = logmath_log10_to_log(lmath, model->lm3g.bo_wt2[i].f);
        }
        E_INFO("%8d = LM.bo_wt2 entries read\n", k);
    }

    
    if (base->n > 2) {
        if (fread(&k, sizeof(k), 1, fp) != 1)
		goto error_out;
	if (do_swap) SWAP_INT32(&k);
    	model->lm3g.n_prob3 = k;
    	model->lm3g.prob3 = ckd_calloc(k, sizeof(*model->lm3g.prob3));
    	if (fread(model->lm3g.prob3, sizeof(*model->lm3g.prob3), k, fp) != (size_t) k) {
    	    E_ERROR("Failed to read trigram probability\n");
    	    goto error_out;
    	}
    	for (i = 0; i < k; i++) {
    	    if (do_swap)
                SWAP_INT32(&model->lm3g.prob3[i].l);
    	    
    	    model->lm3g.prob3[i].l = logmath_log10_to_log(lmath, model->lm3g.prob3[i].f);
    	}
        E_INFO("%8d = LM.prob3 entries read\n", k);
    }

    
    if (do_mmap)
        offset = ftell(fp);
    if (n_trigram > 0) {
        if (do_mmap) {
            memcpy(&k, map_base + offset, sizeof(k));
            offset += sizeof(int32);
            model->lm3g.tseg_base = (int32 *) (map_base + offset);
            offset += k * sizeof(int32);
        }
        else {
            k = (n_bigram + 1) / BG_SEG_SZ + 1;
            if (fread(&k, sizeof(k), 1, fp) != 1)
                goto error_out;
            if (do_swap) SWAP_INT32(&k);
            model->lm3g.tseg_base = ckd_calloc(k, sizeof(int32));
            if (fread(model->lm3g.tseg_base, sizeof(int32), k, fp) !=
                (size_t) k) {
                E_ERROR("Failed to read trigram index\n");
                goto error_out;
            }
            if (do_swap)
                for (i = 0; i < k; i++)
                    SWAP_INT32(&model->lm3g.tseg_base[i]);
        }
        E_INFO("%8d = LM.tseg_base entries read\n", k);
    }

    
    if (do_mmap) {
        memcpy(&k, map_base + offset, sizeof(k));
        offset += sizeof(int32);
        tmp_word_str = (char *) (map_base + offset);
        offset += k;
    }
    else {
        base->writable = TRUE;
        if (fread(&k, sizeof(k), 1, fp) != 1)
            goto error_out;
        if (do_swap) SWAP_INT32(&k);
        tmp_word_str = ckd_calloc(k, 1);
        if (fread(tmp_word_str, 1, k, fp) != (size_t) k) {
            E_ERROR("Failed to read words\n");
            goto error_out;
        }
    }

    
    for (i = 0, j = 0; i < k; i++)
        if (tmp_word_str[i] == '\0')
            j++;
    if (j != n_unigram) {
        E_ERROR("Error reading word strings (%d doesn't match n_unigrams %d)\n",
                j, n_unigram);
        goto error_out;
    }

    
    if (do_mmap) {
        j = 0;
        for (i = 0; i < n_unigram; i++) {
            base->word_str[i] = tmp_word_str + j;
            if (hash_table_enter(base->wid, base->word_str[i],
                                 (void *)(long)i) != (void *)(long)i) {
                E_WARN("Duplicate word in dictionary: %s\n", base->word_str[i]);
            }
            j += strlen(base->word_str[i]) + 1;
        }
    }
    else {
        j = 0;
        for (i = 0; i < n_unigram; i++) {
            base->word_str[i] = ckd_salloc(tmp_word_str + j);
            if (hash_table_enter(base->wid, base->word_str[i],
                                 (void *)(long)i) != (void *)(long)i) {
                E_WARN("Duplicate word in dictionary: %s\n", base->word_str[i]);
            }
            j += strlen(base->word_str[i]) + 1;
        }
        free(tmp_word_str);
    }
    E_INFO("%8d = ascii word strings read\n", i);

    fclose_comp(fp, is_pipe);
    return base;

error_out:
    if (fp)
        fclose_comp(fp, is_pipe);
    ngram_model_free(base);
    return NULL;
}

ngram_model_dmp_t *
ngram_model_dmp_build(ngram_model_t *base)
{
    ngram_model_dmp_t *model;
    ngram_model_t *newbase;
    ngram_iter_t *itor;
    sorted_list_t sorted_prob2;
    sorted_list_t sorted_bo_wt2;
    sorted_list_t sorted_prob3;
    bigram_t *bgptr;
    trigram_t *tgptr;
    int i, bgcount, tgcount, seg;

    if (base->funcs == &ngram_model_dmp_funcs) {
        E_INFO("Using existing DMP model.\n");
        return (ngram_model_dmp_t *)ngram_model_retain(base);
    }

    
    E_INFO("Building DMP model...\n");
    model = ckd_calloc(1, sizeof(*model));
    newbase = &model->base;
    ngram_model_init(newbase, &ngram_model_dmp_funcs,
                     logmath_retain(base->lmath),
                     base->n, base->n_counts[0]);
    
    memcpy(newbase->n_counts, base->n_counts,
           base->n * sizeof(*base->n_counts));
    
    newbase->writable = TRUE;
    
    model->lm3g.unigrams = new_unigram_table(newbase->n_counts[0] + 1);
    for (itor = ngram_model_mgrams(base, 0); itor;
         itor = ngram_iter_next(itor)) {
        int32 prob1, bo_wt1;
        int32 const *wids;

        

        wids = ngram_iter_get(itor, &prob1, &bo_wt1);
        model->lm3g.unigrams[wids[0]].prob1.l = prob1;
        model->lm3g.unigrams[wids[0]].bo_wt1.l = bo_wt1;
        newbase->word_str[wids[0]] = ckd_salloc(ngram_word(base, wids[0]));
        if ((hash_table_enter_int32(newbase->wid,
                                    newbase->word_str[wids[0]], wids[0]))
            != wids[0]) {
                E_WARN("Duplicate word in dictionary: %s\n", newbase->word_str[wids[0]]);
        }
    }
    E_INFO("%8d = #unigrams created\n", newbase->n_counts[0]);
		
    if (newbase->n < 2) 
        return model;
			 
    


    init_sorted_list(&sorted_prob2);
    if (newbase->n > 2) {
        init_sorted_list(&sorted_bo_wt2);
        init_sorted_list(&sorted_prob3);
    }
    
    bgptr = model->lm3g.bigrams = ckd_calloc(newbase->n_counts[1] + 1, sizeof(bigram_t));
    if (newbase->n > 2) {
        tgptr = model->lm3g.trigrams = ckd_calloc(newbase->n_counts[2], sizeof(trigram_t));
        model->lm3g.tseg_base =
            ckd_calloc((newbase->n_counts[1] + 1) / BG_SEG_SZ + 1, sizeof(int32));
    }
    else
        tgptr = NULL;
    


    for (i = 0; i < newbase->n_counts[0]; ++i) {
        ngram_iter_t *uitor;
        bgcount = bgptr - model->lm3g.bigrams;
        
        model->lm3g.unigrams[i].bigrams = bgcount;
        E_DEBUG(2, ("unigram %d: %s => bigram %d\n", i, newbase->word_str[i], bgcount));
        
        uitor = ngram_ng_iter(base, i, NULL, 0);
        for (itor = ngram_iter_successors(uitor);
             itor; ++bgptr, itor = ngram_iter_next(itor)) {
            int32 prob2, bo_wt2;
            int32 const *wids;
            ngram_iter_t *titor;

            wids = ngram_iter_get(itor, &prob2, &bo_wt2);

            assert (bgptr - model->lm3g.bigrams < newbase->n_counts[1]);

            bgptr->wid = wids[1];
            bgptr->prob2 = sorted_id(&sorted_prob2, &prob2);
            if (newbase->n > 2) {
                tgcount = (tgptr - model->lm3g.trigrams);
	        bgcount = (bgptr - model->lm3g.bigrams);

                
                bgptr->bo_wt2 = sorted_id(&sorted_bo_wt2, &bo_wt2);

                

                seg = bgcount >> LOG_BG_SEG_SZ;
                


                if (seg != (bgcount - 1) >> LOG_BG_SEG_SZ)
                    model->lm3g.tseg_base[seg] = tgcount;
                
                bgptr->trigrams = tgcount - model->lm3g.tseg_base[seg];
                E_DEBUG(2, ("bigram %d %s %s => trigram %d:%d\n",
                            bgcount,
                            newbase->word_str[wids[0]],
                            newbase->word_str[wids[1]],
                            seg, bgptr->trigrams));

                
                for (titor = ngram_iter_successors(itor);
                     titor; ++tgptr, titor = ngram_iter_next(titor)) {
                    int32 prob3, dummy;

                    assert(tgptr - model->lm3g.trigrams < newbase->n_counts[2]);
                    wids = ngram_iter_get(titor, &prob3, &dummy);
                    tgptr->wid = wids[2];
                    tgptr->prob3 = sorted_id(&sorted_prob3, &prob3);
                    E_DEBUG(2, ("trigram %d %s %s %s => prob %d\n",
                                tgcount,
                                newbase->word_str[wids[0]],
                                newbase->word_str[wids[1]],
                                newbase->word_str[wids[2]],
                                tgptr->prob3));
                }
            }
        }
        ngram_iter_free(uitor);
    }
    
    bgcount = bgptr - model->lm3g.bigrams;
    tgcount = tgptr - model->lm3g.trigrams;
    seg = bgcount >> LOG_BG_SEG_SZ;
    if (seg != (bgcount - 1) >> LOG_BG_SEG_SZ)
        model->lm3g.tseg_base[seg] = tgcount;
    model->lm3g.unigrams[i].bigrams = bgcount;
    if (newbase->n > 2)
        bgptr->trigrams = tgcount - model->lm3g.tseg_base[seg];

    
    model->lm3g.n_prob2 = sorted_prob2.free;
    model->lm3g.prob2 = vals_in_sorted_list(&sorted_prob2);
    E_INFO("%8d = #bigrams created\n", newbase->n_counts[1]);
    E_INFO("%8d = #prob2 entries\n", model->lm3g.n_prob2);
    free_sorted_list(&sorted_prob2);
    if (newbase->n > 2) {
        
        model->lm3g.n_bo_wt2 = sorted_bo_wt2.free;
        model->lm3g.bo_wt2 = vals_in_sorted_list(&sorted_bo_wt2);
        free_sorted_list(&sorted_bo_wt2);
        E_INFO("%8d = #bo_wt2 entries\n", model->lm3g.n_bo_wt2);
        
        model->lm3g.n_prob3 = sorted_prob3.free;
        model->lm3g.prob3 = vals_in_sorted_list(&sorted_prob3);
        E_INFO("%8d = #trigrams created\n", newbase->n_counts[2]);
        E_INFO("%8d = #prob3 entries\n", model->lm3g.n_prob3);
        free_sorted_list(&sorted_prob3);
        
        model->lm3g.tginfo = ckd_calloc(newbase->n_counts[0], sizeof(tginfo_t *));
        model->lm3g.le = listelem_alloc_init(sizeof(tginfo_t));
    }

    return model;
}

static void
fwrite_int32(FILE *fh, int32 val)
{
    fwrite(&val, 4, 1, fh);
}

static void
fwrite_ug(FILE *fh, unigram_t *ug, logmath_t *lmath)
{
    int32 bogus = -1;
    float32 log10val;

    
    fwrite(&bogus, 4, 1, fh);
    
    log10val = logmath_log_to_log10(lmath, ug->prob1.l);
    fwrite(&log10val, 4, 1, fh);
    log10val = logmath_log_to_log10(lmath, ug->bo_wt1.l);
    fwrite(&log10val, 4, 1, fh);
    fwrite_int32(fh, ug->bigrams);
}

static void
fwrite_bg(FILE *fh, bigram_t *bg)
{
    fwrite(bg, sizeof(*bg), 1, fh);
}

static void
fwrite_tg(FILE *fh, trigram_t *tg)
{
    fwrite(tg, sizeof(*tg), 1, fh);
}



static char const *fmtdesc[] = {
    "BEGIN FILE FORMAT DESCRIPTION",
    "Header string length (int32) and string (including trailing 0)",
    "Original LM filename string-length (int32) and filename (including trailing 0)",
    "(int32) version number (present iff value <= 0)",
    "(int32) original LM file modification timestamp (iff version# present)",
    "(int32) string-length and string (including trailing 0) (iff version# present)",
    "... previous entry continued any number of times (iff version# present)",
    "(int32) 0 (terminating sequence of strings) (iff version# present)",
    "(int32) log_bg_seg_sz (present iff different from default value of LOG2_BG_SEG_SZ)",
    "(int32) lm_t.ucount (must be > 0)",
    "(int32) lm_t.bcount",
    "(int32) lm_t.tcount",
    "lm_t.ucount+1 unigrams (including sentinel)",
    "lm_t.bcount+1 bigrams (including sentinel 64 bits (bg_t) each if version=-1/-2, 128 bits (bg32_t) each if version=-3",
    "lm_t.tcount trigrams (present iff lm_t.tcount > 0 32 bits (tg_t) each if version=-1/-2, 64 bits (tg32_t) each if version=-3)",
    "(int32) lm_t.n_prob2",
    "(int32) lm_t.prob2[]",
    "(int32) lm_t.n_bo_wt2 (present iff lm_t.tcount > 0)",
    "(int32) lm_t.bo_wt2[] (present iff lm_t.tcount > 0)",
    "(int32) lm_t.n_prob3 (present iff lm_t.tcount > 0)",
    "(int32) lm_t.prob3[] (present iff lm_t.tcount > 0)",
    "(int32) (lm_t.bcount+1)/BG_SEG_SZ+1 (present iff lm_t.tcount > 0)",
    "(int32) lm_t.tseg_base[] (present iff lm_t.tcount > 0)",
    "(int32) Sum(all word string-lengths, including trailing 0 for each)",
    "All word strings (including trailing 0 for each)",
    "END FILE FORMAT DESCRIPTION",
    NULL,
};

static void
ngram_model_dmp_write_header(FILE * fh)
{
    int32 k;
    k = strlen(darpa_hdr) + 1;
    fwrite_int32(fh, k);
    fwrite(darpa_hdr, 1, k, fh);
}

static void
ngram_model_dmp_write_lm_filename(FILE * fh, const char *lmfile)
{
    int32 k;

    k = strlen(lmfile) + 1;
    fwrite_int32(fh, k);
    fwrite(lmfile, 1, k, fh);
}

#define LMDMP_VERSION_TG_16BIT -1 /**< VERSION 1 is the simplest DMP file which
				     is trigram or lower which used 16 bits in
				     bigram and trigram.*/

static void
ngram_model_dmp_write_version(FILE * fh, int32 mtime)
{
    fwrite_int32(fh, LMDMP_VERSION_TG_16BIT);   
    fwrite_int32(fh, mtime);
}

static void
ngram_model_dmp_write_ngram_counts(FILE * fh, ngram_model_t *model)
{
    fwrite_int32(fh, model->n_counts[0]);
    fwrite_int32(fh, model->n_counts[1]);
    fwrite_int32(fh, model->n_counts[2]);
}

static void
ngram_model_dmp_write_fmtdesc(FILE * fh)
{
    int32 i, k;
    long pos;

    
    for (i = 0; fmtdesc[i] != NULL; i++) {
        k = strlen(fmtdesc[i]) + 1;
        fwrite_int32(fh, k);
        fwrite(fmtdesc[i], 1, k, fh);
    }
    
    pos = ftell(fh);
    k = pos & 3;
    if (k) {
        fwrite_int32(fh, 4-k);
        fwrite("!!!!", 1, 4-k, fh);
    }
    fwrite_int32(fh, 0);
}

static void
ngram_model_dmp_write_unigram(FILE *fh, ngram_model_t *model)
{
    ngram_model_dmp_t *lm = (ngram_model_dmp_t *)model;
    int32 i;

    for (i = 0; i <= model->n_counts[0]; i++) {
        fwrite_ug(fh, &(lm->lm3g.unigrams[i]), model->lmath);
    }
}


static void
ngram_model_dmp_write_bigram(FILE *fh, ngram_model_t *model)
{
    ngram_model_dmp_t *lm = (ngram_model_dmp_t *)model;
    int32 i;

    for (i = 0; i <= model->n_counts[1]; i++) {
        fwrite_bg(fh, &(lm->lm3g.bigrams[i]));
    }

}

static void
ngram_model_dmp_write_trigram(FILE *fh, ngram_model_t *model)
{
    ngram_model_dmp_t *lm = (ngram_model_dmp_t *)model;
    int32 i;

    for (i = 0; i < model->n_counts[2]; i++) {
        fwrite_tg(fh, &(lm->lm3g.trigrams[i]));
    }
}

static void
ngram_model_dmp_write_bgprob(FILE *fh, ngram_model_t *model)
{
    ngram_model_dmp_t *lm = (ngram_model_dmp_t *)model;
    int32 i;

    fwrite_int32(fh, lm->lm3g.n_prob2);
    for (i = 0; i < lm->lm3g.n_prob2; i++) {
        float32 log10val = logmath_log_to_log10(model->lmath, lm->lm3g.prob2[i].l);
        fwrite(&log10val, 4, 1, fh);
    }
}

static void
ngram_model_dmp_write_tgbowt(FILE *fh, ngram_model_t *model)
{
    ngram_model_dmp_t *lm = (ngram_model_dmp_t *)model;
    int32 i;

    fwrite_int32(fh, lm->lm3g.n_bo_wt2);
    for (i = 0; i < lm->lm3g.n_bo_wt2; i++) {
        float32 log10val = logmath_log_to_log10(model->lmath, lm->lm3g.bo_wt2[i].l);
        fwrite(&log10val, 4, 1, fh);
    }
}

static void
ngram_model_dmp_write_tgprob(FILE *fh, ngram_model_t *model)
{
    ngram_model_dmp_t *lm = (ngram_model_dmp_t *)model;
    int32 i;

    fwrite_int32(fh, lm->lm3g.n_prob3);
    for (i = 0; i < lm->lm3g.n_prob3; i++) {
        float32 log10val = logmath_log_to_log10(model->lmath, lm->lm3g.prob3[i].l);
        fwrite(&log10val, 4, 1, fh);
    }
}

static void
ngram_model_dmp_write_tg_segbase(FILE *fh, ngram_model_t *model)
{
    ngram_model_dmp_t *lm = (ngram_model_dmp_t *)model;
    int32 i, k;

    k = (model->n_counts[1] + 1) / BG_SEG_SZ + 1;
    fwrite_int32(fh, k);
    for (i = 0; i < k; i++)
        fwrite_int32(fh, lm->lm3g.tseg_base[i]);
}

static void
ngram_model_dmp_write_wordstr(FILE *fh, ngram_model_t *model)
{
    int32 i, k;

    k = 0;
    for (i = 0; i < model->n_counts[0]; i++)
        k += strlen(model->word_str[i]) + 1;
    fwrite_int32(fh, k);
    for (i = 0; i < model->n_counts[0]; i++)
        fwrite(model->word_str[i], 1,
               strlen(model->word_str[i]) + 1, fh);
}

int
ngram_model_dmp_write(ngram_model_t *base,
                      const char *file_name)
{
    ngram_model_dmp_t *model;
    ngram_model_t *newbase;
    FILE *fh;

    
    model = ngram_model_dmp_build(base);
    newbase = &model->base;

    

    if ((fh = fopen(file_name, "wb")) == NULL) {
        E_ERROR("Cannot create file %s\n", file_name);
        return -1;
    }
    ngram_model_dmp_write_header(fh);
    ngram_model_dmp_write_lm_filename(fh, file_name);
    ngram_model_dmp_write_version(fh, 0);
    ngram_model_dmp_write_fmtdesc(fh);
    ngram_model_dmp_write_ngram_counts(fh, newbase);
    ngram_model_dmp_write_unigram(fh, newbase);
    if (newbase->n > 1) {
        ngram_model_dmp_write_bigram(fh, newbase);
	if (newbase->n > 2) {
	    ngram_model_dmp_write_trigram(fh, newbase);
	}
	ngram_model_dmp_write_bgprob(fh, newbase);
	if (newbase->n > 2) {
	        ngram_model_dmp_write_tgbowt(fh, newbase);
	        ngram_model_dmp_write_tgprob(fh, newbase);
	        ngram_model_dmp_write_tg_segbase(fh, newbase);
        }
    }
    ngram_model_dmp_write_wordstr(fh, newbase);
    ngram_model_free(newbase);

    return fclose(fh);
}

static int
ngram_model_dmp_apply_weights(ngram_model_t *base, float32 lw,
                              float32 wip, float32 uw)
{
    ngram_model_dmp_t *model = (ngram_model_dmp_t *)base;
    lm3g_apply_weights(base, &model->lm3g, lw, wip, uw);
    return 0;
}




#define NGRAM_MODEL_TYPE ngram_model_dmp_t
#include "lm3g_templates.c"

static void
ngram_model_dmp_free(ngram_model_t *base)
{
    ngram_model_dmp_t *model = (ngram_model_dmp_t *)base;

    ckd_free(model->lm3g.unigrams);
    ckd_free(model->lm3g.prob2);
    if (model->dump_mmap) {
        mmio_file_unmap(model->dump_mmap);
    } 
    else {
        ckd_free(model->lm3g.bigrams);
        if (base->n > 2) {
            ckd_free(model->lm3g.trigrams);
            ckd_free(model->lm3g.tseg_base);
        }
    }
    if (base->n > 2) {
        ckd_free(model->lm3g.bo_wt2);
        ckd_free(model->lm3g.prob3);
    }

    lm3g_tginfo_free(base, &model->lm3g);
}

static ngram_funcs_t ngram_model_dmp_funcs = {
    ngram_model_dmp_free,          
    ngram_model_dmp_apply_weights, 
    lm3g_template_score,           
    lm3g_template_raw_score,       
    lm3g_template_add_ug,          
    lm3g_template_flush,           
    lm3g_template_iter,             
    lm3g_template_mgrams,          
    lm3g_template_successors,      
    lm3g_template_iter_get,        
    lm3g_template_iter_next,       
    lm3g_template_iter_free        
};
