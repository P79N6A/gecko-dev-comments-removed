






#ifndef _PK11PQG_H_
#define  _PK11PQG_H_ 1

#include "blapit.h"

SEC_BEGIN_PROTOS





extern SECStatus PK11_PQG_ParamGen(unsigned int j, PQGParams **pParams, 
							PQGVerify **pVfy);






extern SECStatus PK11_PQG_ParamGenSeedLen( unsigned int j, 
	unsigned int seedBytes, PQGParams **pParams, PQGVerify **pVfy);



























extern SECStatus
PK11_PQG_ParamGenV2(unsigned int L, unsigned int N, unsigned int seedBytes,
		    PQGParams **pParams, PQGVerify **pVfy);
















extern SECStatus PK11_PQG_VerifyParams(const PQGParams *params, 
                                    const PQGVerify *vfy, SECStatus *result);
extern void PK11_PQG_DestroyParams(PQGParams *params);
extern void PK11_PQG_DestroyVerify(PQGVerify *vfy);






extern PQGParams * PK11_PQG_NewParams(const SECItem * prime, const 
				SECItem * subPrime, const SECItem * base);






extern SECStatus PK11_PQG_GetPrimeFromParams(const PQGParams *params, 
							SECItem * prime);






extern SECStatus PK11_PQG_GetSubPrimeFromParams(const PQGParams *params, 
							SECItem * subPrime);






extern SECStatus PK11_PQG_GetBaseFromParams(const PQGParams *params, 
							SECItem *base);







extern PQGVerify * PK11_PQG_NewVerify(unsigned int counter, 
				const SECItem * seed, const SECItem * h);





extern unsigned int PK11_PQG_GetCounterFromVerify(const PQGVerify *verify);





extern SECStatus PK11_PQG_GetSeedFromVerify(const PQGVerify *verify, 
							SECItem *seed);





extern SECStatus PK11_PQG_GetHFromVerify(const PQGVerify *verify, SECItem * h);

SEC_END_PROTOS

#endif
