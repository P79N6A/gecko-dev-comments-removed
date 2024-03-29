



































from ctypes import c_void_p, POINTER, sizeof, Structure, Union, windll, WinError, WINFUNCTYPE, c_ulong
from ctypes.wintypes import BOOL, BYTE, DWORD, HANDLE, LPCWSTR, LPWSTR, UINT, WORD, ULONG
from qijo import QueryInformationJobObject

LPVOID = c_void_p
LPBYTE = POINTER(BYTE)
LPDWORD = POINTER(DWORD)
LPBOOL = POINTER(BOOL)
LPULONG = POINTER(c_ulong)

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
        if self.value and self.value != HANDLE(-1).value:
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

SW_HIDE                 = 0

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



ERROR_ABANDONED_WAIT_0 = 735
            

GetLastErrorProto = WINFUNCTYPE(DWORD                   
                               )
GetLastErrorFlags = ()
GetLastError = GetLastErrorProto(("GetLastError", windll.kernel32), GetLastErrorFlags)



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




INVALID_HANDLE_VALUE = HANDLE(-1) 


COMPKEY_TERMINATE = c_ulong(0)
COMPKEY_JOBOBJECT = c_ulong(1)



JOB_OBJECT_LIMIT_BREAKAWAY_OK = 0x00000800
JOB_OBJECT_LIMIT_SILENT_BREAKAWAY_OK = 0x00001000



JOB_OBJECT_MSG_END_OF_JOB_TIME =          1
JOB_OBJECT_MSG_END_OF_PROCESS_TIME =      2
JOB_OBJECT_MSG_ACTIVE_PROCESS_LIMIT =     3
JOB_OBJECT_MSG_ACTIVE_PROCESS_ZERO =      4
JOB_OBJECT_MSG_NEW_PROCESS =              6
JOB_OBJECT_MSG_EXIT_PROCESS =             7
JOB_OBJECT_MSG_ABNORMAL_EXIT_PROCESS =    8
JOB_OBJECT_MSG_PROCESS_MEMORY_LIMIT =     9
JOB_OBJECT_MSG_JOB_MEMORY_LIMIT =         10


DEBUG_ONLY_THIS_PROCESS = 0x00000002
DEBUG_PROCESS = 0x00000001
DETACHED_PROCESS = 0x00000008
    

GetQueuedCompletionStatusProto = WINFUNCTYPE(BOOL,         
                                             HANDLE,       
                                             LPDWORD,      
                                             LPULONG,      
                                             LPULONG,      
                                             DWORD)        
GetQueuedCompletionStatusFlags = ((1, "CompletionPort", INVALID_HANDLE_VALUE),
                                  (1, "lpNumberOfBytes", None),
                                  (1, "lpCompletionKey", None),
                                  (1, "lpPID", None),
                                  (1, "dwMilliseconds", 0))
GetQueuedCompletionStatus = GetQueuedCompletionStatusProto(("GetQueuedCompletionStatus",
                                                            windll.kernel32),
                                                           GetQueuedCompletionStatusFlags)



CreateIoCompletionPortProto = WINFUNCTYPE(HANDLE,      
                                          HANDLE,      
                                          HANDLE,      
                                          c_ulong,     
                                          DWORD        
                                         )
CreateIoCompletionPortFlags = ((1, "FileHandle", INVALID_HANDLE_VALUE),
                               (1, "ExistingCompletionPort", 0),
                               (1, "CompletionKey", c_ulong(0)),
                               (1, "NumberOfConcurrentThreads", 0))
CreateIoCompletionPort = CreateIoCompletionPortProto(("CreateIoCompletionPort",
                                                      windll.kernel32),
                                                      CreateIoCompletionPortFlags)
CreateIoCompletionPort.errcheck = ErrCheckHandle


SetInformationJobObjectProto = WINFUNCTYPE(BOOL,      
                                           HANDLE,    
                                           DWORD,     
                                           LPVOID,    
                                           DWORD      
                                          )
SetInformationJobObjectProtoFlags = ((1, "hJob", None),
                                     (1, "JobObjectInfoClass", None),
                                     (1, "lpJobObjectInfo", None),
                                     (1, "cbJobObjectInfoLength", 0))
SetInformationJobObject = SetInformationJobObjectProto(("SetInformationJobObject",
                                                        windll.kernel32),
                                                        SetInformationJobObjectProtoFlags)
SetInformationJobObject.errcheck = ErrCheckBool


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



GetCurrentProcessProto = WINFUNCTYPE(HANDLE    
                                     )
GetCurrentProcessFlags = ()
GetCurrentProcess = GetCurrentProcessProto(
    ("GetCurrentProcess", windll.kernel32),
    GetCurrentProcessFlags)
GetCurrentProcess.errcheck = ErrCheckHandle


try:
    IsProcessInJobProto = WINFUNCTYPE(BOOL,     
                                      HANDLE,   
                                      HANDLE,   
                                      LPBOOL      
                                      )
    IsProcessInJobFlags = ((1, "ProcessHandle"),
                           (1, "JobHandle", HANDLE(0)),
                           (2, "Result"))
    IsProcessInJob = IsProcessInJobProto(
        ("IsProcessInJob", windll.kernel32),
        IsProcessInJobFlags)
    IsProcessInJob.errcheck = ErrCheckBool 
except AttributeError:
    
    def IsProcessInJob(process):
        return False




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



TerminateProcessProto = WINFUNCTYPE(BOOL,   
                                    HANDLE, 
                                    UINT    
                                    )
TerminateProcessFlags = ((1, "hProcess"),
                         (1, "uExitCode", 127))
TerminateProcess = TerminateProcessProto(
    ("TerminateProcess", windll.kernel32),
    TerminateProcessFlags)
TerminateProcess.errcheck = ErrCheckBool



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


STILL_ACTIVE = 259


ERROR_CONTROL_C_EXIT = 0x23c



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

def CanCreateJobObject():
    currentProc = GetCurrentProcess()
    if IsProcessInJob(currentProc):
        jobinfo = QueryInformationJobObject(HANDLE(0), 'JobObjectExtendedLimitInformation')
        limitflags = jobinfo['BasicLimitInformation']['LimitFlags']
        return bool(limitflags & JOB_OBJECT_LIMIT_BREAKAWAY_OK) or bool(limitflags & JOB_OBJECT_LIMIT_SILENT_BREAKAWAY_OK)
    else:
        return True



def parent():
    print 'Starting parent'
    currentProc = GetCurrentProcess()
    if IsProcessInJob(currentProc):
        print >> sys.stderr, "You should not be in a job object to test"
        sys.exit(1)
    assert CanCreateJobObject()
    print 'File: %s' % __file__
    command = [sys.executable, __file__, '-child']
    print 'Running command: %s' % command
    process = Popen(command)
    process.kill()
    code = process.returncode
    print 'Child code: %s' % code
    assert code == 127
        
def child():
    print 'Starting child'
    currentProc = GetCurrentProcess()
    injob = IsProcessInJob(currentProc)
    print "Is in a job?: %s" % injob
    can_create = CanCreateJobObject()
    print 'Can create job?: %s' % can_create
    process = Popen('c:\\windows\\notepad.exe')
    assert process._job
    jobinfo = QueryInformationJobObject(process._job, 'JobObjectExtendedLimitInformation')
    print 'Job info: %s' % jobinfo
    limitflags = jobinfo['BasicLimitInformation']['LimitFlags']
    print 'LimitFlags: %s' % limitflags
    process.kill()
