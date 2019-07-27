



#ifndef SANDBOX_WOW_HELPER_TARGET_CODE_H__
#define SANDBOX_WOW_HELPER_TARGET_CODE_H__

#include "sandbox/win/src/nt_internals.h"

namespace sandbox {

extern "C" {




struct PatchInfo {
  HANDLE dll_load;  
  HANDLE continue_load;  
  HANDLE section;  
  NtMapViewOfSectionFunction orig_MapViewOfSection;
  NtSignalAndWaitForSingleObjectFunction signal_and_wait;
  void* patch_location;
};




NTSTATUS WINAPI TargetNtMapViewOfSection(
    PatchInfo* patch_info, HANDLE process, PVOID* base, ULONG_PTR zero_bits,
    SIZE_T commit_size, PLARGE_INTEGER offset, PSIZE_T view_size,
    SECTION_INHERIT inherit, ULONG allocation_type, ULONG protect);


NTSTATUS WINAPI TargetEnd();

} 

}  

#endif  
