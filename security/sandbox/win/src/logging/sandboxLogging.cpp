





#include "sandboxLogging.h"

#include "base/strings/utf_string_conversions.h"
#include "sandbox/win/src/sandbox_policy.h"

namespace mozilla {
namespace sandboxing {

void
ApplyLoggingPolicy(sandbox::TargetPolicy& aPolicy)
{
  
  
  
  
  aPolicy.AddRule(sandbox::TargetPolicy::SUBSYS_NAMED_PIPES,
                  sandbox::TargetPolicy::NAMEDPIPES_ALLOW_ANY, L"dummy");
  aPolicy.AddRule(sandbox::TargetPolicy::SUBSYS_PROCESS,
                  sandbox::TargetPolicy::PROCESS_MIN_EXEC, L"dummy");
  aPolicy.AddRule(sandbox::TargetPolicy::SUBSYS_REGISTRY,
                  sandbox::TargetPolicy::REG_ALLOW_READONLY,
                  L"HKEY_CURRENT_USER\\dummy");
  aPolicy.AddRule(sandbox::TargetPolicy::SUBSYS_SYNC,
                  sandbox::TargetPolicy::EVENTS_ALLOW_READONLY, L"dummy");
  aPolicy.AddRule(sandbox::TargetPolicy::SUBSYS_HANDLES,
                  sandbox::TargetPolicy::HANDLES_DUP_BROKER, L"dummy");
}

static LogFunction sLogFunction = nullptr;

void
ProvideLogFunction(LogFunction aLogFunction)
{
  sLogFunction = aLogFunction;
}

void
LogBlocked(const char* aFunctionName, const char* aContext, uint32_t aFramesToSkip)
{
  if (sLogFunction) {
    sLogFunction("BLOCKED", aFunctionName, aContext,
                  true, aFramesToSkip);
  }
}

void
LogBlocked(const char* aFunctionName, const wchar_t* aContext)
{
  if (sLogFunction) {
    
    LogBlocked(aFunctionName, base::WideToUTF8(aContext).c_str(),
                3);
  }
}

void
LogBlocked(const char* aFunctionName, const wchar_t* aContext,
           uint16_t aLength)
{
  if (sLogFunction) {
    
    LogBlocked(aFunctionName,
               base::WideToUTF8(std::wstring(aContext, aLength)).c_str(),
                3);
  }
}

void
LogAllowed(const char* aFunctionName, const char* aContext)
{
  if (sLogFunction) {
    sLogFunction("Broker ALLOWED", aFunctionName, aContext,
                  false,  0);
  }
}

void
LogAllowed(const char* aFunctionName, const wchar_t* aContext)
{
  if (sLogFunction) {
    LogAllowed(aFunctionName, base::WideToUTF8(aContext).c_str());
  }
}

void
LogAllowed(const char* aFunctionName, const wchar_t* aContext,
           uint16_t aLength)
{
  if (sLogFunction) {
    LogAllowed(aFunctionName,
               base::WideToUTF8(std::wstring(aContext, aLength)).c_str());
  }
}

} 
} 
