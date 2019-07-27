





































#include <string.h>
#include <stdio.h>
#include <assert.h>


#include <sphinxbase/bio.h>


#include "ms_senone.h"

#define MIXW_PARAM_VERSION	"1.0"
#define SPDEF_PARAM_VERSION	"1.2"

static int32
senone_mgau_map_read(senone_t * s, char const *file_name)
{
    FILE *fp;
    int32 byteswap, chksum_present, n_gauden_present;
    uint32 chksum;
    int32 i;
    char eofchk;
    char **argname, **argval;
    void *ptr;
    float32 v;

    E_INFO("Reading senone gauden-codebook map file: %s\n", file_name);

    if ((fp = fopen(file_name, "rb")) == NULL)
        E_FATAL_SYSTEM("Failed to open map file '%s' for reading", file_name);

    
    if (bio_readhdr(fp, &argname, &argval, &byteswap) < 0)
        E_FATAL("Failed to read header from file '%s'\n", file_name);

    
    chksum_present = 0;
    n_gauden_present = 0;
    for (i = 0; argname[i]; i++) {
        if (strcmp(argname[i], "version") == 0) {
            if (strcmp(argval[i], SPDEF_PARAM_VERSION) != 0) {
                E_WARN("Version mismatch(%s): %s, expecting %s\n",
                       file_name, argval[i], SPDEF_PARAM_VERSION);
            }

            
            if (sscanf(argval[i], "%f", &v) != 1)
                E_FATAL("%s: Bad version no. string: %s\n", file_name,
                        argval[i]);

            n_gauden_present = (v > 1.1) ? 1 : 0;
        }
        else if (strcmp(argname[i], "chksum0") == 0) {
            chksum_present = 1; 
        }
    }
    bio_hdrarg_free(argname, argval);
    argname = argval = NULL;

    chksum = 0;

    
    if (n_gauden_present) {
        E_INFO("Reading number of codebooks from %s\n", file_name);
        if (bio_fread
            (&(s->n_gauden), sizeof(int32), 1, fp, byteswap, &chksum) != 1)
            E_FATAL("fread(%s) (#gauden) failed\n", file_name);
    }

    
    if (bio_fread_1d(&ptr, sizeof(uint32), &(s->n_sen), fp,
		     byteswap, &chksum) < 0) {
        E_FATAL("bio_fread_1d(%s) failed\n", file_name);
    }
    s->mgau = ptr;
    E_INFO("Mapping %d senones to %d codebooks\n", s->n_sen, s->n_gauden);

    
    if (!n_gauden_present) {
        s->n_gauden = 1;
        for (i = 0; i < s->n_sen; i++)
            if (s->mgau[i] >= s->n_gauden)
                s->n_gauden = s->mgau[i] + 1;
    }

    if (chksum_present)
        bio_verify_chksum(fp, byteswap, chksum);

    if (fread(&eofchk, 1, 1, fp) == 1)
        E_FATAL("More data than expected in %s: %d\n", file_name, eofchk);

    fclose(fp);

    E_INFO("Read %d->%d senone-codebook mappings\n", s->n_sen,
           s->n_gauden);

    return 1;
}


