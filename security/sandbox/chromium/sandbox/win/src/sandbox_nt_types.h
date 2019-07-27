



#ifndef SANDBOX_SRC_SANDBOX_NT_TYPES_H__
#define SANDBOX_SRC_SANDBOX_NT_TYPES_H__

#include "sandbox/win/src/nt_internals.h"

namespace sandbox {

struct NtExports {
  NtAllocateVirtualMemoryFunction       AllocateVirtualMemory;
  NtCloseFunction                       Close;
  NtDuplicateObjectFunction             DuplicateObject;
  NtFreeVirtualMemoryFunction           FreeVirtualMemory;
  NtMapViewOfSectionFunction            MapViewOfSection;
  NtProtectVirtualMemoryFunction        ProtectVirtualMemory;
  NtQueryInformationProcessFunction     QueryInformationProcess;
  NtQueryObjectFunction                 QueryObject;
  NtQuerySectionFunction                QuerySection;
  NtQueryVirtualMemoryFunction          QueryVirtualMemory;
  NtUnmapViewOfSectionFunction          UnmapViewOfSection;
  RtlAllocateHeapFunction               RtlAllocateHeap;
  RtlAnsiStringToUnicodeStringFunction  RtlAnsiStringToUnicodeString;
  RtlCompareUnicodeStringFunction       RtlCompareUnicodeString;
  RtlCreateHeapFunction                 RtlCreateHeap;
  RtlCreateUserThreadFunction           RtlCreateUserThread;
  RtlDestroyHeapFunction                RtlDestroyHeap;
  RtlFreeHeapFunction                   RtlFreeHeap;
  _strnicmpFunction                     _strnicmp;
  strlenFunction                        strlen;
  wcslenFunction                        wcslen;
  memcpyFunction                        memcpy;
};


enum AllocationType {
  NT_ALLOC,
  NT_PAGE
};

}  


#endif  
