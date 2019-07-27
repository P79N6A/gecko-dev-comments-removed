















































#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#ifdef _MSC_VER
#pragma warning (disable: 4244)
#endif

#include "sphinxbase/ckd_alloc.h"
#include "sphinxbase/err.h"
#include "sphinxbase/cmn.h"

void
cmn_prior_set(cmn_t *cmn, mfcc_t const * vec)
{
    int32 i;

    E_INFO("cmn_prior_set: from < ");
    for (i = 0; i < cmn->veclen; i++)
        E_INFOCONT("%5.2f ", MFCC2FLOAT(cmn->cmn_mean[i]));
    E_INFOCONT(">\n");

    for (i = 0; i < cmn->veclen; i++) {
        cmn->cmn_mean[i] = vec[i];
        cmn->sum[i] = vec[i] * CMN_WIN;
    }
    cmn->nframe = CMN_WIN;

    E_INFO("cmn_prior_set: to   < ");
    for (i = 0; i < cmn->veclen; i++)
        E_INFOCONT("%5.2f ", MFCC2FLOAT(cmn->cmn_mean[i]));
    E_INFOCONT(">\n");
}

void
cmn_prior_get(cmn_t *cmn, mfcc_t * vec)
{
    int32 i;

    for (i = 0; i < cmn->veclen; i++)
        vec[i] = cmn->cmn_mean[i];

}

static void
cmn_prior_shiftwin(cmn_t *cmn)
{
    mfcc_t sf;
    int32 i;

    E_INFO("cmn_prior_update: from < ");
    for (i = 0; i < cmn->veclen; i++)
        E_INFOCONT("%5.2f ", MFCC2FLOAT(cmn->cmn_mean[i]));
    E_INFOCONT(">\n");

    sf = FLOAT2MFCC(1.0) / cmn->nframe;
    for (i = 0; i < cmn->veclen; i++)
        cmn->cmn_mean[i] = cmn->sum[i] / cmn->nframe; 

    
    if (cmn->nframe >= CMN_WIN_HWM) {
        sf = CMN_WIN * sf;
        for (i = 0; i < cmn->veclen; i++)
            cmn->sum[i] = MFCCMUL(cmn->sum[i], sf);
        cmn->nframe = CMN_WIN;
    }

    E_INFO("cmn_prior_update: to   < ");
    for (i = 0; i < cmn->veclen; i++)
        E_INFOCONT("%5.2f ", MFCC2FLOAT(cmn->cmn_mean[i]));
    E_INFOCONT(">\n");
}

void
cmn_prior_update(cmn_t *cmn)
{
    mfcc_t sf;
    int32 i;

    if (cmn->nframe <= 0)
        return;

    E_INFO("cmn_prior_update: from < ");
    for (i = 0; i < cmn->veclen; i++)
        E_INFOCONT("%5.2f ", MFCC2FLOAT(cmn->cmn_mean[i]));
    E_INFOCONT(">\n");

    
    sf = FLOAT2MFCC(1.0) / cmn->nframe;
    for (i = 0; i < cmn->veclen; i++)
        cmn->cmn_mean[i] = cmn->sum[i] / cmn->nframe; 

    
    if (cmn->nframe > CMN_WIN_HWM) {
        sf = CMN_WIN * sf;
        for (i = 0; i < cmn->veclen; i++)
            cmn->sum[i] = MFCCMUL(cmn->sum[i], sf);
        cmn->nframe = CMN_WIN;
    }

    E_INFO("cmn_prior_update: to   < ");
    for (i = 0; i < cmn->veclen; i++)
        E_INFOCONT("%5.2f ", MFCC2FLOAT(cmn->cmn_mean[i]));
    E_INFOCONT(">\n");
}

void
cmn_prior(cmn_t *cmn, mfcc_t **incep, int32 varnorm, int32 nfr)
{
    int32 i, j;

    if (nfr <= 0)
        return;

    if (varnorm)
        E_FATAL
            ("Variance normalization not implemented in live mode decode\n");

    for (i = 0; i < nfr; i++) {

	
	if (incep[i][0] < 0)
	    continue;

        for (j = 0; j < cmn->veclen; j++) {
            cmn->sum[j] += incep[i][j];
            incep[i][j] -= cmn->cmn_mean[j];
        }

        ++cmn->nframe;
    }

    
    if (cmn->nframe > CMN_WIN_HWM)
        cmn_prior_shiftwin(cmn);
}
