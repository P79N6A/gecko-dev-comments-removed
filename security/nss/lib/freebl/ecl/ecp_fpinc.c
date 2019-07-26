















#ifndef PREFIX
#define PREFIX(b) PREFIX1(ECFP_BSIZE, b)
#define PREFIX1(bsize, b) PREFIX2(bsize, b)
#define PREFIX2(bsize, b) ecfp ## bsize ## _ ## b
#endif



mp_err PREFIX(isZero) (const double *d) {
	int i;

	for (i = 0; i < ECFP_NUMDOUBLES; i++) {
		if (d[i] != 0)
			return MP_NO;
	}
	return MP_YES;
}


void PREFIX(zero) (double *t) {
	int i;

	for (i = 0; i < ECFP_NUMDOUBLES; i++) {
		t[i] = 0;
	}
}


void PREFIX(one) (double *t) {
	int i;

	t[0] = 1;
	for (i = 1; i < ECFP_NUMDOUBLES; i++) {
		t[i] = 0;
	}
}


mp_err PREFIX(pt_is_inf_jac) (const ecfp_jac_pt * p) {
	return PREFIX(isZero) (p->z);
}


void PREFIX(set_pt_inf_jac) (ecfp_jac_pt * p) {
	PREFIX(zero) (p->z);
}


mp_err PREFIX(pt_is_inf_aff) (const ecfp_aff_pt * p) {
	if (PREFIX(isZero) (p->x) == MP_YES && PREFIX(isZero) (p->y) == MP_YES)
		return MP_YES;
	return MP_NO;
}


void PREFIX(set_pt_inf_aff) (ecfp_aff_pt * p) {
	PREFIX(zero) (p->x);
	PREFIX(zero) (p->y);
}



mp_err PREFIX(pt_is_inf_jm) (const ecfp_jm_pt * p) {
	return PREFIX(isZero) (p->z);
}


void PREFIX(set_pt_inf_jm) (ecfp_jm_pt * p) {
	PREFIX(zero) (p->z);
}



mp_err PREFIX(pt_is_inf_chud) (const ecfp_chud_pt * p) {
	return PREFIX(isZero) (p->z);
}


void PREFIX(set_pt_inf_chud) (ecfp_chud_pt * p) {
	PREFIX(zero) (p->z);
}


void PREFIX(copy) (double *dest, const double *src) {
	int i;

	for (i = 0; i < ECFP_NUMDOUBLES; i++) {
		dest[i] = src[i];
	}
}


void PREFIX(negLong) (double *dest, const double *src) {
	int i;

	for (i = 0; i < 2 * ECFP_NUMDOUBLES; i++) {
		dest[i] = -src[i];
	}
}




void PREFIX(pt_neg_chud) (const ecfp_chud_pt * p, ecfp_chud_pt * r) {
	int i;

	PREFIX(copy) (r->x, p->x);
	PREFIX(copy) (r->z, p->z);
	PREFIX(copy) (r->z2, p->z2);
	PREFIX(copy) (r->z3, p->z3);
	for (i = 0; i < ECFP_NUMDOUBLES; i++) {
		r->y[i] = -p->y[i];
	}
}




void PREFIX(addShort) (double *r, const double *x, const double *y) {
	int i;

	for (i = 0; i < ECFP_NUMDOUBLES; i++) {
		*r++ = *x++ + *y++;
	}
}




void PREFIX(addLong) (double *r, const double *x, const double *y) {
	int i;

	for (i = 0; i < 2 * ECFP_NUMDOUBLES; i++) {
		*r++ = *x++ + *y++;
	}
}




void PREFIX(subtractShort) (double *r, const double *x, const double *y) {
	int i;

	for (i = 0; i < ECFP_NUMDOUBLES; i++) {
		*r++ = *x++ - *y++;
	}
}




void PREFIX(subtractLong) (double *r, const double *x, const double *y) {
	int i;

	for (i = 0; i < 2 * ECFP_NUMDOUBLES; i++) {
		*r++ = *x++ - *y++;
	}
}




