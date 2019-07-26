






































#ifndef _CSF_WEBRTC_LOGGING_H
#define _CSF_WEBRTC_LOGGING_H

#include "CSFLog.h"
#include <stdarg.h>


#ifndef _USE_CPVE

extern bool g_IncludeWebrtcLogging;

enum log_level {GERROR, GWARNING, GINFO, GDEBUG};

#define LOG_WEBRTC_ERROR CSFLogError
#define LOG_WEBRTC_WARN  CSFLogWarn
#define LOG_WEBRTC_INFO  CSFLogInfo
#define LOG_WEBRTC_DEBUG CSFLogDebug

#endif
#endif
