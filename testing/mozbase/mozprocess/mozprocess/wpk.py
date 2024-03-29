



from __future__ import absolute_import

from ctypes import sizeof, windll, addressof, create_unicode_buffer
from ctypes.wintypes import DWORD, HANDLE

PROCESS_TERMINATE = 0x0001
PROCESS_QUERY_INFORMATION = 0x0400
PROCESS_VM_READ = 0x0010

def get_pids(process_name):
    BIG_ARRAY = DWORD * 4096
    processes = BIG_ARRAY()
    needed = DWORD()

    pids = []
    result = windll.psapi.EnumProcesses(processes,
                                        sizeof(processes),
                                        addressof(needed))
    if not result:
        return pids

    num_results = needed.value / sizeof(DWORD)

    for i in range(num_results):
        pid = processes[i]
        process = windll.kernel32.OpenProcess(PROCESS_QUERY_INFORMATION |
                                              PROCESS_VM_READ,
                                              0, pid)
        if process:
            module = HANDLE()
            result = windll.psapi.EnumProcessModules(process,
                                                     addressof(module),
                                                     sizeof(module),
                                                     addressof(needed))
            if result:
                name = create_unicode_buffer(1024)
                result = windll.psapi.GetModuleBaseNameW(process, module,
                                                         name, len(name))
                
                
                if name.value.startswith(process_name):
                    pids.append(pid)
                windll.kernel32.CloseHandle(module)
            windll.kernel32.CloseHandle(process)

    return pids

def kill_pid(pid):
    process = windll.kernel32.OpenProcess(PROCESS_TERMINATE, 0, pid)
    if process:
        windll.kernel32.TerminateProcess(process, 0)
        windll.kernel32.CloseHandle(process)
