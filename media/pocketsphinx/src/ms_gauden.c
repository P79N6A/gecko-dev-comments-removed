


















































































































#include <assert.h>
#include <string.h>
#include <math.h>
#include <float.h>


#include <sphinxbase/bio.h>
#include <sphinxbase/err.h>
#include <sphinxbase/ckd_alloc.h>


#include "ms_gauden.h"

#define GAUDEN_PARAM_VERSION	"1.0"

#ifndef M_PI
#define M_PI	3.1415926535897932385e0
#endif

#define WORST_DIST	(int32)(0x80000000)

void
gauden_dump(const gauden_t * g)
{
    int32 c;

    for (c = 0; c < g->n_mgau; c++)
        gauden_dump_ind(g, c);
}


void
gauden_dump_ind(const gauden_t * g, int senidx)
{
    int32 f, d, i;

    for (f = 0; f < g->n_feat; f++) {
        E_INFO("Codebook %d, Feature %d (%dx%d):\n",
               senidx, f, g->n_density, g->featlen[f]);

        for (d = 0; d < g->n_density; d++) {
            printf("m[%3d]", d);
            for (i = 0; i < g->featlen[f]; i++)
		printf(" %7.4f", MFCC2FLOAT(g->mean[senidx][f][d][i]));
            printf("\n");
        }
        printf("\n");

        for (d = 0; d < g->n_density; d++) {
            printf("v[%3d]", d);
            for (i = 0; i < g->featlen[f]; i++)
                printf(" %d", (int)g->var[senidx][f][d][i]);
            printf("\n");
        }
        printf("\n");

        for (d = 0; d < g->n_density; d++)
            printf("d[%3d] %d\n", d, (int)g->det[senidx][f][d]);
    }
    fflush(stderr);
}

static int32
gauden_param_read(float32 ***** out_param,      
                  int32 * out_n_mgau,
                  int32 * out_n_feat,
                  int32 * out_n_density,
                  int32 ** out_veclen, const char *file_name)
{
    char tmp;
    FILE *fp;
    int32 i, j, k, l, n, blk;
    int32 n_mgau;
    int32 n_feat;
    int32 n_density;
    int32 *veclen;
    int32 byteswap, chksum_present;
    float32 ****out;
    float32 *buf;
    char **argname, **argval;
    uint32 chksum;

    E_INFO("Reading mixture gaussian parameter: %s\n", file_name);

    if ((fp = fopen(file_name, "rb")) == NULL)
        E_FATAL_SYSTEM("Failed to open file '%s' for reading", file_name);

    
    if (bio_readhdr(fp, &argname, &argval, &byteswap) < 0)
        E_FATAL("Failed to read header from file '%s'\n", file_name);

    
    chksum_present = 0;
    for (i = 0; argname[i]; i++) {
        if (strcmp(argname[i], "version") == 0) {
            if (strcmp(argval[i], GAUDEN_PARAM_VERSION) != 0)
                E_WARN("Version mismatch(%s): %s, expecting %s\n",
                       file_name, argval[i], GAUDEN_PARAM_VERSION);
        }
        else if (strcmp(argname[i], "chksum0") == 0) {
            chksum_present = 1; 
        }
    }
    bio_hdrarg_free(argname, argval);
    argname = argval = NULL;

    chksum = 0;

    
    if (bio_fread(&n_mgau, sizeof(int32), 1, fp, byteswap, &chksum) != 1)
        E_FATAL("fread(%s) (#codebooks) failed\n", file_name);
    *out_n_mgau = n_mgau;

    
    if (bio_fread(&n_feat, sizeof(int32), 1, fp, byteswap, &chksum) != 1)
        E_FATAL("fread(%s) (#features) failed\n", file_name);
    *out_n_feat = n_feat;

    
    if (bio_fread(&n_density, sizeof(int32), 1, fp, byteswap, &chksum) != 1)
        E_FATAL("fread(%s) (#density/codebook) failed\n", file_name);
    *out_n_density = n_density;

    
    veclen = ckd_calloc(n_feat, sizeof(uint32));
    *out_veclen = veclen;
    if (bio_fread(veclen, sizeof(int32), n_feat, fp, byteswap, &chksum) !=
        n_feat)
        E_FATAL("fread(%s) (feature-lengths) failed\n", file_name);

    
    for (i = 0, blk = 0; i < n_feat; i++)
        blk += veclen[i];

    
    if (bio_fread(&n, sizeof(int32), 1, fp, byteswap, &chksum) != 1)
        E_FATAL("fread(%s) (total #floats) failed\n", file_name);
    if (n != n_mgau * n_density * blk) {
        E_FATAL
            ("%s: #mfcc_ts(%d) doesn't match dimensions: %d x %d x %d\n",
             file_name, n, n_mgau, n_density, blk);
    }

    
    if (!(*out_param)) {
        out = (float32 ****) ckd_calloc_3d(n_mgau, n_feat, n_density,
                                         sizeof(float32 *));
        buf = (float32 *) ckd_calloc(n, sizeof(float32));
        for (i = 0, l = 0; i < n_mgau; i++) {
            for (j = 0; j < n_feat; j++) {
                for (k = 0; k < n_density; k++) {
                    out[i][j][k] = &buf[l];
                    l += veclen[j];
                }
            }
        }
    }
    else {
        out = (float32 ****) *out_param;
        buf = out[0][0][0];
    }

    
    if (bio_fread(buf, sizeof(float32), n, fp, byteswap, &chksum) != n)
        E_FATAL("fread(%s) (densitydata) failed\n", file_name);

    if (chksum_present)
        bio_verify_chksum(fp, byteswap, chksum);

    if (fread(&tmp, 1, 1, fp) == 1)
        E_FATAL("More data than expected in %s\n", file_name);

    fclose(fp);

    *out_param = out;

    E_INFO("%d codebook, %d feature, size: \n", n_mgau, n_feat);
    for (i = 0; i < n_feat; i++)
        E_INFO(" %dx%d\n", n_density, veclen[i]);

    return 0;
}

