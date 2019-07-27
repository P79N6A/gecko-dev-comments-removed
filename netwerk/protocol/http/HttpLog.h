





#ifndef HttpLog_h__
#define HttpLog_h__












#include "mozilla/net/NeckoChild.h"


#undef LOG












extern PRLogModuleInfo *gHttpLog;


#define LOG1(args) MOZ_LOG(gHttpLog, mozilla::LogLevel::Error, args)
#define LOG2(args) MOZ_LOG(gHttpLog, mozilla::LogLevel::Warning, args)
#define LOG3(args) MOZ_LOG(gHttpLog, mozilla::LogLevel::Info, args)
#define LOG4(args) MOZ_LOG(gHttpLog, mozilla::LogLevel::Debug, args)
#define LOG5(args) MOZ_LOG(gHttpLog, mozilla::LogLevel::Verbose, args)
#define LOG(args) LOG4(args)

#define LOG1_ENABLED() MOZ_LOG_TEST(gHttpLog, mozilla::LogLevel::Error)
#define LOG2_ENABLED() MOZ_LOG_TEST(gHttpLog, mozilla::LogLevel::Warning)
#define LOG3_ENABLED() MOZ_LOG_TEST(gHttpLog, mozilla::LogLevel::Info)
#define LOG4_ENABLED() MOZ_LOG_TEST(gHttpLog, mozilla::LogLevel::Debug)
#define LOG5_ENABLED() MOZ_LOG_TEST(gHttpLog, mozilla::LogLevel::Verbose)
#define LOG_ENABLED() LOG4_ENABLED()

#endif 
