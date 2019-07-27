





#ifndef HttpLog_h__
#define HttpLog_h__












#include "mozilla/net/NeckoChild.h"


#undef LOG












extern PRLogModuleInfo *gHttpLog;


#define LOG1(args) MOZ_LOG(gHttpLog, 1, args)
#define LOG2(args) MOZ_LOG(gHttpLog, 2, args)
#define LOG3(args) MOZ_LOG(gHttpLog, 3, args)
#define LOG4(args) MOZ_LOG(gHttpLog, 4, args)
#define LOG5(args) MOZ_LOG(gHttpLog, 5, args)
#define LOG(args) LOG4(args)

#define LOG1_ENABLED() PR_LOG_TEST(gHttpLog, 1)
#define LOG2_ENABLED() PR_LOG_TEST(gHttpLog, 2)
#define LOG3_ENABLED() PR_LOG_TEST(gHttpLog, 3)
#define LOG4_ENABLED() PR_LOG_TEST(gHttpLog, 4)
#define LOG5_ENABLED() PR_LOG_TEST(gHttpLog, 5)
#define LOG_ENABLED() LOG4_ENABLED()

#endif 
