



#ifndef nsWyciwyg_h__
#define nsWyciwyg_h__

#include "mozilla/net/NeckoChild.h"


#undef LOG

#include "mozilla/Logging.h"












extern PRLogModuleInfo *gWyciwygLog;


#define LOG1(args) MOZ_LOG(gWyciwygLog, PR_LOG_ERROR, args)
#define LOG2(args) MOZ_LOG(gWyciwygLog, PR_LOG_WARNING, args)
#define LOG3(args) MOZ_LOG(gWyciwygLog, PR_LOG_INFO, args)
#define LOG4(args) MOZ_LOG(gWyciwygLog, PR_LOG_DEBUG, args)
#define LOG(args) LOG4(args)

#define LOG1_ENABLED() MOZ_LOG_TEST(gWyciwygLog, PR_LOG_ERROR)
#define LOG2_ENABLED() MOZ_LOG_TEST(gWyciwygLog, PR_LOG_WARNING)
#define LOG3_ENABLED() MOZ_LOG_TEST(gWyciwygLog, PR_LOG_INFO)
#define LOG4_ENABLED() MOZ_LOG_TEST(gWyciwygLog, PR_LOG_DEBUG)
#define LOG_ENABLED() LOG4_ENABLED()

#define WYCIWYG_TYPE "text/html"

#endif 
