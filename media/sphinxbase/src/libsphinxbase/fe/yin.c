








































#include "sphinxbase/prim_type.h"
#include "sphinxbase/ckd_alloc.h"
#include "sphinxbase/fixpoint.h"

#include "sphinxbase/yin.h"

#include <stdio.h>
#include <string.h>

struct yin_s {
    uint16 frame_size;       
#ifndef FIXED_POINT
    float search_threshold; 
    float search_range;     
#else
    uint16 search_threshold; 
    uint16 search_range;     
#endif
    uint16 nfr;              

    unsigned char wsize;    
    unsigned char wstart;   
    unsigned char wcur;     
    unsigned char endut;    

#ifndef FIXED_POINT
    float **diff_window;    
#else
    fixed32 **diff_window;  
#endif
    uint16 *period_window;  
    int16 *frame;           
};




#ifndef FIXED_POINT
static void
cmn_diff(int16 const *signal, float *out_diff, int ndiff)
{
    double cum;
    int t, j;

    cum = 0.0f;
    out_diff[0] = 1.0f;

    for (t = 1; t < ndiff; ++t) {
        float dd;
        dd = 0.0f;
        for (j = 0; j < ndiff; ++j) {
             int diff = signal[j] - signal[t + j];
             dd += (diff * diff);
        }
        cum += dd;
        out_diff[t] = (float)(dd * t / cum);
    }
}
#else
static void
cmn_diff(int16 const *signal, int32 *out_diff, int ndiff)
{
    uint32 cum, cshift;
    int32 t, tscale;

    out_diff[0] = 32768;
    cum = 0;
    cshift = 0;

    
    for (tscale = 0; tscale < 32; ++tscale)
        if (ndiff & (1<<(31-tscale)))
            break;
    --tscale; 
    


    

    for (t = 1; t < ndiff; ++t) {
        uint32 dd, dshift, norm;
        int j;

        dd = 0;
        dshift = 0;
        for (j = 0; j < ndiff; ++j) {
            int diff = signal[j] - signal[t + j];
            
            if (dd > (1UL<<tscale)) {
                dd >>= 1;
                ++dshift;
            }
            dd += (diff * diff) >> dshift;
        }
        

        if (dshift > cshift) {
            cum += dd << (dshift-cshift);
        }
        else {
            cum += dd >> (cshift-dshift);
        }

        
        while (cum > (1UL<<tscale)) {
            cum >>= 1;
            ++cshift;
        }
        
        if (cum == 0) cum = 1;
        
        norm = (t << tscale) / cum;
        
        out_diff[t] = (int32)(((long long)dd * norm)
                              >> (tscale - 15 + cshift - dshift));
        

    }
}
#endif

yin_t *
yin_init(int frame_size, float search_threshold,
         float search_range, int smooth_window)
{
    yin_t *pe;

    pe = ckd_calloc(1, sizeof(*pe));
    pe->frame_size = frame_size;
#ifndef FIXED_POINT
    pe->search_threshold = search_threshold;
    pe->search_range = search_range;
#else
    pe->search_threshold = (uint16)(search_threshold * 32768);
    pe->search_range = (uint16)(search_range * 32768);
#endif
    pe->wsize = smooth_window * 2 + 1;
    pe->diff_window = ckd_calloc_2d(pe->wsize,
                                    pe->frame_size / 2,
                                    sizeof(**pe->diff_window));
    pe->period_window = ckd_calloc(pe->wsize,
                                   sizeof(*pe->period_window));
    pe->frame = ckd_calloc(pe->frame_size, sizeof(*pe->frame));
    return pe;
}

void
yin_free(yin_t *pe)
{
    ckd_free_2d(pe->diff_window);
    ckd_free(pe->period_window);
    ckd_free(pe);
}

void
yin_start(yin_t *pe)
{
    
    pe->wstart = pe->endut = 0;
    pe->nfr = 0;
}

void
yin_end(yin_t *pe)
{
    pe->endut = 1;
}

int
#ifndef FIXED_POINT
thresholded_search(float *diff_window, float threshold, int start, int end)
#else
thresholded_search(int32 *diff_window, fixed32 threshold, int start, int end)
#endif
{
    int i, argmin;
#ifndef FIXED_POINT
    float min;
#else
    int min;
#endif

    min = diff_window[start];
    argmin = start;
    for (i = start + 1; i < end; ++i) {
#ifndef FIXED_POINT
        float diff = diff_window[i];
#else
        int diff = diff_window[i];
#endif

        if (diff < threshold) {
            min = diff;
            argmin = i;
            break;
        }
        if (diff < min) {
            min = diff;
            argmin = i;
        }
    }
    return argmin;
}

