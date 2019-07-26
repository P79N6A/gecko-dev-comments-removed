



#ifndef __ec2_h_
#define __ec2_h_

#include "ecl-priv.h"


mp_err ec_GF2m_pt_is_inf_aff(const mp_int *px, const mp_int *py);


mp_err ec_GF2m_pt_set_inf_aff(mp_int *px, mp_int *py);



mp_err ec_GF2m_pt_add_aff(const mp_int *px, const mp_int *py,
						  const mp_int *qx, const mp_int *qy, mp_int *rx,
						  mp_int *ry, const ECGroup *group);


mp_err ec_GF2m_pt_sub_aff(const mp_int *px, const mp_int *py,
						  const mp_int *qx, const mp_int *qy, mp_int *rx,
						  mp_int *ry, const ECGroup *group);


mp_err ec_GF2m_pt_dbl_aff(const mp_int *px, const mp_int *py, mp_int *rx,
						  mp_int *ry, const ECGroup *group);


mp_err ec_GF2m_validate_point(const mp_int *px, const mp_int *py, const ECGroup *group);


#ifdef ECL_ENABLE_GF2M_PT_MUL_AFF



mp_err ec_GF2m_pt_mul_aff(const mp_int *n, const mp_int *px,
						  const mp_int *py, mp_int *rx, mp_int *ry,
						  const ECGroup *group);
#endif




mp_err ec_GF2m_pt_mul_mont(const mp_int *n, const mp_int *px,
						   const mp_int *py, mp_int *rx, mp_int *ry,
						   const ECGroup *group);

#ifdef ECL_ENABLE_GF2M_PROJ


mp_err ec_GF2m_pt_aff2proj(const mp_int *px, const mp_int *py, mp_int *rx,
						   mp_int *ry, mp_int *rz, const ECGroup *group);



mp_err ec_GF2m_pt_proj2aff(const mp_int *px, const mp_int *py,
						   const mp_int *pz, mp_int *rx, mp_int *ry,
						   const ECGroup *group);



mp_err ec_GF2m_pt_is_inf_proj(const mp_int *px, const mp_int *py,
							  const mp_int *pz);



mp_err ec_GF2m_pt_set_inf_proj(mp_int *px, mp_int *py, mp_int *pz);



mp_err ec_GF2m_pt_add_proj(const mp_int *px, const mp_int *py,
						   const mp_int *pz, const mp_int *qx,
						   const mp_int *qy, mp_int *rx, mp_int *ry,
						   mp_int *rz, const ECGroup *group);


mp_err ec_GF2m_pt_dbl_proj(const mp_int *px, const mp_int *py,
						   const mp_int *pz, mp_int *rx, mp_int *ry,
						   mp_int *rz, const ECGroup *group);




mp_err ec_GF2m_pt_mul_proj(const mp_int *n, const mp_int *px,
						   const mp_int *py, mp_int *rx, mp_int *ry,
						   const ECGroup *group);
#endif

#endif							
