










#include <Python.h>
#include <windows.h>
#include <Psapi.h>
#include <tlhelp32.h>

#include "security.h"
#include "process_info.h"
#include "ntextapi.h"






typedef LONG NTSTATUS;

typedef NTSTATUS (NTAPI *_NtQueryInformationProcess)(
    HANDLE ProcessHandle,
    DWORD ProcessInformationClass,
    PVOID ProcessInformation,
    DWORD ProcessInformationLength,
    PDWORD ReturnLength
    );

typedef struct _PROCESS_BASIC_INFORMATION
{
    PVOID Reserved1;
    PVOID PebBaseAddress;
    PVOID Reserved2[2];
    ULONG_PTR UniqueProcessId;
    PVOID Reserved3;
} PROCESS_BASIC_INFORMATION, *PPROCESS_BASIC_INFORMATION;









HANDLE
handle_from_pid_waccess(DWORD pid, DWORD dwDesiredAccess)
{
    HANDLE hProcess;
    DWORD  processExitCode = 0;

    if (pid == 0) {
        
        return AccessDenied();
    }

    hProcess = OpenProcess(dwDesiredAccess, FALSE, pid);
    if (hProcess == NULL) {
        if (GetLastError() == ERROR_INVALID_PARAMETER) {
            NoSuchProcess();
        }
        else {
            PyErr_SetFromWindowsErr(0);
        }
        return NULL;
    }

    
    GetExitCodeProcess(hProcess, &processExitCode);
    if (processExitCode == 0) {
        NoSuchProcess();
        CloseHandle(hProcess);
        return NULL;
    }
    return hProcess;
}







HANDLE
handle_from_pid(DWORD pid) {
    DWORD dwDesiredAccess = PROCESS_QUERY_INFORMATION | PROCESS_VM_READ;
    return handle_from_pid_waccess(pid, dwDesiredAccess);
}



PVOID
GetPebAddress(HANDLE ProcessHandle)
{
    _NtQueryInformationProcess NtQueryInformationProcess =
        (_NtQueryInformationProcess)GetProcAddress(
        GetModuleHandleA("ntdll.dll"), "NtQueryInformationProcess");
    PROCESS_BASIC_INFORMATION pbi;

    NtQueryInformationProcess(ProcessHandle, 0, &pbi, sizeof(pbi), NULL);
    return pbi.PebBaseAddress;
}


DWORD*
get_pids(DWORD *numberOfReturnedPIDs) {
    




    
    DWORD *procArray = NULL;
    DWORD procArrayByteSz;
    int procArraySz = 0;

    
    DWORD enumReturnSz = 0;

    do {
        procArraySz += 1024;
        free(procArray);
        procArrayByteSz = procArraySz * sizeof(DWORD);
        procArray = malloc(procArrayByteSz);

        if (! EnumProcesses(procArray, procArrayByteSz, &enumReturnSz)) {
            free(procArray);
            PyErr_SetFromWindowsErr(0);
            return NULL;
        }
    } while(enumReturnSz == procArraySz * sizeof(DWORD));

    
    *numberOfReturnedPIDs = enumReturnSz / sizeof(DWORD);

    return procArray;
}


int
pid_is_running(DWORD pid)
{
    HANDLE hProcess;
    DWORD exitCode;

    
    if (pid == 0) {
        return 1;
    }

    if (pid < 0) {
        return 0;
    }

    hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ,
                           FALSE, pid);
    if (NULL == hProcess) {
        
        if (GetLastError() == ERROR_INVALID_PARAMETER) {
            CloseHandle(hProcess);
            return 0;
        }

        
        if (GetLastError() == ERROR_ACCESS_DENIED) {
            CloseHandle(hProcess);
            return 1;
        }

        CloseHandle(hProcess);
        PyErr_SetFromWindowsErr(0);
        return -1;
    }

    if (GetExitCodeProcess(hProcess, &exitCode)) {
        CloseHandle(hProcess);
        return (exitCode == STILL_ACTIVE);
    }

    
    if (GetLastError() == ERROR_ACCESS_DENIED) {
        CloseHandle(hProcess);
        return 1;
    }

    PyErr_SetFromWindowsErr(0);
    CloseHandle(hProcess);
    return -1;
}


