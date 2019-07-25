








































#include "mpi.h"
#include "mplogic.h"
#include "mpi-priv.h"
#include "ecl-priv.h"
#include "ecp.h"
#include <stdlib.h>
#include <stdio.h>



GFMethod *
GFMethod_consGFp_mont(const mp_int *irr)
{
	mp_err res = MP_OKAY;
	GFMethod *meth = NULL;
	mp_mont_modulus *mmm;

	meth = GFMethod_consGFp(irr);
	if (meth == NULL)
		return NULL;

	mmm = (mp_mont_modulus *) malloc(sizeof(mp_mont_modulus));
	if (mmm == NULL) {
		res = MP_MEM;
		goto CLEANUP;
	}

	meth->field_mul = &ec_GFp_mul_mont;
	meth->field_sqr = &ec_GFp_sqr_mont;
	meth->field_div = &ec_GFp_div_mont;
	meth->field_enc = &ec_GFp_enc_mont;
	meth->field_dec = &ec_GFp_dec_mont;
	meth->extra1 = mmm;
	meth->extra2 = NULL;
	meth->extra_free = &ec_GFp_extra_free_mont;

	mmm->N = meth->irr;
	mmm->n0prime = 0 - s_mp_invmod_radix(MP_DIGIT(&meth->irr, 0));

  CLEANUP:
	if (res != MP_OKAY) {
		GFMethod_free(meth);
		return NULL;
	}
	return meth;
}




mp_err
ec_GFp_mul_mont(const mp_int *a, const mp_int *b, mp_int *r,
				const GFMethod *meth)
{
	mp_err res = MP_OKAY;

#ifdef MP_MONT_USE_MP_MUL
	


	MP_CHECKOK(mp_mul(a, b, r));
	MP_CHECKOK(s_mp_redc(r, (mp_mont_modulus *) meth->extra1));
#else
	mp_int s;

	MP_DIGITS(&s) = 0;
	
	if ((a == r) || (b == r)) {
		MP_CHECKOK(mp_init(&s));
		MP_CHECKOK(s_mp_mul_mont
				   (a, b, &s, (mp_mont_modulus *) meth->extra1));
		MP_CHECKOK(mp_copy(&s, r));
		mp_clear(&s);
	} else {
		return s_mp_mul_mont(a, b, r, (mp_mont_modulus *) meth->extra1);
	}
#endif
  CLEANUP:
	return res;
}


mp_err
ec_GFp_sqr_mont(const mp_int *a, mp_int *r, const GFMethod *meth)
{
	return ec_GFp_mul_mont(a, a, r, meth);
}


mp_err
ec_GFp_div_mont(const mp_int *a, const mp_int *b, mp_int *r,
				const GFMethod *meth)
{
	mp_err res = MP_OKAY;

	



	MP_CHECKOK(ec_GFp_div(a, b, r, meth));
	MP_CHECKOK(ec_GFp_enc_mont(r, r, meth));
	if (a == NULL) {
		MP_CHECKOK(ec_GFp_enc_mont(r, r, meth));
	}
  CLEANUP:
	return res;
}



mp_err
ec_GFp_enc_mont(const mp_int *a, mp_int *r, const GFMethod *meth)
{
	mp_mont_modulus *mmm;
	mp_err res = MP_OKAY;

	mmm = (mp_mont_modulus *) meth->extra1;
	MP_CHECKOK(mp_copy(a, r));
	MP_CHECKOK(s_mp_lshd(r, MP_USED(&mmm->N)));
	MP_CHECKOK(mp_mod(r, &mmm->N, r));
  CLEANUP:
	return res;
}


mp_err
ec_GFp_dec_mont(const mp_int *a, mp_int *r, const GFMethod *meth)
{
	mp_err res = MP_OKAY;

	if (a != r) {
		MP_CHECKOK(mp_copy(a, r));
	}
	MP_CHECKOK(s_mp_redc(r, (mp_mont_modulus *) meth->extra1));
  CLEANUP:
	return res;
}



void
ec_GFp_extra_free_mont(GFMethod *meth)
{
	if (meth->extra1 != NULL) {
		free(meth->extra1);
		meth->extra1 = NULL;
	}
}
