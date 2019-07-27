





#ifndef DataChannelLog_h
#define DataChannelLog_h

#include "base/basictypes.h"
#include "prlog.h"

#ifdef PR_LOGGING
extern PRLogModuleInfo* GetDataChannelLog();
extern PRLogModuleInfo* GetSCTPLog();
#endif

#undef LOG
#define LOG(args) PR_LOG(GetDataChannelLog(), PR_LOG_DEBUG, args)

#endif
