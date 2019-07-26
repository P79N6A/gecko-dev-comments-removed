




#ifndef _KEYI_H_
#define _KEYI_H_


SEC_BEGIN_PROTOS



KeyType seckey_GetKeyType(SECOidTag pubKeyOid);




SECStatus sec_DecodeSigAlg(const SECKEYPublicKey *key, SECOidTag sigAlg,
             const SECItem *param, SECOidTag *encalg, SECOidTag *hashalg);

SEC_END_PROTOS

#endif 
