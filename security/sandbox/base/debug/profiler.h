



#ifndef BASE_DEBUG_PROFILER_H
#define BASE_DEBUG_PROFILER_H

#include <string>

#include "base/base_export.h"
#include "base/basictypes.h"





namespace base {
namespace debug {




BASE_EXPORT void StartProfiling(const std::string& name);


BASE_EXPORT void StopProfiling();


BASE_EXPORT void FlushProfiling();


BASE_EXPORT bool BeingProfiled();


BASE_EXPORT void RestartProfilingAfterFork();


BASE_EXPORT bool IsBinaryInstrumented();
















typedef uintptr_t (*ReturnAddressLocationResolver)(
    uintptr_t return_addr_location);


typedef void (*DynamicFunctionEntryHook)(uintptr_t function,
                                         uintptr_t return_addr_location);








typedef void (*DynamicFunctionEntryHook)(uintptr_t function,
                                         uintptr_t return_addr_location);

typedef void (*AddDynamicSymbol)(const void* address,
                                 size_t length,
                                 const char* name,
                                 size_t name_len);
typedef void (*MoveDynamicSymbol)(const void* address, const void* new_address);





BASE_EXPORT ReturnAddressLocationResolver GetProfilerReturnAddrResolutionFunc();
BASE_EXPORT DynamicFunctionEntryHook GetProfilerDynamicFunctionEntryHookFunc();
BASE_EXPORT AddDynamicSymbol GetProfilerAddDynamicSymbolFunc();
BASE_EXPORT MoveDynamicSymbol GetProfilerMoveDynamicSymbolFunc();

}  
}  

#endif  