static int32
senone_mixw_read(senone_t * s, char const *file_name, logmath_t *lmath)
{
    char eofchk;
    FILE *fp;
    int32 byteswap, chksum_present;
    uint32 chksum;
    float32 *pdf;
    int32 i, f, c, p, n_err;
    char **argname, **argval;

    E_INFO("Reading senone mixture weights: %s\n", file_name);

    if ((fp = fopen(file_name, "rb")) == NULL)
        E_FATAL_SYSTEM("Failed to open mixture weights file '%s' for reading", file_name);

    
    if (bio_readhdr(fp, &argname, &argval, &byteswap) < 0)
        E_FATAL("Failed to read header from file '%s'\n", file_name);

    
    chksum_present = 0;
    for (i = 0; argname[i]; i++) {
        if (strcmp(argname[i], "version") == 0) {
            if (strcmp(argval[i], MIXW_PARAM_VERSION) != 0)
                E_WARN("Version mismatch(%s): %s, expecting %s\n",
                       file_name, argval[i], MIXW_PARAM_VERSION);
        }
        else if (strcmp(argname[i], "chksum0") == 0) {
            chksum_present = 1; 
        }
    }
    bio_hdrarg_free(argname, argval);
    argname = argval = NULL;

    chksum = 0;

    
    if ((bio_fread(&(s->n_sen), sizeof(int32), 1, fp, byteswap, &chksum) !=
         1)
        ||
        (bio_fread(&(s->n_feat), sizeof(int32), 1, fp, byteswap, &chksum)
         != 1)
        || (bio_fread(&(s->n_cw), sizeof(int32), 1, fp, byteswap, &chksum)
            != 1)
        || (bio_fread(&i, sizeof(int32), 1, fp, byteswap, &chksum) != 1)) {
        E_FATAL("bio_fread(%s) (arraysize) failed\n", file_name);
    }
    if (i != s->n_sen * s->n_feat * s->n_cw) {
        E_FATAL
            ("%s: #float32s(%d) doesn't match dimensions: %d x %d x %d\n",
             file_name, i, s->n_sen, s->n_feat, s->n_cw);
    }

    



    if ((s->mixwfloor <= 0.0) || (s->mixwfloor >= 1.0))
        E_FATAL("mixwfloor (%e) not in range (0, 1)\n", s->mixwfloor);

    
    E_INFO("Truncating senone logs3(pdf) values by %d bits\n", SENSCR_SHIFT);

    



    if (s->n_gauden > 1) {
	E_INFO("Not transposing mixture weights in memory\n");
        s->pdf =
            (senprob_t ***) ckd_calloc_3d(s->n_sen, s->n_feat, s->n_cw,
                                          sizeof(senprob_t));
    }
    else {
	E_INFO("Transposing mixture weights in memory\n");
        s->pdf =
            (senprob_t ***) ckd_calloc_3d(s->n_feat, s->n_cw, s->n_sen,
                                          sizeof(senprob_t));
    }

    
    pdf = (float32 *) ckd_calloc(s->n_cw, sizeof(float32));

    
    n_err = 0;
    for (i = 0; i < s->n_sen; i++) {
        for (f = 0; f < s->n_feat; f++) {
            if (bio_fread
                ((void *) pdf, sizeof(float32), s->n_cw, fp, byteswap,
                 &chksum)
                != s->n_cw) {
                E_FATAL("bio_fread(%s) (arraydata) failed\n", file_name);
            }

            
            if (vector_sum_norm(pdf, s->n_cw) <= 0.0)
                n_err++;
            vector_floor(pdf, s->n_cw, s->mixwfloor);
            vector_sum_norm(pdf, s->n_cw);

            
            for (c = 0; c < s->n_cw; c++) {
                p = -(logmath_log(lmath, pdf[c]));
                p += (1 << (SENSCR_SHIFT - 1)) - 1; 

                if (s->n_gauden > 1)
                    s->pdf[i][f][c] =
                        (p < (255 << SENSCR_SHIFT)) ? (p >> SENSCR_SHIFT) : 255;
                else
                    s->pdf[f][c][i] =
                        (p < (255 << SENSCR_SHIFT)) ? (p >> SENSCR_SHIFT) : 255;
            }
        }
    }
    if (n_err > 0)
        E_WARN("Weight normalization failed for %d mixture weights components\n", n_err);

    ckd_free(pdf);

    if (chksum_present)
        bio_verify_chksum(fp, byteswap, chksum);

    if (fread(&eofchk, 1, 1, fp) == 1)
        E_FATAL("More data than expected in %s\n", file_name);

    fclose(fp);

    E_INFO
        ("Read mixture weights for %d senones: %d features x %d codewords\n",
         s->n_sen, s->n_feat, s->n_cw);

    return 1;
}


