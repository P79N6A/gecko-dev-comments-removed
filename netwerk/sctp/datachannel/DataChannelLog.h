





#ifndef DataChannelLog_h
#define DataChannelLog_h

#ifdef MOZ_LOGGING
#define FORCE_PR_LOG
#endif

#if defined(PR_LOG)
#error "This file must be #included before any IPDL-generated files or other files that #include prlog.h"
#endif

#include "base/basictypes.h"
#include "prlog.h"

#ifdef PR_LOGGING
extern PRLogModuleInfo* GetDataChannelLog();
extern PRLogModuleInfo* GetSCTPLog();
#endif

#undef LOG
#define LOG(args) PR_LOG(GetDataChannelLog(), PR_LOG_DEBUG, args)

#endif
