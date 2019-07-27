



#include "sandbox/win/wow_helper/target_code.h"

namespace sandbox {


NTSTATUS WINAPI TargetNtMapViewOfSection(
    PatchInfo *patch_info, HANDLE process, PVOID *base, ULONG_PTR zero_bits,
    SIZE_T commit_size, PLARGE_INTEGER offset, PSIZE_T view_size,
    SECTION_INHERIT inherit, ULONG allocation_type, ULONG protect) {
  NTSTATUS ret = patch_info->orig_MapViewOfSection(patch_info->section, process,
                                                   base, zero_bits, commit_size,
                                                   offset, view_size, inherit,
                                                   allocation_type, protect);

  LARGE_INTEGER timeout;
  timeout.QuadPart = -(5 * 10000000);  

  
  patch_info->signal_and_wait(patch_info->dll_load, patch_info->continue_load,
                              TRUE, &timeout);

  return ret;
}


NTSTATUS WINAPI TargetEnd() {
  return STATUS_SUCCESS;
}

}  
