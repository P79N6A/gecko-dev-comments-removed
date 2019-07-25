




































#ifndef nsWyciwyg_h__
#define nsWyciwyg_h__

#if defined(MOZ_LOGGING)
#define FORCE_PR_LOG
#endif











#if defined(PR_LOG) && !defined(ALLOW_LATE_NSHTTP_H_INCLUDE)
#error "If nsWyciwyg.h #included it must come before any IPDL-generated files or other files that #include prlog.h"
#endif
#include "mozilla/net/NeckoChild.h"
#undef LOG

#include "plstr.h"
#include "prlog.h"
#include "prtime.h"

#if defined(PR_LOGGING)











extern PRLogModuleInfo *gWyciwygLog;
#endif


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
