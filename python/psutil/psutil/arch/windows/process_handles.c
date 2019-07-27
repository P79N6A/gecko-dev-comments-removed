






#ifndef UNICODE
#define UNICODE
#endif

#include <Python.h>
#include <windows.h>
#include <stdio.h>
#include "process_handles.h"

#ifndef NT_SUCCESS
#define NT_SUCCESS(x) ((x) >= 0)
#endif
#define STATUS_INFO_LENGTH_MISMATCH 0xc0000004

#define SystemHandleInformation 16
#define ObjectBasicInformation 0
#define ObjectNameInformation 1
#define ObjectTypeInformation 2


typedef LONG NTSTATUS;

typedef struct _UNICODE_STRING {
    USHORT Length;
    USHORT MaximumLength;
    PWSTR Buffer;
} UNICODE_STRING, *PUNICODE_STRING;

typedef NTSTATUS (NTAPI *_NtQuerySystemInformation)(
    ULONG SystemInformationClass,
    PVOID SystemInformation,
    ULONG SystemInformationLength,
    PULONG ReturnLength
);

typedef NTSTATUS (NTAPI *_NtDuplicateObject)(
    HANDLE SourceProcessHandle,
    HANDLE SourceHandle,
    HANDLE TargetProcessHandle,
    PHANDLE TargetHandle,
    ACCESS_MASK DesiredAccess,
    ULONG Attributes,
    ULONG Options
);

typedef NTSTATUS (NTAPI *_NtQueryObject)(
    HANDLE ObjectHandle,
    ULONG ObjectInformationClass,
    PVOID ObjectInformation,
    ULONG ObjectInformationLength,
    PULONG ReturnLength
);

typedef struct _SYSTEM_HANDLE {
    ULONG ProcessId;
    BYTE ObjectTypeNumber;
    BYTE Flags;
    USHORT Handle;
    PVOID Object;
    ACCESS_MASK GrantedAccess;
} SYSTEM_HANDLE, *PSYSTEM_HANDLE;

typedef struct _SYSTEM_HANDLE_INFORMATION {
    ULONG HandleCount;
    SYSTEM_HANDLE Handles[1];
} SYSTEM_HANDLE_INFORMATION, *PSYSTEM_HANDLE_INFORMATION;

typedef enum _POOL_TYPE {
    NonPagedPool,
    PagedPool,
    NonPagedPoolMustSucceed,
    DontUseThisType,
    NonPagedPoolCacheAligned,
    PagedPoolCacheAligned,
    NonPagedPoolCacheAlignedMustS
} POOL_TYPE, *PPOOL_TYPE;

typedef struct _OBJECT_TYPE_INFORMATION {
    UNICODE_STRING Name;
    ULONG TotalNumberOfObjects;
    ULONG TotalNumberOfHandles;
    ULONG TotalPagedPoolUsage;
    ULONG TotalNonPagedPoolUsage;
    ULONG TotalNamePoolUsage;
    ULONG TotalHandleTableUsage;
    ULONG HighWaterNumberOfObjects;
    ULONG HighWaterNumberOfHandles;
    ULONG HighWaterPagedPoolUsage;
    ULONG HighWaterNonPagedPoolUsage;
    ULONG HighWaterNamePoolUsage;
    ULONG HighWaterHandleTableUsage;
    ULONG InvalidAttributes;
    GENERIC_MAPPING GenericMapping;
    ULONG ValidAccess;
    BOOLEAN SecurityRequired;
    BOOLEAN MaintainHandleCount;
    USHORT MaintainTypeList;
    POOL_TYPE PoolType;
    ULONG PagedPoolUsage;
    ULONG NonPagedPoolUsage;
} OBJECT_TYPE_INFORMATION, *POBJECT_TYPE_INFORMATION;


PVOID
GetLibraryProcAddress(PSTR LibraryName, PSTR ProcName)
{
    return GetProcAddress(GetModuleHandleA(LibraryName), ProcName);
}


