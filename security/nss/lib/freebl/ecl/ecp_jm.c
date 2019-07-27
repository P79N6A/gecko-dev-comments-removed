



#include "ecp.h"
#include "ecl-priv.h"
#include "mplogic.h"
#include <stdlib.h>

#define MAX_SCRATCH 6








mp_err
ec_GFp_pt_dbl_jm(const mp_int *px, const mp_int *py, const mp_int *pz,
				 const mp_int *paz4, mp_int *rx, mp_int *ry, mp_int *rz,
				 mp_int *raz4, mp_int scratch[], const ECGroup *group)
{
	mp_err res = MP_OKAY;
	mp_int *t0, *t1, *M, *S;

	t0 = &scratch[0];
	t1 = &scratch[1];
	M = &scratch[2];
	S = &scratch[3];

#if MAX_SCRATCH < 4
#error "Scratch array defined too small "
#endif

	
	if (ec_GFp_pt_is_inf_jac(px, py, pz) == MP_YES) {
		

		MP_CHECKOK(ec_GFp_pt_set_inf_jac(rx, ry, rz));
		goto CLEANUP;
	}

	
	MP_CHECKOK(group->meth->field_sqr(px, t0, group->meth));
	MP_CHECKOK(group->meth->field_add(t0, t0, M, group->meth));
	MP_CHECKOK(group->meth->field_add(t0, M, t0, group->meth));
	MP_CHECKOK(group->meth->field_add(t0, paz4, M, group->meth));

	
	MP_CHECKOK(group->meth->field_mul(py, pz, S, group->meth));
	MP_CHECKOK(group->meth->field_add(S, S, rz, group->meth));

	
	MP_CHECKOK(group->meth->field_sqr(py, t0, group->meth));
	MP_CHECKOK(group->meth->field_add(t0, t0, t0, group->meth));
	MP_CHECKOK(group->meth->field_sqr(t0, t1, group->meth));
	MP_CHECKOK(group->meth->field_add(t1, t1, t1, group->meth));

	
	MP_CHECKOK(group->meth->field_mul(px, t0, S, group->meth));
	MP_CHECKOK(group->meth->field_add(S, S, S, group->meth));


	
	MP_CHECKOK(group->meth->field_sqr(M, rx, group->meth));
	MP_CHECKOK(group->meth->field_sub(rx, S, rx, group->meth));
	MP_CHECKOK(group->meth->field_sub(rx, S, rx, group->meth));

	
	MP_CHECKOK(group->meth->field_sub(S, rx, S, group->meth));
	MP_CHECKOK(group->meth->field_mul(S, M, ry, group->meth));
	MP_CHECKOK(group->meth->field_sub(ry, t1, ry, group->meth));

	
	MP_CHECKOK(group->meth->field_mul(paz4, t1, raz4, group->meth));
	MP_CHECKOK(group->meth->field_add(raz4, raz4, raz4, group->meth));


  CLEANUP:
	return res;
}






mp_err
ec_GFp_pt_add_jm_aff(const mp_int *px, const mp_int *py, const mp_int *pz,
					 const mp_int *paz4, const mp_int *qx,
					 const mp_int *qy, mp_int *rx, mp_int *ry, mp_int *rz,
					 mp_int *raz4, mp_int scratch[], const ECGroup *group)
{
	mp_err res = MP_OKAY;
	mp_int *A, *B, *C, *D, *C2, *C3;

	A = &scratch[0];
	B = &scratch[1];
	C = &scratch[2];
	D = &scratch[3];
	C2 = &scratch[4];
	C3 = &scratch[5];

#if MAX_SCRATCH < 6
#error "Scratch array defined too small "
#endif

	

	if (ec_GFp_pt_is_inf_jac(px, py, pz) == MP_YES) {
		MP_CHECKOK(ec_GFp_pt_aff2jac(qx, qy, rx, ry, rz, group));
		MP_CHECKOK(group->meth->field_sqr(rz, raz4, group->meth));
		MP_CHECKOK(group->meth->field_sqr(raz4, raz4, group->meth));
		MP_CHECKOK(group->meth->
				   field_mul(raz4, &group->curvea, raz4, group->meth));
		goto CLEANUP;
	}
	if (ec_GFp_pt_is_inf_aff(qx, qy) == MP_YES) {
		MP_CHECKOK(mp_copy(px, rx));
		MP_CHECKOK(mp_copy(py, ry));
		MP_CHECKOK(mp_copy(pz, rz));
		MP_CHECKOK(mp_copy(paz4, raz4));
		goto CLEANUP;
	}

	
	MP_CHECKOK(group->meth->field_sqr(pz, A, group->meth));
	MP_CHECKOK(group->meth->field_mul(A, pz, B, group->meth));
	MP_CHECKOK(group->meth->field_mul(A, qx, A, group->meth));
	MP_CHECKOK(group->meth->field_mul(B, qy, B, group->meth));

	
	MP_CHECKOK(group->meth->field_sub(A, px, C, group->meth));
	MP_CHECKOK(group->meth->field_sub(B, py, D, group->meth));

	
	MP_CHECKOK(group->meth->field_sqr(C, C2, group->meth));
	MP_CHECKOK(group->meth->field_mul(C, C2, C3, group->meth));

	
	MP_CHECKOK(group->meth->field_mul(pz, C, rz, group->meth));

	
	MP_CHECKOK(group->meth->field_mul(px, C2, C, group->meth));
	
	MP_CHECKOK(group->meth->field_sqr(D, A, group->meth));

	
	MP_CHECKOK(group->meth->field_add(C, C, rx, group->meth));
	MP_CHECKOK(group->meth->field_add(C3, rx, rx, group->meth));
	MP_CHECKOK(group->meth->field_sub(A, rx, rx, group->meth));

	
	MP_CHECKOK(group->meth->field_mul(py, C3, C3, group->meth));

	
	MP_CHECKOK(group->meth->field_sub(C, rx, ry, group->meth));
	MP_CHECKOK(group->meth->field_mul(D, ry, ry, group->meth));
	MP_CHECKOK(group->meth->field_sub(ry, C3, ry, group->meth));

	
	MP_CHECKOK(group->meth->field_sqr(rz, raz4, group->meth));
	MP_CHECKOK(group->meth->field_sqr(raz4, raz4, group->meth));
	MP_CHECKOK(group->meth->
			   field_mul(raz4, &group->curvea, raz4, group->meth));
CLEANUP:
	return res;
}









