





#ifndef security_sandbox_loggingCallbacks_h__
#define security_sandbox_loggingCallbacks_h__

#include "mozilla/sandboxing/loggingTypes.h"
#include "mozilla/sandboxing/sandboxLogging.h"

#ifdef TARGET_SANDBOX_EXPORTS
#include <sstream>
#include <iostream>

#include "mozilla/Preferences.h"
#include "nsContentUtils.h"

#ifdef MOZ_STACKWALKING
#include "mozilla/StackWalk.h"
#endif

#define TARGET_SANDBOX_EXPORT __declspec(dllexport)
#else
#define TARGET_SANDBOX_EXPORT __declspec(dllimport)
#endif

namespace mozilla {
namespace sandboxing {







void TARGET_SANDBOX_EXPORT
SetProvideLogFunctionCb(ProvideLogFunctionCb aProvideLogFunctionCb);


static void
PrepareForLogging()
{
  SetProvideLogFunctionCb(ProvideLogFunction);
}

#ifdef TARGET_SANDBOX_EXPORTS
static ProvideLogFunctionCb sProvideLogFunctionCb = nullptr;

void
SetProvideLogFunctionCb(ProvideLogFunctionCb aProvideLogFunctionCb)
{
  sProvideLogFunctionCb = aProvideLogFunctionCb;
}

#ifdef MOZ_STACKWALKING
static uint32_t sStackTraceDepth = 0;


static void
StackFrameToOStringStream(uint32_t aFrameNumber, void* aPC, void* aSP,
                          void* aClosure)
{
  std::ostringstream* stream = static_cast<std::ostringstream*>(aClosure);
  MozCodeAddressDetails details;
  char buf[1024];
  MozDescribeCodeAddress(aPC, &details);
  MozFormatCodeAddressDetails(buf, sizeof(buf), aFrameNumber, aPC, &details);
  *stream << std::endl << "--" << buf;
  stream->flush();
}
#endif


static void
Log(const char* aMessageType,
    const char* aFunctionName,
    const char* aContext,
    const bool aShouldLogStackTrace = false,
    uint32_t aFramesToSkip = 0)
{
  std::ostringstream msgStream;
  msgStream << "Process Sandbox " << aMessageType << ": " << aFunctionName;
  if (aContext) {
    msgStream << " for : " << aContext;
  }

#ifdef MOZ_STACKWALKING
  if (aShouldLogStackTrace) {
    if (sStackTraceDepth) {
      msgStream << std::endl << "Stack Trace:";
      MozStackWalk(StackFrameToOStringStream, aFramesToSkip, sStackTraceDepth,
                   &msgStream, 0, nullptr);
    }
  }
#endif

  std::string msg = msgStream.str();
#if defined(DEBUG)
  
  
  NS_DebugBreak(NS_DEBUG_WARNING, nullptr, msg.c_str(), nullptr, -1);
#endif

  if (nsContentUtils::IsInitialized()) {
    nsContentUtils::LogMessageToConsole(msg.c_str());
  }
}


static void
InitLoggingIfRequired()
{
  if (!sProvideLogFunctionCb) {
    return;
  }

  if (Preferences::GetBool("security.sandbox.windows.log") ||
      PR_GetEnv("MOZ_WIN_SANDBOX_LOGGING")) {
    sProvideLogFunctionCb(Log);

#if defined(MOZ_CONTENT_SANDBOX) && defined(MOZ_STACKWALKING)
    
    
    if (XRE_IsContentProcess()) {
      Preferences::AddUintVarCache(&sStackTraceDepth,
        "security.sandbox.windows.log.stackTraceDepth");
    }
#endif
  }
}
#endif
} 
} 

#endif 
