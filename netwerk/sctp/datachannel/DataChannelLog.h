





#ifndef DataChannelLog_h
#define DataChannelLog_h

#include "base/basictypes.h"
#include "mozilla/Logging.h"

extern PRLogModuleInfo* GetDataChannelLog();
extern PRLogModuleInfo* GetSCTPLog();

#undef LOG
#define LOG(args) PR_LOG(GetDataChannelLog(), PR_LOG_DEBUG, args)

#endif
