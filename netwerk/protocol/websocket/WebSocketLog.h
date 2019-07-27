





#ifndef WebSocketLog_h
#define WebSocketLog_h

#include "base/basictypes.h"
#include "mozilla/Logging.h"
#include "mozilla/net/NeckoChild.h"

extern PRLogModuleInfo* webSocketLog;

#undef LOG
#define LOG(args) PR_LOG(webSocketLog, PR_LOG_DEBUG, args)

#endif