PyObject *
psutil_get_open_files(long pid, HANDLE processHandle)
{
    _NtQuerySystemInformation NtQuerySystemInformation =
        GetLibraryProcAddress("ntdll.dll", "NtQuerySystemInformation");
    _NtQueryObject NtQueryObject =
        GetLibraryProcAddress("ntdll.dll", "NtQueryObject");

    NTSTATUS                    status;
    PSYSTEM_HANDLE_INFORMATION  handleInfo;
    ULONG                       handleInfoSize = 0x10000;
    ULONG                       i;
    ULONG                       fileNameLength;
    PyObject                    *filesList = Py_BuildValue("[]");
    PyObject                    *arg = NULL;
    PyObject                    *fileFromWchar = NULL;

    if (filesList == NULL)
        return NULL;

    handleInfo = (PSYSTEM_HANDLE_INFORMATION)malloc(handleInfoSize);
    if (handleInfo == NULL) {
        Py_DECREF(filesList);
        PyErr_NoMemory();
        return NULL;
    }

    
    
    while ((status = NtQuerySystemInformation(
                         SystemHandleInformation,
                         handleInfo,
                         handleInfoSize,
                         NULL
                     )) == STATUS_INFO_LENGTH_MISMATCH)
    {
        handleInfo = (PSYSTEM_HANDLE_INFORMATION) \
            realloc(handleInfo, handleInfoSize *= 2);
    }

    
    if (!NT_SUCCESS(status)) {
        Py_DECREF(filesList);
        free(handleInfo);
        return NULL;
    }

    for (i = 0; i < handleInfo->HandleCount; i++) {
        SYSTEM_HANDLE            handle = handleInfo->Handles[i];
        HANDLE                   dupHandle = NULL;
        HANDLE                   mapHandle = NULL;
        POBJECT_TYPE_INFORMATION objectTypeInfo = NULL;
        PVOID                    objectNameInfo;
        UNICODE_STRING           objectName;
        ULONG                    returnLength;
        DWORD                    error = 0;
        fileFromWchar = NULL;
        arg = NULL;

        
        if (handle.ProcessId != pid)
            continue;

        
        
        if ((handle.GrantedAccess == 0x0012019f)
                || (handle.GrantedAccess == 0x001a019f)
                || (handle.GrantedAccess == 0x00120189)
                || (handle.GrantedAccess == 0x00100000)) {
            continue;
        }

        if (!DuplicateHandle(processHandle,
                             handle.Handle,
                             GetCurrentProcess(),
                             &dupHandle,
                             0,
                             TRUE,
                             DUPLICATE_SAME_ACCESS))
         {
             
             continue;
         }


        mapHandle = CreateFileMapping(dupHandle,
                                      NULL,
                                      PAGE_READONLY,
                                      0,
                                      0,
                                      NULL);
        if (mapHandle == NULL) {
            error = GetLastError();
            if (error == ERROR_INVALID_HANDLE || error == ERROR_BAD_EXE_FORMAT) {
                CloseHandle(dupHandle);
                
                continue;
            }
        }
        CloseHandle(mapHandle);

        
        objectTypeInfo = (POBJECT_TYPE_INFORMATION)malloc(0x1000);
        if (!NT_SUCCESS(NtQueryObject(
                            dupHandle,
                            ObjectTypeInformation,
                            objectTypeInfo,
                            0x1000,
                            NULL
                        )))
        {
            free(objectTypeInfo);
            CloseHandle(dupHandle);
            continue;
        }

        objectNameInfo = malloc(0x1000);
        if (!NT_SUCCESS(NtQueryObject(
                            dupHandle,
                            ObjectNameInformation,
                            objectNameInfo,
                            0x1000,
                            &returnLength
                        )))
        {
            
            objectNameInfo = realloc(objectNameInfo, returnLength);
            if (!NT_SUCCESS(NtQueryObject(
                                dupHandle,
                                ObjectNameInformation,
                                objectNameInfo,
                                returnLength,
                                NULL
                            )))
            {
                
                







                free(objectTypeInfo);
                free(objectNameInfo);
                CloseHandle(dupHandle);
                continue;

            }
        }

        
        objectName = *(PUNICODE_STRING)objectNameInfo;

        
        if (objectName.Length)
        {
            
            
            fileNameLength = objectName.Length / 2;
            if (wcscmp(objectTypeInfo->Name.Buffer, L"File") == 0) {
                
                fileFromWchar = PyUnicode_FromWideChar(objectName.Buffer,
                                                       fileNameLength);
                if (fileFromWchar == NULL)
                    goto error_py_fun;
#if PY_MAJOR_VERSION >= 3
                arg = Py_BuildValue("N",
                                    PyUnicode_AsUTF8String(fileFromWchar));
#else
                arg = Py_BuildValue("N",
                                    PyUnicode_FromObject(fileFromWchar));
#endif
                if (!arg)
                    goto error_py_fun;
                Py_XDECREF(fileFromWchar);
                fileFromWchar = NULL;
                if (PyList_Append(filesList, arg))
                    goto error_py_fun;
                Py_XDECREF(arg);
            }
            









        }
        else
        {
            
            







            ;;
        }
        free(objectTypeInfo);
        free(objectNameInfo);
        CloseHandle(dupHandle);
    }
    free(handleInfo);
    CloseHandle(processHandle);
    return filesList;

error_py_fun:
    Py_XDECREF(arg);
    Py_XDECREF(fileFromWchar);
    Py_DECREF(filesList);
    return NULL;
}