void 
yin_store(yin_t *pe, int16 const *frame)
{
    memcpy(pe->frame, frame, pe->frame_size * sizeof(*pe->frame));
}

void
yin_write(yin_t *pe, int16 const *frame)
{
    int outptr, difflen;

    
    ++pe->wstart;
    
    outptr = pe->wstart - 1;
    
    if (pe->wstart == pe->wsize)
        pe->wstart = 0;

    
    difflen = pe->frame_size / 2;
    cmn_diff(frame, pe->diff_window[outptr], difflen);

    

    pe->period_window[outptr]
        = thresholded_search(pe->diff_window[outptr],
                             pe->search_threshold, 0, difflen);

    
    ++pe->nfr;
}

void 
yin_write_stored(yin_t *pe)
{
    yin_write(pe, pe->frame);
}

int
yin_read(yin_t *pe, uint16 *out_period, float *out_bestdiff)
{
    int wstart, wlen, half_wsize, i;
    int best, search_width, low_period, high_period;
#ifndef FIXED_POINT
    float best_diff;
#else
    int best_diff;
#endif

    half_wsize = (pe->wsize-1)/2;
    

    if (half_wsize == 0) {
        if (pe->endut)
            return 0;
        *out_period = pe->period_window[0];
#ifndef FIXED_POINT
        *out_bestdiff = pe->diff_window[0][pe->period_window[0]];
#else
        *out_bestdiff = pe->diff_window[0][pe->period_window[0]] / 32768.0f;
#endif
        return 1;
    }

    

    if (pe->endut == 0 && pe->nfr < half_wsize + 1) {
        
        return 0;
    }

    
    
    if (pe->endut) {
        
        if (pe->wcur == pe->wstart)
            return 0;
        
        wstart = (pe->wcur + pe->wsize - half_wsize) % pe->wsize;
        
        wlen = pe->wstart - wstart;
        if (wlen < 0) wlen += pe->wsize;
        
    }
    
    else if (pe->nfr < pe->wsize) {
        wstart = 0;
        wlen = pe->nfr;
    }
    
    else {
        wstart = pe->wstart;
        wlen = pe->wsize;
    }

    
    

    best = pe->period_window[pe->wcur];
    best_diff = pe->diff_window[pe->wcur][best];
    for (i = 0; i < wlen; ++i) {
        int j = wstart + i;
#ifndef FIXED_POINT
        float diff;
#else
        int diff;
#endif

        j %= pe->wsize;
        diff = pe->diff_window[j][pe->period_window[j]];
        

        if (diff < best_diff) {
            best_diff = diff;
            best = pe->period_window[j];
        }
    }
    

    
    if (best == pe->period_window[pe->wcur]) {
        
        if (++pe->wcur == pe->wsize)
            pe->wcur = 0;
        *out_period = best;
#ifndef FIXED_POINT
        *out_bestdiff = best_diff;
#else
        *out_bestdiff = best_diff / 32768.0f;
#endif
        return 1;
    }
    
#ifndef FIXED_POINT
    search_width = (int)(best * pe->search_range);
#else
    search_width = best * pe->search_range / 32768;
#endif
    

    if (search_width == 0) search_width = 1;
    low_period = best - search_width;
    high_period = best + search_width;
    if (low_period < 0) low_period = 0;
    if (high_period > pe->frame_size / 2) high_period = pe->frame_size / 2;
    
    best = thresholded_search(pe->diff_window[pe->wcur],
                              pe->search_threshold,
                              low_period, high_period);
    best_diff = pe->diff_window[pe->wcur][best];

    if (out_period)
        *out_period = (best > 65535) ? 65535 : best;
    if (out_bestdiff) {
#ifndef FIXED_POINT
        *out_bestdiff = (best_diff > 1.0f) ? 1.0f : best_diff;
#else
        *out_bestdiff = (best_diff > 32768) ? 1.0f : best_diff / 32768.0f;
#endif
    }

    
    if (++pe->wcur == pe->wsize)
        pe->wcur = 0;
    return 1;
}
