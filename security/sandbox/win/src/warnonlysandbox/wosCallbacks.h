





#ifndef security_sandbox_wosEnableCallbacks_h__
#define security_sandbox_wosEnableCallbacks_h__

#include "mozilla/warnonlysandbox/wosTypes.h"
#include "mozilla/warnonlysandbox/warnOnlySandbox.h"

#ifdef TARGET_SANDBOX_EXPORTS
#include <sstream>
#include <iostream>

#include "mozilla/Preferences.h"
#include "nsContentUtils.h"
#include "nsStackWalk.h"
#endif

#ifdef TARGET_SANDBOX_EXPORTS
#define TARGET_SANDBOX_EXPORT __declspec(dllexport)
#else
#define TARGET_SANDBOX_EXPORT __declspec(dllimport)
#endif

namespace mozilla {
namespace warnonlysandbox {







void TARGET_SANDBOX_EXPORT
SetProvideLogFunctionCb(ProvideLogFunctionCb aProvideLogFunctionCb);


static void
PrepareForInit()
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

static uint32_t sStackTraceDepth = 0;


static void
StackFrameToOStringStream(void* aPC, void* aSP, void* aClosure)
{
  std::ostringstream* stream = static_cast<std::ostringstream*>(aClosure);
  nsCodeAddressDetails details;
  char buf[1024];
  NS_DescribeCodeAddress(aPC, &details);
  NS_FormatCodeAddressDetails(aPC, &details, buf, sizeof(buf));
  *stream << "--" << buf;
}


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
  msgStream << std::endl;

  if (aShouldLogStackTrace) {
    if (sStackTraceDepth) {
      msgStream << "Stack Trace:" << std::endl;
      NS_StackWalk(StackFrameToOStringStream, aFramesToSkip, sStackTraceDepth,
                   &msgStream, 0, nullptr);
    }
  }

  std::string msg = msgStream.str();
#ifdef DEBUG
  std::cerr << msg;
#endif

  nsContentUtils::LogMessageToConsole(msg.c_str());
}


static void
InitIfRequired()
{
  if (XRE_GetProcessType() == GeckoProcessType_Content
      && Preferences::GetString("browser.tabs.remote.sandbox").EqualsLiteral("warn")
      && sProvideLogFunctionCb) {
    Preferences::AddUintVarCache(&sStackTraceDepth,
      "browser.tabs.remote.sandbox.warnOnlyStackTraceDepth");
    sProvideLogFunctionCb(Log);
  }
}
#endif
} 
} 

#endif 
