






#ifndef __ecl_h_
#define __ecl_h_

#include "ecl-exp.h"
#include "mpi.h"

struct ECGroupStr;
typedef struct ECGroupStr ECGroup;


ECGroup *ECGroup_fromHex(const ECCurveParams * params);


ECGroup *ECGroup_fromName(const ECCurveName name);


void ECGroup_free(ECGroup *group);


ECCurveParams *EC_GetNamedCurveParams(const ECCurveName name);


ECCurveParams *ECCurveParams_dup(const ECCurveParams * params);


void EC_FreeCurveParams(ECCurveParams * params);





mp_err ECPoint_mul(const ECGroup *group, const mp_int *k, const mp_int *px,
				   const mp_int *py, mp_int *qx, mp_int *qy);





mp_err ECPoints_mul(const ECGroup *group, const mp_int *k1,
					const mp_int *k2, const mp_int *px, const mp_int *py,
					mp_int *qx, mp_int *qy);





mp_err ECPoint_validate(const ECGroup *group, const mp_int *px, const 
					mp_int *py);

#endif							
