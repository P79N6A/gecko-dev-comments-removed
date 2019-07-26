



#include "ecp_fp.h"
#include "ecl-priv.h"
#include <stdlib.h>



void
ecfp_tidyShort(double *t, const EC_group_fp * group)
{
	group->ecfp_tidy(t, group->alpha, group);
}




void
ecfp_tidyUpper(double *t, const EC_group_fp * group)
{
	group->ecfp_tidy(t + group->numDoubles,
					 group->alpha + group->numDoubles, group);
}






void
ecfp_tidy(double *t, const double *alpha, const EC_group_fp * group)
{
	double q;
	int i;

	
	for (i = 0; i < group->numDoubles - 1; i++) {
		q = t[i] + alpha[i + 1];
		q -= alpha[i + 1];
		t[i] -= q;
		t[i + 1] += q;

		


	}
}




void
ecfp_positiveTidy(double *t, const EC_group_fp * group)
{
	double q;
	int i;

	
	for (i = 0; i < group->numDoubles - 1; i++) {
		
		q = t[i] - ecfp_beta[i + 1];
		q += group->alpha[i + 1];
		q -= group->alpha[i + 1];
		t[i] -= q;
		t[i + 1] += q;

		

		ECFP_ASSERT(t[i] / ecfp_exp[i] ==
					(unsigned long long) (t[i] / ecfp_exp[i]));
		ECFP_ASSERT(t[i] >= 0);
	}
}



void
ecfp_fp2i(mp_int *mpout, double *d, const ECGroup *ecgroup)
{
	EC_group_fp *group = (EC_group_fp *) ecgroup->extra1;
	unsigned short i16[(group->primeBitSize + 15) / 16];
	double q = 1;

#ifdef ECL_THIRTY_TWO_BIT
	
	unsigned int z = 0;
#else
	uint64_t z = 0;
#endif
	int zBits = 0;
	int copiedBits = 0;
	int i = 0;
	int j = 0;

	mp_digit *out;

	
	MP_SIGN(mpout) = MP_ZPOS;

	
	ecfp_positiveTidy(d, group);

	



	do {
		
		z = 0;
		zBits = 0;
		q = 1;
		i = 0;
		j = 0;
		copiedBits = 0;

		
		group->ecfp_singleReduce(d, group);

		
		s_mp_grow(mpout, group->numInts);
		MP_USED(mpout) = group->numInts;
		out = MP_DIGITS(mpout);

		
		while (copiedBits < group->primeBitSize) {
			if (zBits < 16) {
				z += d[i] * q;
				i++;
				ECFP_ASSERT(i < (group->primeBitSize + 15) / 16);
				zBits += group->doubleBitSize;
			}
			i16[j] = z;
			j++;
			z >>= 16;
			zBits -= 16;
			q *= ecfp_twom16;
			copiedBits += 16;
		}
	} while (z != 0);

	
#ifdef ECL_THIRTY_TWO_BIT
	for (i = 0; i < (group->primeBitSize + 15) / 16; i += 2) {
		*out = 0;
		if (i + 1 < (group->primeBitSize + 15) / 16) {
			*out = i16[i + 1];
			*out <<= 16;
		}
		*out++ += i16[i];
	}
#else							
	for (i = 0; i < (group->primeBitSize + 15) / 16; i += 4) {
		*out = 0;
		if (i + 3 < (group->primeBitSize + 15) / 16) {
			*out = i16[i + 3];
			*out <<= 16;
		}
		if (i + 2 < (group->primeBitSize + 15) / 16) {
			*out += i16[i + 2];
			*out <<= 16;
		}
		if (i + 1 < (group->primeBitSize + 15) / 16) {
			*out += i16[i + 1];
			*out <<= 16;
		}
		*out++ += i16[i];
	}
#endif

	



	if (mp_cmp(mpout, &ecgroup->meth->irr) >= 0) {
		mp_sub(mpout, &ecgroup->meth->irr, mpout);
	}

	

	out = MP_DIGITS(mpout);
	for (i = group->numInts - 1; i > 0; i--) {
		if (out[i] != 0)
			break;
	}
	MP_USED(mpout) = i + 1;

	
	ECFP_ASSERT(mp_cmp(mpout, &ecgroup->meth->irr) < 0);
	ECFP_ASSERT(mp_cmp_z(mpout) >= 0);
}


