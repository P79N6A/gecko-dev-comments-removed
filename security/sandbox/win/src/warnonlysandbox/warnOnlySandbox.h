











#ifndef security_sandbox_warnOnlySandbox_h__
#define security_sandbox_warnOnlySandbox_h__

#include "wosTypes.h"

#ifdef SANDBOX_EXPORTS
#define SANDBOX_EXPORT __declspec(dllexport)
#else
#define SANDBOX_EXPORT __declspec(dllimport)
#endif

namespace sandbox {
class TargetPolicy;
}

namespace mozilla {
namespace warnonlysandbox {



void SANDBOX_EXPORT ProvideLogFunction(LogFunction aLogFunction);


void ApplyWarnOnlyPolicy(sandbox::TargetPolicy& aPolicy);





void LogBlocked(const char* aFunctionName, const char* aContext = nullptr,
                uint32_t aFramesToSkip = 2);


void LogBlocked(const char* aFunctionName, const wchar_t* aContext);
void LogBlocked(const char* aFunctionName, const wchar_t* aContext,
                uint16_t aLength);


void LogAllowed(const char* aFunctionName, const char* aContext = nullptr);


void LogAllowed(const char* aFunctionName, const wchar_t* aContext);
void LogAllowed(const char* aFunctionName, const wchar_t* aContext,
                uint16_t aLength);


} 
} 

#endif 
