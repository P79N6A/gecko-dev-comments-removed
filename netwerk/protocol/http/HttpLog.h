





#ifndef HttpLog_h__
#define HttpLog_h__























#if defined(PR_LOG) && !defined(ALLOW_LATE_HTTPLOG_H_INCLUDE)
#error "If HttpLog.h #included it must come before any IPDL-generated files or other files that #include prlog.h"
#endif



#if defined(MOZ_LOGGING)
#define FORCE_PR_LOG
#endif

#include "mozilla/net/NeckoChild.h"


#undef LOG

#if defined(PR_LOGGING)











extern PRLogModuleInfo *gHttpLog;
#endif


#define LOG1(args) PR_LOG(gHttpLog, 1, args)
#define LOG2(args) PR_LOG(gHttpLog, 2, args)
#define LOG3(args) PR_LOG(gHttpLog, 3, args)
#define LOG4(args) PR_LOG(gHttpLog, 4, args)
#define LOG(args) LOG4(args)

#define LOG1_ENABLED() PR_LOG_TEST(gHttpLog, 1)
#define LOG2_ENABLED() PR_LOG_TEST(gHttpLog, 2)
#define LOG3_ENABLED() PR_LOG_TEST(gHttpLog, 3)
#define LOG4_ENABLED() PR_LOG_TEST(gHttpLog, 4)
#define LOG_ENABLED() LOG4_ENABLED()

#endif 