void PREFIX(multiply)(double *r, const double *x, const double *y) {
	int i, j;

	for(j=0;j<ECFP_NUMDOUBLES-1;j++) {
		r[j] = x[0] * y[j];
		r[j+(ECFP_NUMDOUBLES-1)] = x[ECFP_NUMDOUBLES-1] * y[j];
	}
	r[ECFP_NUMDOUBLES-1] = x[0] * y[ECFP_NUMDOUBLES-1];
	r[ECFP_NUMDOUBLES-1] += x[ECFP_NUMDOUBLES-1] * y[0];
	r[2*ECFP_NUMDOUBLES-2] = x[ECFP_NUMDOUBLES-1] * y[ECFP_NUMDOUBLES-1];
	r[2*ECFP_NUMDOUBLES-1] = 0;
	
	for(i=1;i<ECFP_NUMDOUBLES-1;i++) {
		for(j=0;j<ECFP_NUMDOUBLES;j++) {
			r[i+j] += (x[i] * y[j]);
		}
	}
}




void PREFIX(square) (double *r, const double *x) {
	PREFIX(multiply) (r, x, x);
}



void PREFIX(pt_dbl_jac) (const ecfp_jac_pt * dp, ecfp_jac_pt * dr,
						 const EC_group_fp * group) {
	double t0[2 * ECFP_NUMDOUBLES], t1[2 * ECFP_NUMDOUBLES],
		M[2 * ECFP_NUMDOUBLES], S[2 * ECFP_NUMDOUBLES];

	
	if (PREFIX(pt_is_inf_jac) (dp) == MP_YES) {
		
		PREFIX(set_pt_inf_jac) (dr);
		goto CLEANUP;
	}

	

	

	if (group->aIsM3) {
		
		PREFIX(square) (t1, dp->z);
		group->ecfp_reduce(t1, t1, group);	

		PREFIX(addShort) (t0, dp->x, t1);	
		PREFIX(subtractShort) (t1, dp->x, t1);	
		PREFIX(multiply) (M, t0, t1);	
		PREFIX(addLong) (t0, M, M);	
		PREFIX(addLong) (M, t0, M);	
		group->ecfp_reduce(M, M, group);
	} else {
		
		
		PREFIX(square) (t0, dp->x);
		PREFIX(addLong) (M, t0, t0);
		PREFIX(addLong) (t0, t0, M);	
		PREFIX(square) (M, dp->z);
		group->ecfp_reduce(M, M, group);
		PREFIX(square) (t1, M);
		group->ecfp_reduce(t1, t1, group);
		PREFIX(multiply) (M, t1, group->curvea);	
		PREFIX(addLong) (M, M, t0);
		group->ecfp_reduce(M, M, group);
	}

	
	PREFIX(multiply) (t1, dp->y, dp->z);
	PREFIX(addLong) (t1, t1, t1);
	group->ecfp_reduce(dr->z, t1, group);

	
	PREFIX(square) (t0, dp->y);
	group->ecfp_reduce(t0, t0, group);
	PREFIX(addShort) (t0, t0, t0);

	
	PREFIX(multiply) (S, dp->x, t0);
	PREFIX(addLong) (S, S, S);
	group->ecfp_reduce(S, S, group);

	
	PREFIX(square) (t1, M);
	PREFIX(subtractShort) (t1, t1, S);
	PREFIX(subtractShort) (t1, t1, S);
	group->ecfp_reduce(dr->x, t1, group);

	
	PREFIX(square) (t1, t0);	
	PREFIX(subtractShort) (S, S, dr->x);
	PREFIX(multiply) (t0, M, S);
	PREFIX(subtractLong) (t0, t0, t1);
	PREFIX(subtractLong) (t0, t0, t1);
	group->ecfp_reduce(dr->y, t0, group);

  CLEANUP:
	return;
}




