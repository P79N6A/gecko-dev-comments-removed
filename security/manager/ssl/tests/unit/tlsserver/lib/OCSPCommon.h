






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
  ORTUnauthorized      
};

struct OCSPHost
{
  const char *mHostName;
  OCSPResponseType mORT;
};

SECItemArray *
GetOCSPResponseForType(OCSPResponseType aORT, CERTCertificate *aCert,
                       PLArenaPool *aArena);

#endif 
