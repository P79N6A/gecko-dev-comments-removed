



#include "sandbox/win/src/nt_internals.h"
#include "sandbox/win/src/sandbox_types.h"

#ifndef SANDBOX_SRC_INTERCEPTORS_64_H_
#define SANDBOX_SRC_INTERCEPTORS_64_H_

namespace sandbox {

extern "C" {




SANDBOX_INTERCEPT NTSTATUS WINAPI TargetNtMapViewOfSection64(
    HANDLE section, HANDLE process, PVOID *base, ULONG_PTR zero_bits,
    SIZE_T commit_size, PLARGE_INTEGER offset, PSIZE_T view_size,
    SECTION_INHERIT inherit, ULONG allocation_type, ULONG protect);




SANDBOX_INTERCEPT NTSTATUS WINAPI TargetNtUnmapViewOfSection64(HANDLE process,
                                                               PVOID base);





SANDBOX_INTERCEPT NTSTATUS WINAPI TargetNtSetInformationThread64(
    HANDLE thread, NT_THREAD_INFORMATION_CLASS thread_info_class,
    PVOID thread_information, ULONG thread_information_bytes);


SANDBOX_INTERCEPT NTSTATUS WINAPI TargetNtOpenThreadToken64(
    HANDLE thread, ACCESS_MASK desired_access, BOOLEAN open_as_self,
    PHANDLE token);


SANDBOX_INTERCEPT NTSTATUS WINAPI TargetNtOpenThreadTokenEx64(
    HANDLE thread, ACCESS_MASK desired_access, BOOLEAN open_as_self,
    ULONG handle_attributes, PHANDLE token);





SANDBOX_INTERCEPT NTSTATUS WINAPI TargetNtCreateFile64(
    PHANDLE file, ACCESS_MASK desired_access,
    POBJECT_ATTRIBUTES object_attributes, PIO_STATUS_BLOCK io_status,
    PLARGE_INTEGER allocation_size, ULONG file_attributes, ULONG sharing,
    ULONG disposition, ULONG options, PVOID ea_buffer, ULONG ea_length);


SANDBOX_INTERCEPT NTSTATUS WINAPI TargetNtOpenFile64(
    PHANDLE file, ACCESS_MASK desired_access,
    POBJECT_ATTRIBUTES object_attributes, PIO_STATUS_BLOCK io_status,
    ULONG sharing, ULONG options);


SANDBOX_INTERCEPT NTSTATUS WINAPI TargetNtQueryAttributesFile64(
    POBJECT_ATTRIBUTES object_attributes,
    PFILE_BASIC_INFORMATION file_attributes);


SANDBOX_INTERCEPT NTSTATUS WINAPI TargetNtQueryFullAttributesFile64(
    POBJECT_ATTRIBUTES object_attributes,
    PFILE_NETWORK_OPEN_INFORMATION file_attributes);


SANDBOX_INTERCEPT NTSTATUS WINAPI TargetNtSetInformationFile64(
    HANDLE file, PIO_STATUS_BLOCK io_status, PVOID file_information,
    ULONG length, FILE_INFORMATION_CLASS file_information_class);





SANDBOX_INTERCEPT HANDLE WINAPI TargetCreateNamedPipeW64(
    LPCWSTR pipe_name, DWORD open_mode, DWORD pipe_mode, DWORD max_instance,
    DWORD out_buffer_size, DWORD in_buffer_size, DWORD default_timeout,
    LPSECURITY_ATTRIBUTES security_attributes);





SANDBOX_INTERCEPT NTSTATUS WINAPI TargetNtOpenThread64(
    PHANDLE thread, ACCESS_MASK desired_access,
    POBJECT_ATTRIBUTES object_attributes, PCLIENT_ID client_id);


SANDBOX_INTERCEPT NTSTATUS WINAPI TargetNtOpenProcess64(
    PHANDLE process, ACCESS_MASK desired_access,
    POBJECT_ATTRIBUTES object_attributes, PCLIENT_ID client_id);


SANDBOX_INTERCEPT NTSTATUS WINAPI TargetNtOpenProcessToken64(
    HANDLE process, ACCESS_MASK desired_access, PHANDLE token);


SANDBOX_INTERCEPT NTSTATUS WINAPI TargetNtOpenProcessTokenEx64(
    HANDLE process, ACCESS_MASK desired_access, ULONG handle_attributes,
    PHANDLE token);


SANDBOX_INTERCEPT BOOL WINAPI TargetCreateProcessW64(
    LPCWSTR application_name, LPWSTR command_line,
    LPSECURITY_ATTRIBUTES process_attributes,
    LPSECURITY_ATTRIBUTES thread_attributes, BOOL inherit_handles, DWORD flags,
    LPVOID environment, LPCWSTR current_directory, LPSTARTUPINFOW startup_info,
    LPPROCESS_INFORMATION process_information);


SANDBOX_INTERCEPT BOOL WINAPI TargetCreateProcessA64(
    LPCSTR application_name, LPSTR command_line,
    LPSECURITY_ATTRIBUTES process_attributes,
    LPSECURITY_ATTRIBUTES thread_attributes, BOOL inherit_handles, DWORD flags,
    LPVOID environment, LPCSTR current_directory, LPSTARTUPINFOA startup_info,
    LPPROCESS_INFORMATION process_information);





SANDBOX_INTERCEPT NTSTATUS WINAPI TargetNtCreateKey64(
    PHANDLE key, ACCESS_MASK desired_access,
    POBJECT_ATTRIBUTES object_attributes, ULONG title_index,
    PUNICODE_STRING class_name, ULONG create_options, PULONG disposition);


SANDBOX_INTERCEPT NTSTATUS WINAPI TargetNtOpenKey64(
    PHANDLE key, ACCESS_MASK desired_access,
    POBJECT_ATTRIBUTES object_attributes);


SANDBOX_INTERCEPT NTSTATUS WINAPI TargetNtOpenKeyEx64(
    PHANDLE key, ACCESS_MASK desired_access,
    POBJECT_ATTRIBUTES object_attributes, ULONG open_options);





SANDBOX_INTERCEPT NTSTATUS WINAPI TargetNtCreateEvent64(
    PHANDLE event_handle, ACCESS_MASK desired_access,
    POBJECT_ATTRIBUTES object_attributes, EVENT_TYPE event_type,
    BOOLEAN initial_state);

SANDBOX_INTERCEPT NTSTATUS WINAPI TargetNtOpenEvent64(
    PHANDLE event_handle, ACCESS_MASK desired_access,
    POBJECT_ATTRIBUTES object_attributes);





SANDBOX_INTERCEPT BOOL WINAPI TargetGdiDllInitialize64(
    HANDLE dll,
    DWORD reason);


SANDBOX_INTERCEPT HGDIOBJ WINAPI TargetGetStockObject64(int object);


SANDBOX_INTERCEPT ATOM WINAPI TargetRegisterClassW64(const WNDCLASS* wnd_class);

}  

}  

#endif  