static void
gauden_param_free(mfcc_t **** p)
{
    ckd_free(p[0][0][0]);
    ckd_free_3d(p);
}







static int32
gauden_dist_precompute(gauden_t * g, logmath_t *lmath, float32 varfloor)
{
    int32 i, m, f, d, flen;
    mfcc_t *meanp;
    mfcc_t *varp;
    mfcc_t *detp;
    int32 floored;

    floored = 0;
    
    g->det = ckd_calloc_3d(g->n_mgau, g->n_feat, g->n_density, sizeof(***g->det));

    for (m = 0; m < g->n_mgau; m++) {
        for (f = 0; f < g->n_feat; f++) {
            flen = g->featlen[f];

            
            for (d = 0, detp = g->det[m][f]; d < g->n_density; d++, detp++) {
                *detp = 0;
                for (i = 0, varp = g->var[m][f][d], meanp = g->mean[m][f][d];
                     i < flen; i++, varp++, meanp++) {
                    float32 *fvarp = (float32 *)varp;

#ifdef FIXED_POINT
                    float32 *fmp = (float32 *)meanp;
                    *meanp = FLOAT2MFCC(*fmp);
#endif
                    if (*fvarp < varfloor) {
                        *fvarp = varfloor;
                        ++floored;
                    }
                    *detp += (mfcc_t)logmath_log(lmath,
                                                 1.0 / sqrt(*fvarp * 2.0 * M_PI));
                    
                    *varp = (mfcc_t)logmath_ln_to_log(lmath,
                                                      (1.0 / (*fvarp * 2.0)));
                }
            }
        }
    }

    E_INFO("%d variance values floored\n", floored);

    return 0;
}


gauden_t *
gauden_init(char const *meanfile, char const *varfile, float32 varfloor, logmath_t *lmath)
{
    int32 i, m, f, d, *flen;
    float32 ****fgau;
    gauden_t *g;

    assert(meanfile != NULL);
    assert(varfile != NULL);
    assert(varfloor > 0.0);

    g = (gauden_t *) ckd_calloc(1, sizeof(gauden_t));
    g->lmath = lmath;

    
    fgau = NULL;
    gauden_param_read(&fgau, &g->n_mgau, &g->n_feat, &g->n_density,
                      &g->featlen, meanfile);
    g->mean = (mfcc_t ****)fgau;
    fgau = NULL;
    gauden_param_read(&fgau, &m, &f, &d, &flen, varfile);
    g->var = (mfcc_t ****)fgau;

    
    if ((m != g->n_mgau) || (f != g->n_feat) || (d != g->n_density))
        E_FATAL
            ("Mixture-gaussians dimensions for means and variances differ\n");
    for (i = 0; i < g->n_feat; i++)
        if (g->featlen[i] != flen[i])
            E_FATAL("Feature lengths for means and variances differ\n");
    ckd_free(flen);

    
    gauden_dist_precompute(g, lmath, varfloor);

    return g;
}

void
gauden_free(gauden_t * g)
{
    if (g == NULL)
        return;
    if (g->mean)
        gauden_param_free(g->mean);
    if (g->var)
        gauden_param_free(g->var);
    if (g->det)
        ckd_free_3d(g->det);
    if (g->featlen)
        ckd_free(g->featlen);
    ckd_free(g);
}


static int32
compute_dist_all(gauden_dist_t * out_dist, mfcc_t* obs, int32 featlen,
                 mfcc_t ** mean, mfcc_t ** var, mfcc_t * det,
                 int32 n_density)
{
    int32 i, d;

    for (d = 0; d < n_density; ++d) {
        mfcc_t *m;
        mfcc_t *v;
        mfcc_t dval;

        m = mean[d];
        v = var[d];
        dval = det[d];

        for (i = 0; i < featlen; i++) {
            mfcc_t diff;
#ifdef FIXED_POINT
            
            mfcc_t pdval = dval;
            diff = obs[i] - m[i];
            dval -= MFCCMUL(MFCCMUL(diff, diff), v[i]);
            if (dval > pdval) {
                dval = WORST_SCORE;
                break;
            }
#else
            diff = obs[i] - m[i];
            

            dval -= diff * diff * v[i];
#endif
        }

        out_dist[d].dist = dval;
        out_dist[d].id = d;
    }

    return 0;
}






