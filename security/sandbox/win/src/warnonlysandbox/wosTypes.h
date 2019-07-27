





#ifndef security_sandbox_wosTypes_h__
#define security_sandbox_wosTypes_h__

#include <stdint.h>

namespace mozilla {
namespace warnonlysandbox {



typedef void (*LogFunction) (const char* aMessageType,
                             const char* aFunctionName,
                             const char* aContext,
                             const bool aShouldLogStackTrace,
                             uint32_t aFramesToSkip);
typedef void (*ProvideLogFunctionCb) (LogFunction aLogFunction);

} 
} 

#endif 