void PREFIX(pt_add_jac_aff) (const ecfp_jac_pt * p, const ecfp_aff_pt * q,
							 ecfp_jac_pt * r, const EC_group_fp * group) {
	
	double A[2 * ECFP_NUMDOUBLES], B[2 * ECFP_NUMDOUBLES],
		C[2 * ECFP_NUMDOUBLES], C2[2 * ECFP_NUMDOUBLES],
		D[2 * ECFP_NUMDOUBLES], C3[2 * ECFP_NUMDOUBLES];

	
	if (PREFIX(pt_is_inf_aff) (q) == MP_YES) {
		PREFIX(copy) (r->x, p->x);
		PREFIX(copy) (r->y, p->y);
		PREFIX(copy) (r->z, p->z);
		goto CLEANUP;
	} else if (PREFIX(pt_is_inf_jac) (p) == MP_YES) {
		PREFIX(copy) (r->x, q->x);
		PREFIX(copy) (r->y, q->y);
		
		PREFIX(one) (r->z);
		goto CLEANUP;
	}

	


	
	PREFIX(square) (A, p->z);
	group->ecfp_reduce(A, A, group);
	PREFIX(multiply) (B, A, p->z);
	group->ecfp_reduce(B, B, group);

	
	PREFIX(multiply) (C, q->x, A);
	PREFIX(subtractShort) (C, C, p->x);
	group->ecfp_reduce(C, C, group);

	
	PREFIX(multiply) (D, q->y, B);
	PREFIX(subtractShort) (D, D, p->y);
	group->ecfp_reduce(D, D, group);

	
	PREFIX(square) (C2, C);
	group->ecfp_reduce(C2, C2, group);
	PREFIX(multiply) (C3, C2, C);
	group->ecfp_reduce(C3, C3, group);

	
	PREFIX(multiply) (A, p->z, C);
	group->ecfp_reduce(r->z, A, group);

	
	PREFIX(multiply) (C, p->x, C2);

	
	PREFIX(square) (A, D);

	
	PREFIX(subtractShort) (A, A, C3);
	PREFIX(subtractLong) (A, A, C);
	PREFIX(subtractLong) (A, A, C);
	group->ecfp_reduce(r->x, A, group);

	
	PREFIX(multiply) (B, p->y, C3);

	
	PREFIX(subtractShort) (C, C, r->x);
	group->ecfp_reduce(C, C, group);

	
	PREFIX(multiply) (A, D, C);
	PREFIX(subtractLong) (A, A, B);
	group->ecfp_reduce(r->y, A, group);

  CLEANUP:
	return;
}