int
pid_in_proclist(DWORD pid)
{
    DWORD *proclist = NULL;
    DWORD numberOfReturnedPIDs;
    DWORD i;

    proclist = get_pids(&numberOfReturnedPIDs);
    if (NULL == proclist) {
        return -1;
    }

    for (i = 0; i < numberOfReturnedPIDs; i++) {
        if (pid == proclist[i]) {
            free(proclist);
            return 1;
        }
    }

    free(proclist);
    return 0;
}



BOOL is_running(HANDLE hProcess)
{
    DWORD dwCode;

    if (NULL == hProcess) {
        return FALSE;
    }

    if (GetExitCodeProcess(hProcess, &dwCode)) {
        return (dwCode == STILL_ACTIVE);
    }
    return FALSE;
}




PyObject*
get_name(long pid)
{
    HANDLE h = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    PROCESSENTRY32 pe = { 0 };
    pe.dwSize = sizeof(PROCESSENTRY32);

    if( Process32First(h, &pe)) {
        do {
            if (pe.th32ProcessID == pid) {
                CloseHandle(h);
                return Py_BuildValue("s", pe.szExeFile);
            }
        } while(Process32Next(h, &pe));

        
        NoSuchProcess();
        CloseHandle(h);
        return NULL;
    }

    CloseHandle(h);
    return PyErr_SetFromWindowsErr(0);
}



PyObject*
get_ppid(long pid)
{
    HANDLE h = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    PROCESSENTRY32 pe = { 0 };
    pe.dwSize = sizeof(PROCESSENTRY32);

    if( Process32First(h, &pe)) {
        do {
            if (pe.th32ProcessID == pid) {
                CloseHandle(h);
                return Py_BuildValue("I", pe.th32ParentProcessID);
            }
        } while(Process32Next(h, &pe));

        
        NoSuchProcess();
        CloseHandle(h);
        return NULL;
    }

    CloseHandle(h);
    return PyErr_SetFromWindowsErr(0);
}






PyObject*
get_arg_list(long pid)
{
    int nArgs, i;
    LPWSTR *szArglist = NULL;
    HANDLE hProcess = NULL;
    PVOID pebAddress;
    PVOID rtlUserProcParamsAddress;
    UNICODE_STRING commandLine;
    WCHAR *commandLineContents = NULL;
    PyObject *arg = NULL;
    PyObject *arg_from_wchar = NULL;
    PyObject *argList = NULL;

    hProcess = handle_from_pid(pid);
    if(hProcess == NULL) {
        return NULL;
    }

    pebAddress = GetPebAddress(hProcess);

    
#ifdef _WIN64
    if (!ReadProcessMemory(hProcess, (PCHAR)pebAddress + 32,
        &rtlUserProcParamsAddress, sizeof(PVOID), NULL))
#else
    if (!ReadProcessMemory(hProcess, (PCHAR)pebAddress + 0x10,
        &rtlUserProcParamsAddress, sizeof(PVOID), NULL))
#endif
    {
        
        PyErr_SetFromWindowsErr(0);
        goto error;
    }

    
#ifdef _WIN64
    if (!ReadProcessMemory(hProcess, (PCHAR)rtlUserProcParamsAddress + 112,
        &commandLine, sizeof(commandLine), NULL))
#else
    if (!ReadProcessMemory(hProcess, (PCHAR)rtlUserProcParamsAddress + 0x40,
        &commandLine, sizeof(commandLine), NULL))
#endif
    {
        
        PyErr_SetFromWindowsErr(0);
        goto error;
    }


    
    commandLineContents = (WCHAR *)malloc(commandLine.Length+1);

    
    if (!ReadProcessMemory(hProcess, commandLine.Buffer,
        commandLineContents, commandLine.Length, NULL))
    {
        
        PyErr_SetFromWindowsErr(0);
        goto error;
    }

    
    

    
    
    commandLineContents[(commandLine.Length/sizeof(WCHAR))] = '\0';

    
    
    szArglist = CommandLineToArgvW(commandLineContents, &nArgs);
    if (NULL == szArglist) {
        
        
        arg_from_wchar = PyUnicode_FromWideChar(commandLineContents,
                                                commandLine.Length / 2);
        if (arg_from_wchar == NULL)
            goto error;
        #if PY_MAJOR_VERSION >= 3
            argList = Py_BuildValue("N", PyUnicode_AsUTF8String(arg_from_wchar));
        #else
            argList = Py_BuildValue("N", PyUnicode_FromObject(arg_from_wchar));
        #endif
        if (!argList)
            goto error;
    }
    else {
        
        
        argList = Py_BuildValue("[]");
        if (!argList)
            goto error;
        for(i=0; i<nArgs; i++) {
            arg_from_wchar = NULL;
            arg = NULL;
            
            
            arg_from_wchar = PyUnicode_FromWideChar(szArglist[i],
                                                    wcslen(szArglist[i])
                                                    );
            if (arg_from_wchar == NULL)
                goto error;
            #if PY_MAJOR_VERSION >= 3
                arg = PyUnicode_FromObject(arg_from_wchar);
            #else
                arg = PyUnicode_AsUTF8String(arg_from_wchar);
            #endif
            if (arg == NULL)
                goto error;
            Py_XDECREF(arg_from_wchar);
            if (PyList_Append(argList, arg))
                goto error;
            Py_XDECREF(arg);
        }
    }

    if (szArglist != NULL)
        LocalFree(szArglist);
    free(commandLineContents);
    CloseHandle(hProcess);
    return argList;

error:
    Py_XDECREF(arg);
    Py_XDECREF(arg_from_wchar);
    Py_XDECREF(argList);
    if (hProcess != NULL)
        CloseHandle(hProcess);
    if (commandLineContents != NULL)
        free(commandLineContents);
    if (szArglist != NULL)
        LocalFree(szArglist);
    return NULL;
}


