









#ifndef _PQG_H_
#define _PQG_H_ 1




unsigned int PQG_GetLength(const SECItem *obj);



SECStatus PQG_Check(const PQGParams *params);

HASH_HashType PQG_GetHashType(const PQGParams *params);

#endif 