senone_t *
senone_init(gauden_t *g, char const *mixwfile, char const *sen2mgau_map_file,
	    float32 mixwfloor, logmath_t *lmath, bin_mdef_t *mdef)
{
    senone_t *s;
    int32 n = 0, i;

    s = (senone_t *) ckd_calloc(1, sizeof(senone_t));
    s->lmath = logmath_init(logmath_get_base(lmath), SENSCR_SHIFT, TRUE);
    s->mixwfloor = mixwfloor;

    s->n_gauden = g->n_mgau;
    if (sen2mgau_map_file) {
	if (!(strcmp(sen2mgau_map_file, ".semi.") == 0
	      || strcmp(sen2mgau_map_file, ".ptm.") == 0
	      || strcmp(sen2mgau_map_file, ".cont.") == 0)) {
	    senone_mgau_map_read(s, sen2mgau_map_file);
	    n = s->n_sen;
	}
    }
    else {
	if (s->n_gauden == 1)
	    sen2mgau_map_file = ".semi.";
	else if (s->n_gauden == bin_mdef_n_ciphone(mdef))
	    sen2mgau_map_file = ".ptm.";
	else
	    sen2mgau_map_file = ".cont.";
    }

    senone_mixw_read(s, mixwfile, lmath);

    if (strcmp(sen2mgau_map_file, ".semi.") == 0) {
        
	E_INFO("Mapping all senones to one codebook\n");
        s->mgau = (uint32 *) ckd_calloc(s->n_sen, sizeof(*s->mgau));
    }
    else if (strcmp(sen2mgau_map_file, ".ptm.") == 0) {
        
	E_INFO("Mapping senones to context-independent phone codebooks\n");
        s->mgau = (uint32 *) ckd_calloc(s->n_sen, sizeof(*s->mgau));
        for (i = 0; i < s->n_sen; i++)
	    s->mgau[i] = bin_mdef_sen2cimap(mdef, i);
    }
    else if (strcmp(sen2mgau_map_file, ".cont.") == 0
             || strcmp(sen2mgau_map_file, ".s3cont.") == 0) {
        
	E_INFO("Mapping senones to individual codebooks\n");
        if (s->n_sen <= 1)
            E_FATAL("#senone=%d; must be >1\n", s->n_sen);

        s->mgau = (uint32 *) ckd_calloc(s->n_sen, sizeof(*s->mgau));
        for (i = 0; i < s->n_sen; i++)
            s->mgau[i] = i;
	
        s->n_gauden = s->n_sen;
    }
    else {
        if (s->n_sen != n)
            E_FATAL("#senones inconsistent: %d in %s; %d in %s\n",
                    n, sen2mgau_map_file, s->n_sen, mixwfile);
    }

    s->featscr = NULL;
    return s;
}

void
senone_free(senone_t * s)
{
    if (s == NULL)
        return;
    if (s->pdf)
        ckd_free_3d((void *) s->pdf);
    if (s->mgau)
        ckd_free(s->mgau);
    if (s->featscr)
        ckd_free(s->featscr);
    logmath_free(s->lmath);
    ckd_free(s);
}







int32
senone_eval(senone_t * s, int id, gauden_dist_t ** dist, int32 n_top)
{
    int32 scr;                  
    int32 fden;                 
    int32 fscr;                 
    int32 fwscr;                
    int32 f, t;
    gauden_dist_t *fdist;

    assert((id >= 0) && (id < s->n_sen));
    assert((n_top > 0) && (n_top <= s->n_cw));

    scr = 0;

    for (f = 0; f < s->n_feat; f++) {
#ifdef SPHINX_DEBUG
        int top;
#endif
        fdist = dist[f];

        
#ifdef SPHINX_DEBUG
	top = 
#endif
	fden = ((int32)fdist[0].dist + ((1<<SENSCR_SHIFT) - 1)) >> SENSCR_SHIFT;
        fscr = (s->n_gauden > 1)
	    ? (fden + -s->pdf[id][f][fdist[0].id])  
	    : (fden + -s->pdf[f][fdist[0].id][id]); 
        E_DEBUG(1, ("fden[%d][%d] l+= %d + %d = %d\n",
                    id, f, -(fscr - fden), -(fden-top), -(fscr-top)));
        
        for (t = 1; t < n_top; t++) {
	    fden = ((int32)fdist[t].dist + ((1<<SENSCR_SHIFT) - 1)) >> SENSCR_SHIFT;
            fwscr = (s->n_gauden > 1) ?
                (fden + -s->pdf[id][f][fdist[t].id]) :
                (fden + -s->pdf[f][fdist[t].id][id]);
            fscr = logmath_add(s->lmath, fscr, fwscr);
            E_DEBUG(1, ("fden[%d][%d] l+= %d + %d = %d\n",
                        id, f, -(fwscr - fden), -(fden-top), -(fscr-top)));
        }
	

        scr -= fscr;
    }
    
    scr /= s->aw;

    
    if (scr > 32767)
      scr = 32767;
    if (scr < -32768)
      scr = -32768;
    return scr;
}