void PREFIX(pt_add_jac) (const ecfp_jac_pt * p, const ecfp_jac_pt * q,
						 ecfp_jac_pt * r, const EC_group_fp * group) {

	
	double t0[2 * ECFP_NUMDOUBLES], t1[2 * ECFP_NUMDOUBLES],
		U[2 * ECFP_NUMDOUBLES], R[2 * ECFP_NUMDOUBLES],
		S[2 * ECFP_NUMDOUBLES], H[2 * ECFP_NUMDOUBLES],
		H3[2 * ECFP_NUMDOUBLES];

	
	if (PREFIX(pt_is_inf_jac) (p) == MP_YES) {
		PREFIX(copy) (r->x, q->x);
		PREFIX(copy) (r->y, q->y);
		PREFIX(copy) (r->z, q->z);
		goto CLEANUP;
	}

	
	if (PREFIX(pt_is_inf_jac) (q) == MP_YES) {
		PREFIX(copy) (r->x, p->x);
		PREFIX(copy) (r->y, p->y);
		PREFIX(copy) (r->z, p->z);
		goto CLEANUP;
	}

	
	PREFIX(square) (t0, q->z);
	group->ecfp_reduce(t0, t0, group);
	PREFIX(multiply) (U, p->x, t0);
	group->ecfp_reduce(U, U, group);
	PREFIX(multiply) (t1, t0, q->z);
	group->ecfp_reduce(t1, t1, group);
	PREFIX(multiply) (t0, p->y, t1);
	group->ecfp_reduce(S, t0, group);

	
	PREFIX(square) (t0, p->z);
	group->ecfp_reduce(t0, t0, group);
	PREFIX(multiply) (H, q->x, t0);
	PREFIX(subtractShort) (H, H, U);
	group->ecfp_reduce(H, H, group);
	PREFIX(multiply) (t1, t0, p->z);	
	group->ecfp_reduce(t1, t1, group);
	PREFIX(multiply) (t0, t1, q->y);	
	PREFIX(subtractShort) (t0, t0, S);
	group->ecfp_reduce(R, t0, group);

	
	PREFIX(square) (t0, H);
	group->ecfp_reduce(t0, t0, group);
	PREFIX(multiply) (t1, U, t0);
	group->ecfp_reduce(U, t1, group);
	PREFIX(multiply) (H3, t0, H);
	group->ecfp_reduce(H3, H3, group);

	
	PREFIX(multiply) (t0, q->z, H);
	group->ecfp_reduce(t0, t0, group);
	PREFIX(multiply) (t1, t0, p->z);
	group->ecfp_reduce(r->z, t1, group);

	
	PREFIX(square) (t0, R);
	PREFIX(subtractShort) (t0, t0, H3);
	PREFIX(subtractShort) (t0, t0, U);
	PREFIX(subtractShort) (t0, t0, U);
	group->ecfp_reduce(r->x, t0, group);

	
	PREFIX(subtractShort) (t1, U, r->x);
	PREFIX(multiply) (t0, t1, R);
	PREFIX(multiply) (t1, S, H3);
	PREFIX(subtractLong) (t1, t0, t1);
	group->ecfp_reduce(r->y, t1, group);

  CLEANUP:
	return;
}



void PREFIX(pt_dbl_jm) (const ecfp_jm_pt * p, ecfp_jm_pt * r,
						const EC_group_fp * group) {

	
	double t0[2 * ECFP_NUMDOUBLES], t1[2 * ECFP_NUMDOUBLES],
		M[2 * ECFP_NUMDOUBLES], S[2 * ECFP_NUMDOUBLES],
		U[2 * ECFP_NUMDOUBLES], T[2 * ECFP_NUMDOUBLES];

	
	if (PREFIX(pt_is_inf_jm) (p) == MP_YES) {
		
		PREFIX(set_pt_inf_jm) (r);
		goto CLEANUP;
	}

	
	PREFIX(square) (t0, p->x);
	PREFIX(addLong) (M, t0, t0);
	PREFIX(addLong) (t0, t0, M);	
	PREFIX(addShort) (t0, t0, p->az4);
	group->ecfp_reduce(M, t0, group);

	
	PREFIX(multiply) (t1, p->y, p->z);
	PREFIX(addLong) (t1, t1, t1);
	group->ecfp_reduce(r->z, t1, group);

	
	PREFIX(square) (t0, p->y);
	group->ecfp_reduce(t0, t0, group);
	PREFIX(addShort) (t0, t0, t0);
	PREFIX(square) (U, t0);
	group->ecfp_reduce(U, U, group);
	PREFIX(addShort) (U, U, U);

	
	PREFIX(multiply) (S, p->x, t0);
	group->ecfp_reduce(S, S, group);
	PREFIX(addShort) (S, S, S);

	
	PREFIX(square) (T, M);
	PREFIX(subtractShort) (T, T, S);
	PREFIX(subtractShort) (T, T, S);
	group->ecfp_reduce(r->x, T, group);

	
	PREFIX(subtractShort) (S, S, r->x);
	PREFIX(multiply) (t0, M, S);
	PREFIX(subtractShort) (t0, t0, U);
	group->ecfp_reduce(r->y, t0, group);

	
	PREFIX(multiply) (t1, U, p->az4);
	PREFIX(addLong) (t1, t1, t1);
	group->ecfp_reduce(r->az4, t1, group);

  CLEANUP:
	return;
}




