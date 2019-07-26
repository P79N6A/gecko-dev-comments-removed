



#ifndef __ecp_h_
#define __ecp_h_

#include "ecl-priv.h"


mp_err ec_GFp_pt_is_inf_aff(const mp_int *px, const mp_int *py);


mp_err ec_GFp_pt_set_inf_aff(mp_int *px, mp_int *py);



mp_err ec_GFp_pt_add_aff(const mp_int *px, const mp_int *py,
						 const mp_int *qx, const mp_int *qy, mp_int *rx,
						 mp_int *ry, const ECGroup *group);


mp_err ec_GFp_pt_sub_aff(const mp_int *px, const mp_int *py,
						 const mp_int *qx, const mp_int *qy, mp_int *rx,
						 mp_int *ry, const ECGroup *group);


mp_err ec_GFp_pt_dbl_aff(const mp_int *px, const mp_int *py, mp_int *rx,
						 mp_int *ry, const ECGroup *group);


mp_err ec_GFp_validate_point(const mp_int *px, const mp_int *py, const ECGroup *group);

#ifdef ECL_ENABLE_GFP_PT_MUL_AFF



mp_err ec_GFp_pt_mul_aff(const mp_int *n, const mp_int *px,
						 const mp_int *py, mp_int *rx, mp_int *ry,
						 const ECGroup *group);
#endif



mp_err ec_GFp_pt_aff2jac(const mp_int *px, const mp_int *py, mp_int *rx,
						 mp_int *ry, mp_int *rz, const ECGroup *group);



mp_err ec_GFp_pt_jac2aff(const mp_int *px, const mp_int *py,
						 const mp_int *pz, mp_int *rx, mp_int *ry,
						 const ECGroup *group);



mp_err ec_GFp_pt_is_inf_jac(const mp_int *px, const mp_int *py,
							const mp_int *pz);



mp_err ec_GFp_pt_set_inf_jac(mp_int *px, mp_int *py, mp_int *pz);



mp_err ec_GFp_pt_add_jac_aff(const mp_int *px, const mp_int *py,
							 const mp_int *pz, const mp_int *qx,
							 const mp_int *qy, mp_int *rx, mp_int *ry,
							 mp_int *rz, const ECGroup *group);


mp_err ec_GFp_pt_dbl_jac(const mp_int *px, const mp_int *py,
						 const mp_int *pz, mp_int *rx, mp_int *ry,
						 mp_int *rz, const ECGroup *group);

#ifdef ECL_ENABLE_GFP_PT_MUL_JAC



mp_err ec_GFp_pt_mul_jac(const mp_int *n, const mp_int *px,
						 const mp_int *py, mp_int *rx, mp_int *ry,
						 const ECGroup *group);
#endif






mp_err
 ec_GFp_pts_mul_jac(const mp_int *k1, const mp_int *k2, const mp_int *px,
					const mp_int *py, mp_int *rx, mp_int *ry,
					const ECGroup *group);









mp_err
 ec_GFp_pt_mul_jm_wNAF(const mp_int *n, const mp_int *px, const mp_int *py,
					   mp_int *rx, mp_int *ry, const ECGroup *group);

#endif							
