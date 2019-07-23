



































#ifndef nsAuth_h__
#define nsAuth_h__


enum pType {
     PACKAGE_TYPE_KERBEROS,
     PACKAGE_TYPE_NEGOTIATE,
     PACKAGE_TYPE_NTLM
};

#if defined(MOZ_LOGGING)
#define FORCE_PR_LOG
#endif

#include "prlog.h"

#if defined( PR_LOGGING )






extern PRLogModuleInfo* gNegotiateLog;

#define LOG(args) PR_LOG(gNegotiateLog, PR_LOG_DEBUG, args)
#else
#define LOG(args)
#endif

#endif 