void PREFIX(pt_dbl_aff2chud) (const ecfp_aff_pt * p, ecfp_chud_pt * r,
							  const EC_group_fp * group) {
	double t0[2 * ECFP_NUMDOUBLES], t1[2 * ECFP_NUMDOUBLES],
		M[2 * ECFP_NUMDOUBLES], twoY2[2 * ECFP_NUMDOUBLES],
		S[2 * ECFP_NUMDOUBLES];

	
	if (PREFIX(pt_is_inf_aff) (p) == MP_YES) {
		PREFIX(set_pt_inf_chud) (r);
		goto CLEANUP;
	}

	
	PREFIX(square) (t0, p->x);
	PREFIX(addLong) (t1, t0, t0);
	PREFIX(addLong) (t1, t1, t0);
	PREFIX(addShort) (t1, t1, group->curvea);
	group->ecfp_reduce(M, t1, group);

	
	PREFIX(square) (twoY2, p->y);
	PREFIX(addLong) (twoY2, twoY2, twoY2);
	group->ecfp_reduce(twoY2, twoY2, group);
	PREFIX(multiply) (S, p->x, twoY2);
	PREFIX(addLong) (S, S, S);
	group->ecfp_reduce(S, S, group);

	
	PREFIX(square) (t0, M);
	PREFIX(subtractShort) (t0, t0, S);
	PREFIX(subtractShort) (t0, t0, S);
	group->ecfp_reduce(r->x, t0, group);

	
	PREFIX(subtractShort) (t0, S, r->x);
	PREFIX(multiply) (t1, t0, M);
	PREFIX(square) (t0, twoY2);
	PREFIX(subtractLong) (t1, t1, t0);
	PREFIX(subtractLong) (t1, t1, t0);
	group->ecfp_reduce(r->y, t1, group);

	
	PREFIX(addShort) (r->z, p->y, p->y);

	
	PREFIX(square) (t0, r->z);
	group->ecfp_reduce(r->z2, t0, group);

	
	PREFIX(multiply) (t0, r->z, r->z2);
	group->ecfp_reduce(r->z3, t0, group);

  CLEANUP:
	return;
}




