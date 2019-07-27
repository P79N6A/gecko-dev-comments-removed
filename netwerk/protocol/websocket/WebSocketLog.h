





#ifndef WebSocketLog_h
#define WebSocketLog_h

#include "base/basictypes.h"
#include "prlog.h"
#include "mozilla/net/NeckoChild.h"

#ifdef PR_LOGGING
extern PRLogModuleInfo* webSocketLog;
#endif

#undef LOG
#define LOG(args) PR_LOG(webSocketLog, PR_LOG_DEBUG, args)

#endif
