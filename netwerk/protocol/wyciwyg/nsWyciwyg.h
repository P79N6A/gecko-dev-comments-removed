



#ifndef nsWyciwyg_h__
#define nsWyciwyg_h__

#include "mozilla/net/NeckoChild.h"


#undef LOG

#include "mozilla/Logging.h"












extern PRLogModuleInfo *gWyciwygLog;


#define LOG1(args) MOZ_LOG(gWyciwygLog, mozilla::LogLevel::Error, args)
#define LOG2(args) MOZ_LOG(gWyciwygLog, mozilla::LogLevel::Warning, args)
#define LOG3(args) MOZ_LOG(gWyciwygLog, mozilla::LogLevel::Info, args)
#define LOG4(args) MOZ_LOG(gWyciwygLog, mozilla::LogLevel::Debug, args)
#define LOG(args) LOG4(args)

#define LOG1_ENABLED() MOZ_LOG_TEST(gWyciwygLog, mozilla::LogLevel::Error)
#define LOG2_ENABLED() MOZ_LOG_TEST(gWyciwygLog, mozilla::LogLevel::Warning)
#define LOG3_ENABLED() MOZ_LOG_TEST(gWyciwygLog, mozilla::LogLevel::Info)
#define LOG4_ENABLED() MOZ_LOG_TEST(gWyciwygLog, mozilla::LogLevel::Debug)
#define LOG_ENABLED() LOG4_ENABLED()

#define WYCIWYG_TYPE "text/html"

#endif 