void PREFIX(pt_add_jm_chud) (ecfp_jm_pt * p, ecfp_chud_pt * q,
							 ecfp_jm_pt * r, const EC_group_fp * group) {

	double t0[2 * ECFP_NUMDOUBLES], t1[2 * ECFP_NUMDOUBLES],
		U[2 * ECFP_NUMDOUBLES], R[2 * ECFP_NUMDOUBLES],
		S[2 * ECFP_NUMDOUBLES], H[2 * ECFP_NUMDOUBLES],
		H3[2 * ECFP_NUMDOUBLES], pz2[2 * ECFP_NUMDOUBLES];

	

	if (PREFIX(pt_is_inf_jm) (p) == MP_YES) {
		PREFIX(copy) (r->x, q->x);
		PREFIX(copy) (r->y, q->y);
		PREFIX(copy) (r->z, q->z);
		PREFIX(square) (t0, q->z2);
		group->ecfp_reduce(t0, t0, group);
		PREFIX(multiply) (t1, t0, group->curvea);
		group->ecfp_reduce(r->az4, t1, group);
		goto CLEANUP;
	}
	
	if (PREFIX(pt_is_inf_chud) (q) == MP_YES) {
		PREFIX(copy) (r->x, p->x);
		PREFIX(copy) (r->y, p->y);
		PREFIX(copy) (r->z, p->z);
		PREFIX(copy) (r->az4, p->az4);
		goto CLEANUP;
	}

	
	PREFIX(multiply) (U, p->x, q->z2);
	group->ecfp_reduce(U, U, group);

	
	PREFIX(square) (t0, p->z);
	group->ecfp_reduce(pz2, t0, group);
	PREFIX(multiply) (H, pz2, q->x);
	group->ecfp_reduce(H, H, group);
	PREFIX(subtractShort) (H, H, U);

	
	PREFIX(square) (t0, H);
	group->ecfp_reduce(t0, t0, group);
	PREFIX(multiply) (t1, U, t0);
	group->ecfp_reduce(U, t1, group);
	PREFIX(multiply) (H3, t0, H);
	group->ecfp_reduce(H3, H3, group);

	
	PREFIX(multiply) (S, p->y, q->z3);
	group->ecfp_reduce(S, S, group);

	
	PREFIX(multiply) (t0, pz2, p->z);
	group->ecfp_reduce(t0, t0, group);
	PREFIX(multiply) (R, t0, q->y);
	PREFIX(subtractShort) (R, R, S);
	group->ecfp_reduce(R, R, group);

	
	PREFIX(multiply) (t1, q->z, H);
	group->ecfp_reduce(t1, t1, group);
	PREFIX(multiply) (t0, p->z, t1);
	group->ecfp_reduce(r->z, t0, group);

	
	PREFIX(square) (t0, R);
	PREFIX(subtractShort) (t0, t0, H3);
	PREFIX(subtractShort) (t0, t0, U);
	PREFIX(subtractShort) (t0, t0, U);
	group->ecfp_reduce(r->x, t0, group);

	
	PREFIX(subtractShort) (t1, U, r->x);
	PREFIX(multiply) (t0, t1, R);
	PREFIX(multiply) (t1, S, H3);
	PREFIX(subtractLong) (t1, t0, t1);
	group->ecfp_reduce(r->y, t1, group);

	if (group->aIsM3) {			
		
		PREFIX(square) (t0, r->z);
		group->ecfp_reduce(t0, t0, group);
		PREFIX(square) (t1, t0);
		PREFIX(addLong) (t0, t1, t1);
		PREFIX(addLong) (t0, t0, t1);
		PREFIX(negLong) (t0, t0);
		group->ecfp_reduce(r->az4, t0, group);
	} else {					
		
		PREFIX(square) (t0, r->z);
		group->ecfp_reduce(t0, t0, group);
		PREFIX(square) (t1, t0);
		group->ecfp_reduce(t1, t1, group);
		PREFIX(multiply) (t0, group->curvea, t1);
		group->ecfp_reduce(r->az4, t0, group);
	}
  CLEANUP:
	return;
}



void PREFIX(pt_add_chud) (const ecfp_chud_pt * p, const ecfp_chud_pt * q,
						  ecfp_chud_pt * r, const EC_group_fp * group) {

	
	double t0[2 * ECFP_NUMDOUBLES], t1[2 * ECFP_NUMDOUBLES],
		U[2 * ECFP_NUMDOUBLES], R[2 * ECFP_NUMDOUBLES],
		S[2 * ECFP_NUMDOUBLES], H[2 * ECFP_NUMDOUBLES],
		H3[2 * ECFP_NUMDOUBLES];

	
	if (PREFIX(pt_is_inf_chud) (p) == MP_YES) {
		PREFIX(copy) (r->x, q->x);
		PREFIX(copy) (r->y, q->y);
		PREFIX(copy) (r->z, q->z);
		PREFIX(copy) (r->z2, q->z2);
		PREFIX(copy) (r->z3, q->z3);
		goto CLEANUP;
	}

	
	if (PREFIX(pt_is_inf_chud) (q) == MP_YES) {
		PREFIX(copy) (r->x, p->x);
		PREFIX(copy) (r->y, p->y);
		PREFIX(copy) (r->z, p->z);
		PREFIX(copy) (r->z2, p->z2);
		PREFIX(copy) (r->z3, p->z3);
		goto CLEANUP;
	}

	
	PREFIX(multiply) (U, p->x, q->z2);
	group->ecfp_reduce(U, U, group);

	
	PREFIX(multiply) (H, q->x, p->z2);
	PREFIX(subtractShort) (H, H, U);
	group->ecfp_reduce(H, H, group);

	
	PREFIX(square) (t0, H);
	group->ecfp_reduce(t0, t0, group);
	PREFIX(multiply) (t1, U, t0);
	group->ecfp_reduce(U, t1, group);
	PREFIX(multiply) (H3, t0, H);
	group->ecfp_reduce(H3, H3, group);

	
	PREFIX(multiply) (S, p->y, q->z3);
	group->ecfp_reduce(S, S, group);

	
	PREFIX(multiply) (t0, q->z, H);
	group->ecfp_reduce(t0, t0, group);
	PREFIX(multiply) (t1, t0, p->z);
	group->ecfp_reduce(r->z, t1, group);

	
	PREFIX(multiply) (t0, q->y, p->z3);
	PREFIX(subtractShort) (t0, t0, S);
	group->ecfp_reduce(R, t0, group);

	
	PREFIX(square) (t0, R);
	PREFIX(subtractShort) (t0, t0, H3);
	PREFIX(subtractShort) (t0, t0, U);
	PREFIX(subtractShort) (t0, t0, U);
	group->ecfp_reduce(r->x, t0, group);

	
	PREFIX(subtractShort) (t1, U, r->x);
	PREFIX(multiply) (t0, t1, R);
	PREFIX(multiply) (t1, S, H3);
	PREFIX(subtractLong) (t1, t0, t1);
	group->ecfp_reduce(r->y, t1, group);

	
	PREFIX(square) (t0, r->z);
	group->ecfp_reduce(r->z2, t0, group);

	
	PREFIX(multiply) (t0, r->z, r->z2);
	group->ecfp_reduce(r->z3, t0, group);

  CLEANUP:
	return;
}