#define PH_FIRST_PROCESS(Processes) ((PSYSTEM_PROCESS_INFORMATION)(Processes))

#define PH_NEXT_PROCESS(Process) ( \
    ((PSYSTEM_PROCESS_INFORMATION)(Process))->NextEntryOffset ? \
    (PSYSTEM_PROCESS_INFORMATION)((PCHAR)(Process) + \
    ((PSYSTEM_PROCESS_INFORMATION)(Process))->NextEntryOffset) : \
    NULL \
    )

const STATUS_INFO_LENGTH_MISMATCH = 0xC0000004;
const STATUS_BUFFER_TOO_SMALL = 0xC0000023L;






int
get_process_info(DWORD pid, PSYSTEM_PROCESS_INFORMATION *retProcess, PVOID *retBuffer)
{
    static ULONG initialBufferSize = 0x4000;
    NTSTATUS status;
    PVOID buffer;
    ULONG bufferSize;
    PSYSTEM_PROCESS_INFORMATION process;

    
    typedef DWORD (_stdcall *NTQSI_PROC) (int, PVOID, ULONG, PULONG);
    NTQSI_PROC NtQuerySystemInformation;
    HINSTANCE hNtDll;
    hNtDll = LoadLibrary(TEXT("ntdll.dll"));
    NtQuerySystemInformation = (NTQSI_PROC)GetProcAddress(
                                hNtDll, "NtQuerySystemInformation");

    bufferSize = initialBufferSize;
    buffer = malloc(bufferSize);

    while (TRUE) {
        status = NtQuerySystemInformation(SystemProcessInformation, buffer,
                                          bufferSize, &bufferSize);

        if (status == STATUS_BUFFER_TOO_SMALL || status == STATUS_INFO_LENGTH_MISMATCH)
        {
            free(buffer);
            buffer = malloc(bufferSize);
        }
        else {
            break;
        }
    }

    if (status != 0) {
        PyErr_Format(PyExc_RuntimeError, "NtQuerySystemInformation() failed");
        FreeLibrary(hNtDll);
        free(buffer);
        return 0;
    }

    if (bufferSize <= 0x20000) {
        initialBufferSize = bufferSize;
    }

    process = PH_FIRST_PROCESS(buffer);
    do {
        if (process->UniqueProcessId == (HANDLE)pid) {
            *retProcess = process;
            *retBuffer = buffer;
            return 1;
        }
    } while ( (process = PH_NEXT_PROCESS(process)) );

    NoSuchProcess();
    FreeLibrary(hNtDll);
    free(buffer);
    return 0;
}
