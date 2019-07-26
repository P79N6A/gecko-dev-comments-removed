










#ifndef WEBRTC_SYSTEM_WRAPPERS_SOURCE_THREAD_WINDOWS_SET_NAME_H_
#define WEBRTC_SYSTEM_WRAPPERS_SOURCE_THREAD_WINDOWS_SET_NAME_H_

namespace webrtc {

struct THREADNAME_INFO
{
   DWORD dwType;     
   LPCSTR szName;    
   DWORD dwThreadID; 
   DWORD dwFlags;    
};

void SetThreadName(DWORD dwThreadID, LPCSTR szThreadName)
{
    THREADNAME_INFO info;
    info.dwType = 0x1000;
    info.szName = szThreadName;
    info.dwThreadID = dwThreadID;
    info.dwFlags = 0;

    __try
    {
        RaiseException(0x406D1388, 0, sizeof(info) / sizeof(DWORD),
                       (ULONG_PTR*)&info);
    }
    __except (EXCEPTION_CONTINUE_EXECUTION)
    {
    }
}
} 
#endif 