void PREFIX(precompute_chud) (ecfp_chud_pt * out, const ecfp_aff_pt * p,
							  const EC_group_fp * group) {

	ecfp_chud_pt p2;

	
	PREFIX(copy) (out[8].x, p->x);
	PREFIX(copy) (out[8].y, p->y);
	PREFIX(one) (out[8].z);
	PREFIX(one) (out[8].z2);
	PREFIX(one) (out[8].z3);

	
	PREFIX(pt_dbl_aff2chud) (p, &p2, group);

	
	PREFIX(pt_add_chud) (&out[8], &p2, &out[9], group);
	PREFIX(pt_add_chud) (&out[9], &p2, &out[10], group);
	PREFIX(pt_add_chud) (&out[10], &p2, &out[11], group);
	PREFIX(pt_add_chud) (&out[11], &p2, &out[12], group);
	PREFIX(pt_add_chud) (&out[12], &p2, &out[13], group);
	PREFIX(pt_add_chud) (&out[13], &p2, &out[14], group);
	PREFIX(pt_add_chud) (&out[14], &p2, &out[15], group);

	
	PREFIX(pt_neg_chud) (&out[8], &out[7]);
	PREFIX(pt_neg_chud) (&out[9], &out[6]);
	PREFIX(pt_neg_chud) (&out[10], &out[5]);
	PREFIX(pt_neg_chud) (&out[11], &out[4]);
	PREFIX(pt_neg_chud) (&out[12], &out[3]);
	PREFIX(pt_neg_chud) (&out[13], &out[2]);
	PREFIX(pt_neg_chud) (&out[14], &out[1]);
	PREFIX(pt_neg_chud) (&out[15], &out[0]);
}



void PREFIX(precompute_jac) (ecfp_jac_pt * precomp, const ecfp_aff_pt * p,
							 const EC_group_fp * group) {
	int i;

	
	
	PREFIX(set_pt_inf_jac) (&precomp[0]);
	
	PREFIX(copy) (precomp[1].x, p->x);
	PREFIX(copy) (precomp[1].y, p->y);
	if (PREFIX(pt_is_inf_aff) (p) == MP_YES) {
		PREFIX(zero) (precomp[1].z);
	} else {
		PREFIX(one) (precomp[1].z);
	}
	
	group->pt_dbl_jac(&precomp[1], &precomp[2], group);

	
	for (i = 3; i < 16; i++) {
		group->pt_add_jac_aff(&precomp[i - 1], p, &precomp[i], group);
	}
}
