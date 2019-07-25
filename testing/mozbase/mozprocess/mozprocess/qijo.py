






































from ctypes import c_void_p, POINTER, sizeof, Structure, windll, WinError, WINFUNCTYPE, addressof, c_size_t, c_ulong
from ctypes.wintypes import BOOL, BYTE, DWORD, HANDLE, LARGE_INTEGER

LPVOID = c_void_p
LPDWORD = POINTER(DWORD)
SIZE_T = c_size_t
ULONG_PTR = POINTER(c_ulong)




ULONGLONG = BYTE * 8

class IO_COUNTERS(Structure):
    
    
    _fields_ = [('dummy', ULONGLONG * 6)]

class JOBOBJECT_BASIC_ACCOUNTING_INFORMATION(Structure):
    _fields_ = [('TotalUserTime', LARGE_INTEGER),
                ('TotalKernelTime', LARGE_INTEGER),
                ('ThisPeriodTotalUserTime', LARGE_INTEGER),
                ('ThisPeriodTotalKernelTime', LARGE_INTEGER),
                ('TotalPageFaultCount', DWORD),
                ('TotalProcesses', DWORD),
                ('ActiveProcesses', DWORD),
                ('TotalTerminatedProcesses', DWORD)]

class JOBOBJECT_BASIC_AND_IO_ACCOUNTING_INFORMATION(Structure):
    _fields_ = [('BasicInfo', JOBOBJECT_BASIC_ACCOUNTING_INFORMATION),
                ('IoInfo', IO_COUNTERS)]


class JOBOBJECT_BASIC_LIMIT_INFORMATION(Structure):
    _fields_ = [('PerProcessUserTimeLimit', LARGE_INTEGER),
                ('PerJobUserTimeLimit', LARGE_INTEGER),
                ('LimitFlags', DWORD),
                ('MinimumWorkingSetSize', SIZE_T),
                ('MaximumWorkingSetSize', SIZE_T),
                ('ActiveProcessLimit', DWORD),
                ('Affinity', ULONG_PTR),
                ('PriorityClass', DWORD),
                ('SchedulingClass', DWORD)
                ]

class JOBOBJECT_ASSOCIATE_COMPLETION_PORT(Structure):
    _fields_ = [('CompletionKey', c_ulong),
                ('CompletionPort', HANDLE)]


class JOBOBJECT_EXTENDED_LIMIT_INFORMATION(Structure):
    _fields_ = [('BasicLimitInformation', JOBOBJECT_BASIC_LIMIT_INFORMATION),
                ('IoInfo', IO_COUNTERS),
                ('ProcessMemoryLimit', SIZE_T),
                ('JobMemoryLimit', SIZE_T),
                ('PeakProcessMemoryUsed', SIZE_T),
                ('PeakJobMemoryUsed', SIZE_T)]



JobObjectAssociateCompletionPortInformation = 7
JobObjectBasicAndIoAccountingInformation = 8
JobObjectExtendedLimitInformation = 9

class JobObjectInfo(object):
    mapping = { 'JobObjectBasicAndIoAccountingInformation': 8,
                'JobObjectExtendedLimitInformation': 9,
                'JobObjectAssociateCompletionPortInformation': 7
                }
    structures = {
                   7: JOBOBJECT_ASSOCIATE_COMPLETION_PORT,
                   8: JOBOBJECT_BASIC_AND_IO_ACCOUNTING_INFORMATION,
                   9: JOBOBJECT_EXTENDED_LIMIT_INFORMATION
                   }
    def __init__(self, _class):
        if isinstance(_class, basestring):
            assert _class in self.mapping, 'Class should be one of %s; you gave %s' % (self.mapping, _class)
            _class = self.mapping[_class]
        assert _class in self.structures, 'Class should be one of %s; you gave %s' % (self.structures, _class)
        self.code = _class
        self.info = self.structures[_class]()


QueryInformationJobObjectProto = WINFUNCTYPE(
    BOOL,        
    HANDLE,      
    DWORD,       
    LPVOID,      
    DWORD,       
    LPDWORD      
    )

QueryInformationJobObjectFlags = (
    (1, 'hJob'),
    (1, 'JobObjectInfoClass'),
    (1, 'lpJobObjectInfo'),
    (1, 'cbJobObjectInfoLength'),
    (1, 'lpReturnLength', None)
    )

_QueryInformationJobObject = QueryInformationJobObjectProto(
    ('QueryInformationJobObject', windll.kernel32),
    QueryInformationJobObjectFlags
    )

class SubscriptableReadOnlyStruct(object):
    def __init__(self, struct):
        self._struct = struct

    def _delegate(self, name):
        result = getattr(self._struct, name)
        if isinstance(result, Structure):
            return SubscriptableReadOnlyStruct(result)
        return result

    def __getitem__(self, name):
        match = [fname for fname, ftype in self._struct._fields_
                 if fname == name]
        if match:
            return self._delegate(name)
        raise KeyError(name)

    def __getattr__(self, name):
        return self._delegate(name)

def QueryInformationJobObject(hJob, JobObjectInfoClass):
    jobinfo = JobObjectInfo(JobObjectInfoClass)
    result = _QueryInformationJobObject(
        hJob=hJob,
        JobObjectInfoClass=jobinfo.code,
        lpJobObjectInfo=addressof(jobinfo.info),
        cbJobObjectInfoLength=sizeof(jobinfo.info)
        )
    if not result:
        raise WinError()
    return SubscriptableReadOnlyStruct(jobinfo.info)
