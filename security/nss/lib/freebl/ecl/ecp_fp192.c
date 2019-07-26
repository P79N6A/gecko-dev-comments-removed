



#include "ecp_fp.h"
#include <stdlib.h>

#define ECFP_BSIZE 192
#define ECFP_NUMDOUBLES 8

#include "ecp_fpinc.c"




void
ecfp192_singleReduce(double *d, const EC_group_fp * group)
{
	double q;

	ECFP_ASSERT(group->doubleBitSize == 24);
	ECFP_ASSERT(group->primeBitSize == 192);
	ECFP_ASSERT(group->numDoubles == 8);

	q = d[ECFP_NUMDOUBLES - 1] - ecfp_beta_192;
	q += group->bitSize_alpha;
	q -= group->bitSize_alpha;

	d[ECFP_NUMDOUBLES - 1] -= q;
	d[0] += q * ecfp_twom192;
	d[2] += q * ecfp_twom128;
	ecfp_positiveTidy(d, group);
}






void
ecfp_reduce_192(double *r, double *x, const EC_group_fp * group)
{
	double x8, x9, x10, q;

	ECFP_ASSERT(group->doubleBitSize == 24);
	ECFP_ASSERT(group->primeBitSize == 192);
	ECFP_ASSERT(group->numDoubles == 8);

	
	ecfp_tidyUpper(x, group);

	x8 = x[8] + x[14] * ecfp_twom128;	
	x9 = x[9] + x[15] * ecfp_twom128;	

	

	q = x8 + group->alpha[9];
	q -= group->alpha[9];
	x8 -= q;
	x9 += q;

	q = x9 + group->alpha[10];
	q -= group->alpha[10];
	x9 -= q;
	x10 = x[10] + q;

	r[7] = x[7] + x[15] * ecfp_twom192 + x[13] * ecfp_twom128;	


	r[6] = x[6] + x[14] * ecfp_twom192 + x[12] * ecfp_twom128;
	r[5] = x[5] + x[13] * ecfp_twom192 + x[11] * ecfp_twom128;
	r[4] = x[4] + x[12] * ecfp_twom192 + x10 * ecfp_twom128;
	r[3] = x[3] + x[11] * ecfp_twom192 + x9 * ecfp_twom128;	

	r[2] = x[2] + x10 * ecfp_twom192 + x8 * ecfp_twom128;
	r[1] = x[1] + x9 * ecfp_twom192;	
	r[0] = x[0] + x8 * ecfp_twom192;

	



	q = r[ECFP_NUMDOUBLES - 2] + group->alpha[ECFP_NUMDOUBLES - 1];
	q -= group->alpha[ECFP_NUMDOUBLES - 1];
	r[ECFP_NUMDOUBLES - 2] -= q;
	r[ECFP_NUMDOUBLES - 1] += q;

	
	
	q = r[ECFP_NUMDOUBLES - 1] - ecfp_beta_192;
	q += group->bitSize_alpha;
	q -= group->bitSize_alpha;

	r[ECFP_NUMDOUBLES - 1] -= q;
	r[0] += q * ecfp_twom192;
	r[2] += q * ecfp_twom128;

	
	ecfp_tidyShort(r, group);
}


mp_err
ec_group_set_nistp192_fp(ECGroup *group)
{
	EC_group_fp *fpg;

	
	fpg = (EC_group_fp *) malloc(sizeof(EC_group_fp));
	if (fpg == NULL) {
		return MP_MEM;
	}

	fpg->numDoubles = ECFP_NUMDOUBLES;
	fpg->primeBitSize = ECFP_BSIZE;
	fpg->orderBitSize = 192;
	fpg->doubleBitSize = 24;
	fpg->numInts = (ECFP_BSIZE + ECL_BITS - 1) / ECL_BITS;
	fpg->aIsM3 = 1;
	fpg->ecfp_singleReduce = &ecfp192_singleReduce;
	fpg->ecfp_reduce = &ecfp_reduce_192;
	fpg->ecfp_tidy = &ecfp_tidy;

	fpg->pt_add_jac_aff = &ecfp192_pt_add_jac_aff;
	fpg->pt_add_jac = &ecfp192_pt_add_jac;
	fpg->pt_add_jm_chud = &ecfp192_pt_add_jm_chud;
	fpg->pt_add_chud = &ecfp192_pt_add_chud;
	fpg->pt_dbl_jac = &ecfp192_pt_dbl_jac;
	fpg->pt_dbl_jm = &ecfp192_pt_dbl_jm;
	fpg->pt_dbl_aff2chud = &ecfp192_pt_dbl_aff2chud;
	fpg->precompute_chud = &ecfp192_precompute_chud;
	fpg->precompute_jac = &ecfp192_precompute_jac;

	group->point_mul = &ec_GFp_point_mul_wNAF_fp;
	group->points_mul = &ec_pts_mul_basic;
	group->extra1 = fpg;
	group->extra_free = &ec_GFp_extra_free_fp;

	ec_set_fp_precision(fpg);
	fpg->bitSize_alpha = ECFP_TWO192 * fpg->alpha[0];

	return MP_OKAY;
}
