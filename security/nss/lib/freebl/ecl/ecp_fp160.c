



#include "ecp_fp.h"
#include <stdlib.h>

#define ECFP_BSIZE 160
#define ECFP_NUMDOUBLES 7

#include "ecp_fpinc.c"





void
ecfp160_singleReduce(double *d, const EC_group_fp * group)
{
	double q;

	ECFP_ASSERT(group->doubleBitSize == 24);
	ECFP_ASSERT(group->primeBitSize == 160);
	ECFP_ASSERT(ECFP_NUMDOUBLES == 7);

	q = d[ECFP_NUMDOUBLES - 1] - ecfp_beta_160;
	q += group->bitSize_alpha;
	q -= group->bitSize_alpha;

	d[ECFP_NUMDOUBLES - 1] -= q;
	d[0] += q * ecfp_twom160;
	d[1] += q * ecfp_twom129;
	ecfp_positiveTidy(d, group);

	
	ECFP_ASSERT(d[ECFP_NUMDOUBLES - 1] / ecfp_exp[ECFP_NUMDOUBLES - 1] ==
				(unsigned long long) (d[ECFP_NUMDOUBLES - 1] /
									  ecfp_exp[ECFP_NUMDOUBLES - 1]));
	ECFP_ASSERT(d[ECFP_NUMDOUBLES - 1] >= 0);
}






void
ecfp160_reduce(double *r, double *x, const EC_group_fp * group)
{

	double x7, x8, q;

	ECFP_ASSERT(group->doubleBitSize == 24);
	ECFP_ASSERT(group->primeBitSize == 160);
	ECFP_ASSERT(ECFP_NUMDOUBLES == 7);

	
	ecfp_tidyUpper(x, group);

	

	x7 = x[7] + x[13] * ecfp_twom129;	

	
	q = x7 + group->alpha[8];
	q -= group->alpha[8];
	x7 -= q;					
	x8 = x[8] + q;				

	r[6] = x[6] + x[13] * ecfp_twom160 + x[12] * ecfp_twom129;	


	r[5] = x[5] + x[12] * ecfp_twom160 + x[11] * ecfp_twom129;
	r[4] = x[4] + x[11] * ecfp_twom160 + x[10] * ecfp_twom129;
	r[3] = x[3] + x[10] * ecfp_twom160 + x[9] * ecfp_twom129;
	r[2] = x[2] + x[9] * ecfp_twom160 + x8 * ecfp_twom129;	

	r[1] = x[1] + x8 * ecfp_twom160 + x7 * ecfp_twom129;	

	r[0] = x[0] + x7 * ecfp_twom160;

	


	q = r[ECFP_NUMDOUBLES - 2] + group->alpha[ECFP_NUMDOUBLES - 1];
	q -= group->alpha[ECFP_NUMDOUBLES - 1];
	r[ECFP_NUMDOUBLES - 2] -= q;
	r[ECFP_NUMDOUBLES - 1] += q;

	
	
	q = r[ECFP_NUMDOUBLES - 1] - ecfp_beta_160;
	q += group->bitSize_alpha;
	q -= group->bitSize_alpha;

	r[ECFP_NUMDOUBLES - 1] -= q;
	r[0] += q * ecfp_twom160;
	r[1] += q * ecfp_twom129;

	
	ecfp_tidyShort(r, group);
}


mp_err
ec_group_set_secp160r1_fp(ECGroup *group)
{

	EC_group_fp *fpg = NULL;

	
	fpg = (EC_group_fp *) malloc(sizeof(EC_group_fp));
	if (fpg == NULL) {
		return MP_MEM;
	}

	fpg->numDoubles = ECFP_NUMDOUBLES;
	fpg->primeBitSize = ECFP_BSIZE;
	fpg->orderBitSize = 161;
	fpg->doubleBitSize = 24;
	fpg->numInts = (ECFP_BSIZE + ECL_BITS - 1) / ECL_BITS;
	fpg->aIsM3 = 1;
	fpg->ecfp_singleReduce = &ecfp160_singleReduce;
	fpg->ecfp_reduce = &ecfp160_reduce;
	fpg->ecfp_tidy = &ecfp_tidy;

	fpg->pt_add_jac_aff = &ecfp160_pt_add_jac_aff;
	fpg->pt_add_jac = &ecfp160_pt_add_jac;
	fpg->pt_add_jm_chud = &ecfp160_pt_add_jm_chud;
	fpg->pt_add_chud = &ecfp160_pt_add_chud;
	fpg->pt_dbl_jac = &ecfp160_pt_dbl_jac;
	fpg->pt_dbl_jm = &ecfp160_pt_dbl_jm;
	fpg->pt_dbl_aff2chud = &ecfp160_pt_dbl_aff2chud;
	fpg->precompute_chud = &ecfp160_precompute_chud;
	fpg->precompute_jac = &ecfp160_precompute_jac;

	group->point_mul = &ec_GFp_point_mul_wNAF_fp;
	group->points_mul = &ec_pts_mul_basic;
	group->extra1 = fpg;
	group->extra_free = &ec_GFp_extra_free_fp;

	ec_set_fp_precision(fpg);
	fpg->bitSize_alpha = ECFP_TWO160 * fpg->alpha[0];
	return MP_OKAY;
}
