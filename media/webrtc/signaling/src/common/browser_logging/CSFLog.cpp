



#include <stdio.h>
#include <string.h>
#include <stdarg.h>

#include "CSFLog.h"
#include "base/basictypes.h"

#include <map>
#include "prrwlock.h"
#include "prthread.h"
#include "nsThreadUtils.h"

static PRLogModuleInfo *gLogModuleInfo = nullptr;

PRLogModuleInfo *GetSignalingLogInfo()
{
  if (gLogModuleInfo == nullptr)
    gLogModuleInfo = PR_NewLogModule("signaling");

  return gLogModuleInfo;
}

static PRLogModuleInfo *gWebRTCLogModuleInfo = nullptr;

PRLogModuleInfo *GetWebRTCLogInfo()
{
  if (gWebRTCLogModuleInfo == nullptr)
    gWebRTCLogModuleInfo = PR_NewLogModule("webrtc_trace");

  return gWebRTCLogModuleInfo;
}


void CSFLogV(CSFLogLevel priority, const char* sourceFile, int sourceLine, const char* tag , const char* format, va_list args)
{
#ifdef STDOUT_LOGGING
  printf("%s\n:",tag);
  vprintf(format, args);
#else

  PRLogModuleLevel level = static_cast<PRLogModuleLevel>(priority);

  GetSignalingLogInfo();

  
  if (!PR_LOG_TEST(gLogModuleInfo,level)) {
    return;
  }

  
  const char *lastSlash = sourceFile;
  while (*sourceFile) {
    if (*sourceFile == '/' || *sourceFile == '\\') {
      lastSlash = sourceFile;
    }
    sourceFile++;
  }
  sourceFile = lastSlash;
  if (*sourceFile == '/' || *sourceFile == '\\') {
    sourceFile++;
  }

#define MAX_MESSAGE_LENGTH 1024
  char message[MAX_MESSAGE_LENGTH];

  const char *threadName = NULL;

  
  if (NS_IsMainThread()) {
    threadName = "main";
  } else {
    threadName = PR_GetThreadName(PR_GetCurrentThread());
  }

  
  if (!threadName) {
    threadName = "";
  }

  vsnprintf(message, MAX_MESSAGE_LENGTH, format, args);
  PR_LOG(gLogModuleInfo, level, ("[%s|%s] %s:%d: %s",
                                  threadName, tag, sourceFile, sourceLine,
                                  message));
#endif

}

void CSFLog( CSFLogLevel priority, const char* sourceFile, int sourceLine, const char* tag , const char* format, ...)
{
	va_list ap;
  va_start(ap, format);

  CSFLogV(priority, sourceFile, sourceLine, tag, format, ap);
  va_end(ap);
}

