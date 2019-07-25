



































from ctypes import c_void_p, POINTER, sizeof, Structure, windll, WinError, WINFUNCTYPE
from ctypes.wintypes import BOOL, BYTE, DWORD, HANDLE, LPCWSTR, LPWSTR, UINT, WORD
from subprocess import SW_HIDE
from qijo import QueryInformationJobObject

LPVOID = c_void_p
LPBYTE = POINTER(BYTE)
LPDWORD = POINTER(DWORD)

def ErrCheckBool(result, func, args):
    """errcheck function for Windows functions that return a BOOL True
    on success"""
    if not result:
        raise WinError()
    return args




class AutoHANDLE(HANDLE):
    """Subclass of HANDLE which will call CloseHandle() on deletion."""
    
    CloseHandleProto = WINFUNCTYPE(BOOL, HANDLE)
    CloseHandle = CloseHandleProto(("CloseHandle", windll.kernel32))
    CloseHandle.errcheck = ErrCheckBool
    
    def Close(self):
        if self.value:
            self.CloseHandle(self)
            self.value = 0
    
    def __del__(self):
        self.Close()

    def __int__(self):
        return self.value

def ErrCheckHandle(result, func, args):
    """errcheck function for Windows functions that return a HANDLE."""
    if not result:
        raise WinError()
    return AutoHANDLE(result)



class PROCESS_INFORMATION(Structure):
    _fields_ = [("hProcess", HANDLE),
                ("hThread", HANDLE),
                ("dwProcessID", DWORD),
                ("dwThreadID", DWORD)]

    def __init__(self):
        Structure.__init__(self)
        
        self.cb = sizeof(self)

LPPROCESS_INFORMATION = POINTER(PROCESS_INFORMATION)



class STARTUPINFO(Structure):
    _fields_ = [("cb", DWORD),
                ("lpReserved", LPWSTR),
                ("lpDesktop", LPWSTR),
                ("lpTitle", LPWSTR),
                ("dwX", DWORD),
                ("dwY", DWORD),
                ("dwXSize", DWORD),
                ("dwYSize", DWORD),
                ("dwXCountChars", DWORD),
                ("dwYCountChars", DWORD),
                ("dwFillAttribute", DWORD),
                ("dwFlags", DWORD),
                ("wShowWindow", WORD),
                ("cbReserved2", WORD),
                ("lpReserved2", LPBYTE),
                ("hStdInput", HANDLE),
                ("hStdOutput", HANDLE),
                ("hStdError", HANDLE)
                ]
LPSTARTUPINFO = POINTER(STARTUPINFO)

STARTF_USESHOWWINDOW    = 0x01
STARTF_USESIZE          = 0x02
STARTF_USEPOSITION      = 0x04
STARTF_USECOUNTCHARS    = 0x08
STARTF_USEFILLATTRIBUTE = 0x10
STARTF_RUNFULLSCREEN    = 0x20
STARTF_FORCEONFEEDBACK  = 0x40
STARTF_FORCEOFFFEEDBACK = 0x80
STARTF_USESTDHANDLES    = 0x100



class EnvironmentBlock:
    """An object which can be passed as the lpEnv parameter of CreateProcess.
    It is initialized with a dictionary."""

    def __init__(self, dict):
        if not dict:
            self._as_parameter_ = None
        else:
            values = ["%s=%s" % (key, value)
                      for (key, value) in dict.iteritems()]
            values.append("")
            self._as_parameter_ = LPCWSTR("\0".join(values))
        


CreateProcessProto = WINFUNCTYPE(BOOL,                  
                                 LPCWSTR,               
                                 LPWSTR,                
                                 LPVOID,                
                                 LPVOID,                
                                 BOOL,                  
                                 DWORD,                 
                                 LPVOID,                
                                 LPCWSTR,               
                                 LPSTARTUPINFO,         
                                 LPPROCESS_INFORMATION  
                                 )

CreateProcessFlags = ((1, "lpApplicationName", None),
                      (1, "lpCommandLine"),
                      (1, "lpProcessAttributes", None),
                      (1, "lpThreadAttributes", None),
                      (1, "bInheritHandles", True),
                      (1, "dwCreationFlags", 0),
                      (1, "lpEnvironment", None),
                      (1, "lpCurrentDirectory", None),
                      (1, "lpStartupInfo"),
                      (2, "lpProcessInformation"))

