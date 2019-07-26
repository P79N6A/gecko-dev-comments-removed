






#ifndef OCSPCommon_h
#define OCSPCommon_h

#include "certt.h"
#include "seccomon.h"

enum OCSPResponseType
{
  ORTNull = 0,
  ORTGood,             
  ORTRevoked,          
  ORTUnknown,          
  ORTGoodOtherCert,    
  ORTGoodOtherCA,      
  ORTExpired,          
  ORTExpiredFreshCA,   
  ORTNone,             
  ORTEmpty,            
  ORTMalformed,        
  ORTSrverr,           
  ORTTryLater,         
  ORTNeedsSig,         
  ORTUnauthorized,     
  ORTBadSignature,     
  ORTSkipResponseBytes, 
  ORTCriticalExtension, 
  ORTNoncriticalExtension, 
  ORTEmptyExtensions,  
  ORTDelegatedIncluded, 
  ORTDelegatedIncludedLast, 
  ORTDelegatedMissing, 
  ORTDelegatedMissingMultiple, 
  ORTLongValidityAlmostExpired, 
  ORTAncientAlmostExpired, 
};

struct OCSPHost
{
  const char *mHostName;
  OCSPResponseType mORT;
  const char *mAdditionalCertName; 
};

SECItemArray *
GetOCSPResponseForType(OCSPResponseType aORT, CERTCertificate *aCert,
                       PLArenaPool *aArena, const char *aAdditionalCertName);

#endif 
