



#include "ecp.h"
#include "mplogic.h"
#include <stdlib.h>


mp_err
ec_GFp_pt_is_inf_aff(const mp_int *px, const mp_int *py)
{

	if ((mp_cmp_z(px) == 0) && (mp_cmp_z(py) == 0)) {
		return MP_YES;
	} else {
		return MP_NO;
	}

}


mp_err
ec_GFp_pt_set_inf_aff(mp_int *px, mp_int *py)
{
	mp_zero(px);
	mp_zero(py);
	return MP_OKAY;
}





mp_err
ec_GFp_pt_add_aff(const mp_int *px, const mp_int *py, const mp_int *qx,
				  const mp_int *qy, mp_int *rx, mp_int *ry,
				  const ECGroup *group)
{
	mp_err res = MP_OKAY;
	mp_int lambda, temp, tempx, tempy;

	MP_DIGITS(&lambda) = 0;
	MP_DIGITS(&temp) = 0;
	MP_DIGITS(&tempx) = 0;
	MP_DIGITS(&tempy) = 0;
	MP_CHECKOK(mp_init(&lambda));
	MP_CHECKOK(mp_init(&temp));
	MP_CHECKOK(mp_init(&tempx));
	MP_CHECKOK(mp_init(&tempy));
	
	if (ec_GFp_pt_is_inf_aff(px, py) == 0) {
		MP_CHECKOK(mp_copy(qx, rx));
		MP_CHECKOK(mp_copy(qy, ry));
		res = MP_OKAY;
		goto CLEANUP;
	}
	
	if (ec_GFp_pt_is_inf_aff(qx, qy) == 0) {
		MP_CHECKOK(mp_copy(px, rx));
		MP_CHECKOK(mp_copy(py, ry));
		res = MP_OKAY;
		goto CLEANUP;
	}
	
	if (mp_cmp(px, qx) != 0) {
		MP_CHECKOK(group->meth->field_sub(py, qy, &tempy, group->meth));
		MP_CHECKOK(group->meth->field_sub(px, qx, &tempx, group->meth));
		MP_CHECKOK(group->meth->
				   field_div(&tempy, &tempx, &lambda, group->meth));
	} else {
		
		if (((mp_cmp(py, qy) != 0)) || (mp_cmp_z(qy) == 0)) {
			mp_zero(rx);
			mp_zero(ry);
			res = MP_OKAY;
			goto CLEANUP;
		}
		
		MP_CHECKOK(group->meth->field_sqr(qx, &tempx, group->meth));
		MP_CHECKOK(mp_set_int(&temp, 3));
		if (group->meth->field_enc) {
			MP_CHECKOK(group->meth->field_enc(&temp, &temp, group->meth));
		}
		MP_CHECKOK(group->meth->
				   field_mul(&tempx, &temp, &tempx, group->meth));
		MP_CHECKOK(group->meth->
				   field_add(&tempx, &group->curvea, &tempx, group->meth));
		MP_CHECKOK(mp_set_int(&temp, 2));
		if (group->meth->field_enc) {
			MP_CHECKOK(group->meth->field_enc(&temp, &temp, group->meth));
		}
		MP_CHECKOK(group->meth->field_mul(qy, &temp, &tempy, group->meth));
		MP_CHECKOK(group->meth->
				   field_div(&tempx, &tempy, &lambda, group->meth));
	}
	
	MP_CHECKOK(group->meth->field_sqr(&lambda, &tempx, group->meth));
	MP_CHECKOK(group->meth->field_sub(&tempx, px, &tempx, group->meth));
	MP_CHECKOK(group->meth->field_sub(&tempx, qx, &tempx, group->meth));
	
	MP_CHECKOK(group->meth->field_sub(qx, &tempx, &tempy, group->meth));
	MP_CHECKOK(group->meth->
			   field_mul(&tempy, &lambda, &tempy, group->meth));
	MP_CHECKOK(group->meth->field_sub(&tempy, qy, &tempy, group->meth));
	MP_CHECKOK(mp_copy(&tempx, rx));
	MP_CHECKOK(mp_copy(&tempy, ry));

  CLEANUP:
	mp_clear(&lambda);
	mp_clear(&temp);
	mp_clear(&tempx);
	mp_clear(&tempy);
	return res;
}





mp_err
ec_GFp_pt_sub_aff(const mp_int *px, const mp_int *py, const mp_int *qx,
				  const mp_int *qy, mp_int *rx, mp_int *ry,
				  const ECGroup *group)
{
	mp_err res = MP_OKAY;
	mp_int nqy;

	MP_DIGITS(&nqy) = 0;
	MP_CHECKOK(mp_init(&nqy));
	
	MP_CHECKOK(group->meth->field_neg(qy, &nqy, group->meth));
	res = group->point_add(px, py, qx, &nqy, rx, ry, group);
  CLEANUP:
	mp_clear(&nqy);
	return res;
}




mp_err
ec_GFp_pt_dbl_aff(const mp_int *px, const mp_int *py, mp_int *rx,
				  mp_int *ry, const ECGroup *group)
{
	return ec_GFp_pt_add_aff(px, py, px, py, rx, ry, group);
}


#ifdef ECL_ENABLE_GFP_PT_MUL_AFF