def ErrCheckCreateProcess(result, func, args):
    ErrCheckBool(result, func, args)
    
    pi = args[9]
    return AutoHANDLE(pi.hProcess), AutoHANDLE(pi.hThread), pi.dwProcessID, pi.dwThreadID

CreateProcess = CreateProcessProto(("CreateProcessW", windll.kernel32),
                                   CreateProcessFlags)
CreateProcess.errcheck = ErrCheckCreateProcess

CREATE_BREAKAWAY_FROM_JOB = 0x01000000
CREATE_DEFAULT_ERROR_MODE = 0x04000000
CREATE_NEW_CONSOLE = 0x00000010
CREATE_NEW_PROCESS_GROUP = 0x00000200
CREATE_NO_WINDOW = 0x08000000
CREATE_SUSPENDED = 0x00000004
CREATE_UNICODE_ENVIRONMENT = 0x00000400
DEBUG_ONLY_THIS_PROCESS = 0x00000002
DEBUG_PROCESS = 0x00000001
DETACHED_PROCESS = 0x00000008



CreateJobObjectProto = WINFUNCTYPE(HANDLE,             
                                   LPVOID,             
                                   LPCWSTR             
                                   )

CreateJobObjectFlags = ((1, "lpJobAttributes", None),
                        (1, "lpName", None))

CreateJobObject = CreateJobObjectProto(("CreateJobObjectW", windll.kernel32),
                                       CreateJobObjectFlags)
CreateJobObject.errcheck = ErrCheckHandle



AssignProcessToJobObjectProto = WINFUNCTYPE(BOOL,      
                                            HANDLE,    
                                            HANDLE     
                                            )
AssignProcessToJobObjectFlags = ((1, "hJob"),
                                 (1, "hProcess"))
AssignProcessToJobObject = AssignProcessToJobObjectProto(
    ("AssignProcessToJobObject", windll.kernel32),
    AssignProcessToJobObjectFlags)
AssignProcessToJobObject.errcheck = ErrCheckBool



def ErrCheckResumeThread(result, func, args):
    if result == -1:
        raise WinError()

    return args

ResumeThreadProto = WINFUNCTYPE(DWORD,      
                                HANDLE      
                                )
ResumeThreadFlags = ((1, "hThread"),)
ResumeThread = ResumeThreadProto(("ResumeThread", windll.kernel32),
                                 ResumeThreadFlags)
ResumeThread.errcheck = ErrCheckResumeThread



TerminateJobObjectProto = WINFUNCTYPE(BOOL,   
                                      HANDLE, 
                                      UINT    
                                      )
TerminateJobObjectFlags = ((1, "hJob"),
                           (1, "uExitCode", 127))
TerminateJobObject = TerminateJobObjectProto(
    ("TerminateJobObject", windll.kernel32),
    TerminateJobObjectFlags)
TerminateJobObject.errcheck = ErrCheckBool



WaitForSingleObjectProto = WINFUNCTYPE(DWORD,  
                                       HANDLE, 
                                       DWORD,  
                                       )
WaitForSingleObjectFlags = ((1, "hHandle"),
                            (1, "dwMilliseconds", -1))
WaitForSingleObject = WaitForSingleObjectProto(
    ("WaitForSingleObject", windll.kernel32),
    WaitForSingleObjectFlags)

INFINITE = -1
WAIT_TIMEOUT = 0x0102
WAIT_OBJECT_0 = 0x0
WAIT_ABANDONED = 0x0080



GetExitCodeProcessProto = WINFUNCTYPE(BOOL,    
                                      HANDLE,  
                                      LPDWORD, 
                                      )
GetExitCodeProcessFlags = ((1, "hProcess"),
                           (2, "lpExitCode"))
GetExitCodeProcess = GetExitCodeProcessProto(
    ("GetExitCodeProcess", windll.kernel32),
    GetExitCodeProcessFlags)
GetExitCodeProcess.errcheck = ErrCheckBool
