













































#ifndef _nsCache_h_
#define _nsCache_h_

#include "nsISupports.h"
#include "nsIFile.h"
#include "nsAString.h"
#include "prtime.h"
#include "nsError.h"
#include "prlog.h"


#if defined(PR_LOGGING)
extern PRLogModuleInfo * gCacheLog;
void   CacheLogInit();
void   CacheLogPrintPath(PRLogModuleLevel level,
                         char *           format,
                         nsIFile *        item);
#define CACHE_LOG_INIT()        CacheLogInit()
#define CACHE_LOG_ALWAYS(args)  PR_LOG(gCacheLog, PR_LOG_ALWAYS, args)
#define CACHE_LOG_ERROR(args)   PR_LOG(gCacheLog, PR_LOG_ERROR, args)
#define CACHE_LOG_WARNING(args) PR_LOG(gCacheLog, PR_LOG_WARNING, args)
#define CACHE_LOG_DEBUG(args)   PR_LOG(gCacheLog, PR_LOG_DEBUG, args)
#define CACHE_LOG_PATH(level, format, item) \
                                CacheLogPrintPath(level, format, item)
#else
#define CACHE_LOG_INIT()        {}
#define CACHE_LOG_ALWAYS(args)  {}
#define CACHE_LOG_ERROR(args)   {}
#define CACHE_LOG_WARNING(args) {}
#define CACHE_LOG_DEBUG(args)   {}
#define CACHE_LOG_PATH(level, format, item)  {}
#endif


extern PRUint32  SecondsFromPRTime(PRTime prTime);
extern PRTime    PRTimeFromSeconds(PRUint32 seconds);


extern nsresult  ClientIDFromCacheKey(const nsACString&  key, char ** result);
extern nsresult  ClientKeyFromCacheKey(const nsCString& key, nsACString &result);


#endif 