mp_err
ec_GFp_pt_mul_aff(const mp_int *n, const mp_int *px, const mp_int *py,
				  mp_int *rx, mp_int *ry, const ECGroup *group)
{
	mp_err res = MP_OKAY;
	mp_int k, k3, qx, qy, sx, sy;
	int b1, b3, i, l;

	MP_DIGITS(&k) = 0;
	MP_DIGITS(&k3) = 0;
	MP_DIGITS(&qx) = 0;
	MP_DIGITS(&qy) = 0;
	MP_DIGITS(&sx) = 0;
	MP_DIGITS(&sy) = 0;
	MP_CHECKOK(mp_init(&k));
	MP_CHECKOK(mp_init(&k3));
	MP_CHECKOK(mp_init(&qx));
	MP_CHECKOK(mp_init(&qy));
	MP_CHECKOK(mp_init(&sx));
	MP_CHECKOK(mp_init(&sy));

	
	if (mp_cmp_z(n) == 0) {
		mp_zero(rx);
		mp_zero(ry);
		res = MP_OKAY;
		goto CLEANUP;
	}
	
	MP_CHECKOK(mp_copy(px, &qx));
	MP_CHECKOK(mp_copy(py, &qy));
	MP_CHECKOK(mp_copy(n, &k));
	
	if (mp_cmp_z(n) < 0) {
		MP_CHECKOK(group->meth->field_neg(&qy, &qy, group->meth));
		MP_CHECKOK(mp_neg(&k, &k));
	}
#ifdef ECL_DEBUG				
	l = mpl_significant_bits(&k) - 1;
	MP_CHECKOK(mp_copy(&qx, &sx));
	MP_CHECKOK(mp_copy(&qy, &sy));
	for (i = l - 1; i >= 0; i--) {
		
		MP_CHECKOK(group->point_dbl(&sx, &sy, &sx, &sy, group));
		
		if (mpl_get_bit(&k, i) != 0) {
			MP_CHECKOK(group->
					   point_add(&sx, &sy, &qx, &qy, &sx, &sy, group));
		}
	}
#else							

	
	MP_CHECKOK(mp_set_int(&k3, 3));
	MP_CHECKOK(mp_mul(&k, &k3, &k3));
	
	MP_CHECKOK(mp_copy(&qx, &sx));
	MP_CHECKOK(mp_copy(&qy, &sy));
	
	l = mpl_significant_bits(&k3) - 1;
	
	for (i = l - 1; i >= 1; i--) {
		
		MP_CHECKOK(group->point_dbl(&sx, &sy, &sx, &sy, group));
		b3 = MP_GET_BIT(&k3, i);
		b1 = MP_GET_BIT(&k, i);
		
		if ((b3 == 1) && (b1 == 0)) {
			MP_CHECKOK(group->
					   point_add(&sx, &sy, &qx, &qy, &sx, &sy, group));
			
		} else if ((b3 == 0) && (b1 == 1)) {
			MP_CHECKOK(group->
					   point_sub(&sx, &sy, &qx, &qy, &sx, &sy, group));
		}
	}
#endif
	
	MP_CHECKOK(mp_copy(&sx, rx));
	MP_CHECKOK(mp_copy(&sy, ry));

  CLEANUP:
	mp_clear(&k);
	mp_clear(&k3);
	mp_clear(&qx);
	mp_clear(&qy);
	mp_clear(&sx);
	mp_clear(&sy);
	return res;
}
#endif


mp_err 
ec_GFp_validate_point(const mp_int *px, const mp_int *py, const ECGroup *group)
{
	mp_err res = MP_NO;
	mp_int accl, accr, tmp, pxt, pyt;

	MP_DIGITS(&accl) = 0;
	MP_DIGITS(&accr) = 0;
	MP_DIGITS(&tmp) = 0;
	MP_DIGITS(&pxt) = 0;
	MP_DIGITS(&pyt) = 0;
	MP_CHECKOK(mp_init(&accl));
	MP_CHECKOK(mp_init(&accr));
	MP_CHECKOK(mp_init(&tmp));
	MP_CHECKOK(mp_init(&pxt));
	MP_CHECKOK(mp_init(&pyt));

    
	if (ec_GFp_pt_is_inf_aff(px, py) == MP_YES) {
		res = MP_NO;
		goto CLEANUP;
	}
    


	if ((MP_SIGN(px) == MP_NEG) || (mp_cmp(px, &group->meth->irr) >= 0) || 
		(MP_SIGN(py) == MP_NEG) || (mp_cmp(py, &group->meth->irr) >= 0)) {
		res = MP_NO;
		goto CLEANUP;
	}
    
	if (group->meth->field_enc) {
		group->meth->field_enc(px, &pxt, group->meth);
		group->meth->field_enc(py, &pyt, group->meth);
	} else {
		mp_copy(px, &pxt);
		mp_copy(py, &pyt);
	}
	
	MP_CHECKOK( group->meth->field_sqr(&pyt, &accl, group->meth) );
	
	MP_CHECKOK( group->meth->field_sqr(&pxt, &tmp, group->meth) );
	MP_CHECKOK( group->meth->field_add(&tmp, &group->curvea, &tmp, group->meth) );
	MP_CHECKOK( group->meth->field_mul(&tmp, &pxt, &accr, group->meth) );
	MP_CHECKOK( group->meth->field_add(&accr, &group->curveb, &accr, group->meth) );
	
	MP_CHECKOK( group->meth->field_sub(&accl, &accr, &accr, group->meth) );
	if (mp_cmp_z(&accr) != 0) {
		res = MP_NO;
		goto CLEANUP;
	}
    


	MP_CHECKOK( ECPoint_mul(group, &group->order, px, py, &pxt, &pyt) );
	if (ec_GFp_pt_is_inf_aff(&pxt, &pyt) != MP_YES) {
		res = MP_NO;
		goto CLEANUP;
	}

	res = MP_YES;

CLEANUP:
	mp_clear(&accl);
	mp_clear(&accr);
	mp_clear(&tmp);
	mp_clear(&pxt);
	mp_clear(&pyt);
	return res;
}
