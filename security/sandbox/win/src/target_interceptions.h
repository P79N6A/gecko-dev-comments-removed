



#include "sandbox/win/src/nt_internals.h"
#include "sandbox/win/src/sandbox_types.h"

#ifndef SANDBOX_SRC_TARGET_INTERCEPTIONS_H__
#define SANDBOX_SRC_TARGET_INTERCEPTIONS_H__

namespace sandbox {

extern "C" {




SANDBOX_INTERCEPT NTSTATUS WINAPI TargetNtMapViewOfSection(
    NtMapViewOfSectionFunction orig_MapViewOfSection, HANDLE section,
    HANDLE process, PVOID *base, ULONG_PTR zero_bits, SIZE_T commit_size,
    PLARGE_INTEGER offset, PSIZE_T view_size, SECTION_INHERIT inherit,
    ULONG allocation_type, ULONG protect);




SANDBOX_INTERCEPT NTSTATUS WINAPI TargetNtUnmapViewOfSection(
    NtUnmapViewOfSectionFunction orig_UnmapViewOfSection, HANDLE process,
    PVOID base);

}  

}  

#endif  