void
ecfp_i2fp(double *out, const mp_int *x, const ECGroup *ecgroup)
{
	int i;
	int j = 0;
	int size;
	double shift = 1;
	mp_digit *in;
	EC_group_fp *group = (EC_group_fp *) ecgroup->extra1;

#ifdef ECL_DEBUG
	

	mp_int cmp;

	MP_DIGITS(&cmp) = NULL;
	mp_init(&cmp);
#endif

	ECFP_ASSERT(group != NULL);

	
	for (i = 0; i < group->numDoubles; i++)
		out[i] = 0;
	i = 0;

	size = MP_USED(x);
	in = MP_DIGITS(x);

	
#ifdef ECL_THIRTY_TWO_BIT
	while (j < size) {
		while (group->doubleBitSize * (i + 1) <= 32 * j) {
			i++;
		}
		ECFP_ASSERT(group->doubleBitSize * i <= 32 * j);
		out[i] = in[j];
		out[i] *= shift;
		shift *= ecfp_two32;
		j++;
	}
#else
	while (j < size) {
		while (group->doubleBitSize * (i + 1) <= 64 * j) {
			i++;
		}
		ECFP_ASSERT(group->doubleBitSize * i <= 64 * j);
		out[i] = (in[j] & 0x00000000FFFFFFFF) * shift;

		while (group->doubleBitSize * (i + 1) <= 64 * j + 32) {
			i++;
		}
		ECFP_ASSERT(24 * i <= 64 * j + 32);
		out[i] = (in[j] & 0xFFFFFFFF00000000) * shift;

		shift *= ecfp_two64;
		j++;
	}
#endif
	
	ecfp_tidyShort(out, group);

#ifdef ECL_DEBUG
	
	ecfp_fp2i(&cmp, out, ecgroup);
	ECFP_ASSERT(mp_cmp(&cmp, x) == 0);
	mp_clear(&cmp);
#endif
}





mp_err
ec_GFp_point_mul_jac_4w_fp(const mp_int *n, const mp_int *px,
						   const mp_int *py, mp_int *rx, mp_int *ry,
						   const ECGroup *ecgroup)
{
	mp_err res = MP_OKAY;
	ecfp_jac_pt precomp[16], r;
	ecfp_aff_pt p;
	EC_group_fp *group;

	mp_int rz;
	int i, ni, d;

	ARGCHK(ecgroup != NULL, MP_BADARG);
	ARGCHK((n != NULL) && (px != NULL) && (py != NULL), MP_BADARG);

	group = (EC_group_fp *) ecgroup->extra1;
	MP_DIGITS(&rz) = 0;
	MP_CHECKOK(mp_init(&rz));

	
	ecfp_i2fp(p.x, px, ecgroup);
	ecfp_i2fp(p.y, py, ecgroup);
	ecfp_i2fp(group->curvea, &ecgroup->curvea, ecgroup);

	
	group->precompute_jac(precomp, &p, group);

	
	d = (mpl_significant_bits(n) + 3) / 4;

	
	for (i = 0; i < group->numDoubles; i++) {
		r.z[i] = 0;
	}

	for (i = d - 1; i >= 0; i--) {
		
		ni = MP_GET_BIT(n, 4 * i + 3);
		ni <<= 1;
		ni |= MP_GET_BIT(n, 4 * i + 2);
		ni <<= 1;
		ni |= MP_GET_BIT(n, 4 * i + 1);
		ni <<= 1;
		ni |= MP_GET_BIT(n, 4 * i);

		
		group->pt_dbl_jac(&r, &r, group);
		group->pt_dbl_jac(&r, &r, group);
		group->pt_dbl_jac(&r, &r, group);
		group->pt_dbl_jac(&r, &r, group);

		
		group->pt_add_jac(&r, &precomp[ni], &r, group);
	}

	
	ecfp_fp2i(rx, r.x, ecgroup);
	ecfp_fp2i(ry, r.y, ecgroup);
	ecfp_fp2i(&rz, r.z, ecgroup);

	
	MP_CHECKOK(ec_GFp_pt_jac2aff(rx, ry, &rz, rx, ry, ecgroup));

  CLEANUP:
	mp_clear(&rz);
	return res;
}













