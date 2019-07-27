



#ifndef nsWyciwyg_h__
#define nsWyciwyg_h__

#include "mozilla/net/NeckoChild.h"


#undef LOG

#include "mozilla/Logging.h"












extern PRLogModuleInfo *gWyciwygLog;


#define LOG1(args) PR_LOG(gWyciwygLog, 1, args)
#define LOG2(args) PR_LOG(gWyciwygLog, 2, args)
#define LOG3(args) PR_LOG(gWyciwygLog, 3, args)
#define LOG4(args) PR_LOG(gWyciwygLog, 4, args)
#define LOG(args) LOG4(args)

#define LOG1_ENABLED() PR_LOG_TEST(gWyciwygLog, 1)
#define LOG2_ENABLED() PR_LOG_TEST(gWyciwygLog, 2)
#define LOG3_ENABLED() PR_LOG_TEST(gWyciwygLog, 3)
#define LOG4_ENABLED() PR_LOG_TEST(gWyciwygLog, 4)
#define LOG_ENABLED() LOG4_ENABLED()

#define WYCIWYG_TYPE "text/html"

#endif 