static int32
compute_dist(gauden_dist_t * out_dist, int32 n_top,
             mfcc_t * obs, int32 featlen,
             mfcc_t ** mean, mfcc_t ** var, mfcc_t * det,
             int32 n_density)
{
    int32 i, j, d;
    gauden_dist_t *worst;

    
    if (n_top >= n_density)
        return (compute_dist_all
                (out_dist, obs, featlen, mean, var, det, n_density));

    for (i = 0; i < n_top; i++)
        out_dist[i].dist = WORST_DIST;
    worst = &(out_dist[n_top - 1]);

    for (d = 0; d < n_density; d++) {
        mfcc_t *m;
        mfcc_t *v;
        mfcc_t dval;

        m = mean[d];
        v = var[d];
        dval = det[d];

        for (i = 0; (i < featlen) && (dval >= worst->dist); i++) {
            mfcc_t diff;
#ifdef FIXED_POINT
            
            mfcc_t pdval = dval;
            diff = obs[i] - m[i];
            dval -= MFCCMUL(MFCCMUL(diff, diff), v[i]);
            if (dval > pdval) {
                dval = WORST_SCORE;
                break;
            }
#else
            diff = obs[i] - m[i];
            

            dval -= diff * diff * v[i];
#endif
        }

        if ((i < featlen) || (dval < worst->dist))     
            continue;

        
        for (i = 0; (i < n_top) && (dval < out_dist[i].dist); i++);
        assert(i < n_top);
        for (j = n_top - 1; j > i; --j)
            out_dist[j] = out_dist[j - 1];
        out_dist[i].dist = dval;
        out_dist[i].id = d;
    }

    return 0;
}







int32
gauden_dist(gauden_t * g,
            int mgau, int32 n_top, mfcc_t** obs, gauden_dist_t ** out_dist)
{
    int32 f;

    assert((n_top > 0) && (n_top <= g->n_density));

    for (f = 0; f < g->n_feat; f++) {
        compute_dist(out_dist[f], n_top,
                     obs[f], g->featlen[f],
                     g->mean[mgau][f], g->var[mgau][f], g->det[mgau][f],
                     g->n_density);
        E_DEBUG(3, ("Top CW(%d,%d) = %d %d\n", mgau, f, out_dist[f][0].id,
                    (int)out_dist[f][0].dist >> SENSCR_SHIFT));
    }

    return 0;
}

int32
gauden_mllr_transform(gauden_t *g, ps_mllr_t *mllr, cmd_ln_t *config)
{
    int32 i, m, f, d, *flen;
    float32 ****fgau;

    
    if (g->mean)
        gauden_param_free(g->mean);
    if (g->var)
        gauden_param_free(g->var);
    if (g->det)
        ckd_free_3d(g->det);
    if (g->featlen)
        ckd_free(g->featlen);
    g->mean = NULL;
    g->var = NULL;
    g->det = NULL;
    g->featlen = NULL;

    
    fgau = NULL;
    gauden_param_read(&fgau, &g->n_mgau, &g->n_feat, &g->n_density,
                      &g->featlen, cmd_ln_str_r(config, "-mean"));
    g->mean = (mfcc_t ****)fgau;
    fgau = NULL;
    gauden_param_read(&fgau, &m, &f, &d, &flen, cmd_ln_str_r(config, "-var"));
    g->var = (mfcc_t ****)fgau;

    
    if ((m != g->n_mgau) || (f != g->n_feat) || (d != g->n_density))
        E_FATAL
            ("Mixture-gaussians dimensions for means and variances differ\n");
    for (i = 0; i < g->n_feat; i++)
        if (g->featlen[i] != flen[i])
            E_FATAL("Feature lengths for means and variances differ\n");
    ckd_free(flen);

    
    for (i = 0; i < g->n_mgau; ++i) {
        for (f = 0; f < g->n_feat; ++f) {
            float64 *temp;
            temp = (float64 *) ckd_calloc(g->featlen[f], sizeof(float64));
            
            for (d = 0; d < g->n_density; d++) {
                int l;
                for (l = 0; l < g->featlen[f]; l++) {
                    temp[l] = 0.0;
                    for (m = 0; m < g->featlen[f]; m++) {
                        
                        temp[l] += mllr->A[f][0][l][m] * g->mean[i][f][d][m];
                    }
                    temp[l] += mllr->b[f][0][l];
                }

                for (l = 0; l < g->featlen[f]; l++) {
                    g->mean[i][f][d][l] = (float32) temp[l];
                    g->var[i][f][d][l] *= mllr->h[f][0][l];
                }
            }
            ckd_free(temp);
        }
    }

    

    gauden_dist_precompute(g, g->lmath, cmd_ln_float32_r(config, "-varfloor"));
    return 0;
}