mp_err
ec_GFp_pt_mul_jac_fp(const mp_int *n, const mp_int *px, const mp_int *py,
					 mp_int *rx, mp_int *ry, const ECGroup *ecgroup)
{
	mp_err res;
	mp_int sx, sy, sz;

	ecfp_aff_pt p;
	ecfp_jac_pt r;
	EC_group_fp *group = (EC_group_fp *) ecgroup->extra1;

	int i, l;

	MP_DIGITS(&sx) = 0;
	MP_DIGITS(&sy) = 0;
	MP_DIGITS(&sz) = 0;
	MP_CHECKOK(mp_init(&sx));
	MP_CHECKOK(mp_init(&sy));
	MP_CHECKOK(mp_init(&sz));

	
	if (mp_cmp_z(n) == 0) {
		mp_zero(rx);
		mp_zero(ry);
		res = MP_OKAY;
		goto CLEANUP;
		
	} else if (mp_cmp_z(n) < 0) {
		res = MP_RANGE;
		goto CLEANUP;
	}

	
	ecfp_i2fp(p.x, px, ecgroup);
	ecfp_i2fp(p.y, py, ecgroup);
	ecfp_i2fp(group->curvea, &(ecgroup->curvea), ecgroup);

	
	for (i = 0; i < group->numDoubles; i++) {
		r.z[i] = 0;
	}

	
	l = mpl_significant_bits(n) - 1;

	for (i = l; i >= 0; i--) {
		
		group->pt_dbl_jac(&r, &r, group);

		
		if (MP_GET_BIT(n, i) != 0) {
			group->pt_add_jac_aff(&r, &p, &r, group);
		}
	}

	
	ecfp_fp2i(&sx, r.x, ecgroup);
	ecfp_fp2i(&sy, r.y, ecgroup);
	ecfp_fp2i(&sz, r.z, ecgroup);

	
	MP_CHECKOK(ec_GFp_pt_jac2aff(&sx, &sy, &sz, rx, ry, ecgroup));

  CLEANUP:
	mp_clear(&sx);
	mp_clear(&sy);
	mp_clear(&sz);
	return res;
}







mp_err
ec_GFp_point_mul_wNAF_fp(const mp_int *n, const mp_int *px,
						 const mp_int *py, mp_int *rx, mp_int *ry,
						 const ECGroup *ecgroup)
{
	mp_err res = MP_OKAY;
	mp_int sx, sy, sz;
	EC_group_fp *group = (EC_group_fp *) ecgroup->extra1;
	ecfp_chud_pt precomp[16];

	ecfp_aff_pt p;
	ecfp_jm_pt r;

	signed char naf[group->orderBitSize + 1];
	int i;

	MP_DIGITS(&sx) = 0;
	MP_DIGITS(&sy) = 0;
	MP_DIGITS(&sz) = 0;
	MP_CHECKOK(mp_init(&sx));
	MP_CHECKOK(mp_init(&sy));
	MP_CHECKOK(mp_init(&sz));

	
	if (mp_cmp_z(n) == 0) {
		mp_zero(rx);
		mp_zero(ry);
		res = MP_OKAY;
		goto CLEANUP;
		
	} else if (mp_cmp_z(n) < 0) {
		res = MP_RANGE;
		goto CLEANUP;
	}

	
	ecfp_i2fp(p.x, px, ecgroup);
	ecfp_i2fp(p.y, py, ecgroup);
	ecfp_i2fp(group->curvea, &(ecgroup->curvea), ecgroup);

	
	group->precompute_chud(precomp, &p, group);

	
	ec_compute_wNAF(naf, group->orderBitSize, n, 5);

	
	for (i = 0; i < group->numDoubles; i++) {
		r.z[i] = 0;
	}

	
	for (i = group->orderBitSize; i >= 0; i--) {
		
		group->pt_dbl_jm(&r, &r, group);

		if (naf[i] != 0) {
			group->pt_add_jm_chud(&r, &precomp[(naf[i] + 15) / 2], &r,
								  group);
		}
	}

	
	ecfp_fp2i(&sx, r.x, ecgroup);
	ecfp_fp2i(&sy, r.y, ecgroup);
	ecfp_fp2i(&sz, r.z, ecgroup);

	
	MP_CHECKOK(ec_GFp_pt_jac2aff(&sx, &sy, &sz, rx, ry, ecgroup));

  CLEANUP:
	mp_clear(&sx);
	mp_clear(&sy);
	mp_clear(&sz);
	return res;
}


void
ec_GFp_extra_free_fp(ECGroup *group)
{
	if (group->extra1 != NULL) {
		free(group->extra1);
		group->extra1 = NULL;
	}
}





int
ec_set_fp_precision(EC_group_fp * group)
{
	double a = 9007199254740992.0;	
	double b = a + 1;

	if (a == b) {
		group->fpPrecision = 53;
		group->alpha = ecfp_alpha_53;
		return 53;
	}
	group->fpPrecision = 64;
	group->alpha = ecfp_alpha_64;
	return 64;
}
