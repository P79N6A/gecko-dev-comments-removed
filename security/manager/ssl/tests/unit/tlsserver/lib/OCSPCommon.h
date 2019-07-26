






#ifndef OCSPCommon_h
#define OCSPCommon_h

#include "certt.h"
#include "seccomon.h"

enum OCSPStapleResponseType
{
  OSRTNull = 0,
  OSRTGood,             
  OSRTRevoked,          
  OSRTUnknown,          
  OSRTGoodOtherCert,    
  OSRTGoodOtherCA,      
  OSRTExpired,          
  OSRTExpiredFreshCA,   
  OSRTNone,             
  OSRTEmpty,            
  OSRTMalformed,        
  OSRTSrverr,           
  OSRTTryLater,         
  OSRTNeedsSig,         
  OSRTUnauthorized      
};

struct OCSPHost
{
  const char *mHostName;
  OCSPStapleResponseType mOSRT;
};

SECItemArray *
GetOCSPResponseForType(OCSPStapleResponseType aOSRT, CERTCertificate *aCert,
                       PLArenaPool *aArena);

#endif 
