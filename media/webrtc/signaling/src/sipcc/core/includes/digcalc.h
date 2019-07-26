






































#ifndef _DIGCALC_H_
#define _DIGCALC_H_

#define HASHLEN 16
typedef char HASH[HASHLEN];
#define HASHHEXLEN 32
typedef char HASHHEX[HASHHEXLEN + 1];

#define IN
#define OUT


void DigestCalcHA1(
    IN char *pszAlg,
    IN char *pszUserName,
    IN char *pszRealm,
    IN char *pszPassword,
    IN char *pszNonce,
    IN char *pszCNonce,
    OUT HASHHEX SessionKey
    );


void DigestCalcResponse(
    IN HASHHEX HA1,         
    IN char *pszNonce,      
    IN char *pszNonceCount, 
    IN char *pszCNonce,     
    IN char *pszQop,        
    IN char *pszMethod,     
    IN char *pszDigestUri,  
    IN HASHHEX HEntity,     
    OUT HASHHEX Response    
    );

void DigestString(char *string, HASHHEX response);

#endif
