



#include "sandbox/win/src/nt_internals.h"
#include "sandbox/win/src/sandbox_types.h"

#ifndef SANDBOX_SRC_FILESYSTEM_INTERCEPTION_H__
#define SANDBOX_SRC_FILESYSTEM_INTERCEPTION_H__

namespace sandbox {

extern "C" {


SANDBOX_INTERCEPT NTSTATUS WINAPI TargetNtCreateFile(
    NtCreateFileFunction orig_CreateFile, PHANDLE file,
    ACCESS_MASK desired_access, POBJECT_ATTRIBUTES object_attributes,
    PIO_STATUS_BLOCK io_status, PLARGE_INTEGER allocation_size,
    ULONG file_attributes, ULONG sharing, ULONG disposition, ULONG options,
    PVOID ea_buffer, ULONG ea_length);


SANDBOX_INTERCEPT NTSTATUS WINAPI TargetNtOpenFile(
    NtOpenFileFunction orig_OpenFile, PHANDLE file, ACCESS_MASK desired_access,
    POBJECT_ATTRIBUTES object_attributes, PIO_STATUS_BLOCK io_status,
    ULONG sharing, ULONG options);



SANDBOX_INTERCEPT NTSTATUS WINAPI TargetNtQueryAttributesFile(
    NtQueryAttributesFileFunction orig_QueryAttributes,
    POBJECT_ATTRIBUTES object_attributes,
    PFILE_BASIC_INFORMATION file_attributes);



SANDBOX_INTERCEPT NTSTATUS WINAPI TargetNtQueryFullAttributesFile(
    NtQueryFullAttributesFileFunction orig_QueryAttributes,
    POBJECT_ATTRIBUTES object_attributes,
    PFILE_NETWORK_OPEN_INFORMATION file_attributes);


SANDBOX_INTERCEPT NTSTATUS WINAPI TargetNtSetInformationFile(
    NtSetInformationFileFunction orig_SetInformationFile, HANDLE file,
    PIO_STATUS_BLOCK io_status, PVOID file_information, ULONG length,
    FILE_INFORMATION_CLASS file_information_class);

}  

}  

#endif  