mp_err
ec_GFp_pt_mul_jm_wNAF(const mp_int *n, const mp_int *px, const mp_int *py,
					  mp_int *rx, mp_int *ry, const ECGroup *group)
{
	mp_err res = MP_OKAY;
	mp_int precomp[16][2], rz, tpx, tpy;
	mp_int raz4;
	mp_int scratch[MAX_SCRATCH];
	signed char *naf = NULL;
	int i, orderBitSize;

	MP_DIGITS(&rz) = 0;
	MP_DIGITS(&raz4) = 0;
	MP_DIGITS(&tpx) = 0;
	MP_DIGITS(&tpy) = 0;
	for (i = 0; i < 16; i++) {
		MP_DIGITS(&precomp[i][0]) = 0;
		MP_DIGITS(&precomp[i][1]) = 0;
	}
	for (i = 0; i < MAX_SCRATCH; i++) {
		MP_DIGITS(&scratch[i]) = 0;
	}

	ARGCHK(group != NULL, MP_BADARG);
	ARGCHK((n != NULL) && (px != NULL) && (py != NULL), MP_BADARG);

	
	MP_CHECKOK(mp_init(&tpx));
	MP_CHECKOK(mp_init(&tpy));;
	MP_CHECKOK(mp_init(&rz));
	MP_CHECKOK(mp_init(&raz4));

	for (i = 0; i < 16; i++) {
		MP_CHECKOK(mp_init(&precomp[i][0]));
		MP_CHECKOK(mp_init(&precomp[i][1]));
	}
	for (i = 0; i < MAX_SCRATCH; i++) {
		MP_CHECKOK(mp_init(&scratch[i]));
	}

	
	MP_CHECKOK(mp_copy(px, &precomp[8][0]));
	MP_CHECKOK(mp_copy(py, &precomp[8][1]));

	
	MP_CHECKOK(group->
			   point_dbl(&precomp[8][0], &precomp[8][1], &tpx, &tpy,
						 group));

	
	for (i = 8; i < 15; i++) {
		MP_CHECKOK(group->
				   point_add(&precomp[i][0], &precomp[i][1], &tpx, &tpy,
							 &precomp[i + 1][0], &precomp[i + 1][1],
							 group));
	}

	
	for (i = 0; i < 8; i++) {
		MP_CHECKOK(mp_copy(&precomp[15 - i][0], &precomp[i][0]));
		MP_CHECKOK(group->meth->
				   field_neg(&precomp[15 - i][1], &precomp[i][1],
							 group->meth));
	}

	
	MP_CHECKOK(ec_GFp_pt_set_inf_jac(rx, ry, &rz));

	orderBitSize = mpl_significant_bits(&group->order);

	
	naf = (signed char *) malloc(sizeof(signed char) * (orderBitSize + 1));
	if (naf == NULL) {
		res = MP_MEM;
		goto CLEANUP;
	}

	
	ec_compute_wNAF(naf, orderBitSize, n, 5);

	
	for (i = orderBitSize; i >= 0; i--) {
		
		ec_GFp_pt_dbl_jm(rx, ry, &rz, &raz4, rx, ry, &rz, 
					     &raz4, scratch, group);
		if (naf[i] != 0) {
			ec_GFp_pt_add_jm_aff(rx, ry, &rz, &raz4,
								 &precomp[(naf[i] + 15) / 2][0],
								 &precomp[(naf[i] + 15) / 2][1], rx, ry,
								 &rz, &raz4, scratch, group);
		}
	}

	
	MP_CHECKOK(ec_GFp_pt_jac2aff(rx, ry, &rz, rx, ry, group));

  CLEANUP:
	for (i = 0; i < MAX_SCRATCH; i++) {
		mp_clear(&scratch[i]);
	}
	for (i = 0; i < 16; i++) {
		mp_clear(&precomp[i][0]);
		mp_clear(&precomp[i][1]);
	}
	mp_clear(&tpx);
	mp_clear(&tpy);
	mp_clear(&rz);
	mp_clear(&raz4);
	free(naf);
	return res;
}
