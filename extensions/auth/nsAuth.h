



#ifndef nsAuth_h__
#define nsAuth_h__


enum pType {
     PACKAGE_TYPE_KERBEROS,
     PACKAGE_TYPE_NEGOTIATE,
     PACKAGE_TYPE_NTLM
};

#include "mozilla/Logging.h"







extern PRLogModuleInfo* gNegotiateLog;

#define LOG(args) PR_LOG(gNegotiateLog, PR_LOG_DEBUG, args)

#endif 
