









#include "audio_device_utility_win.h"
#include "audio_device_config.h"

#include "critical_section_wrapper.h"
#include "trace.h"

#include <windows.h>
#include <tchar.h>
#include <strsafe.h>

#define STRING_MAX_SIZE 256

typedef void (WINAPI *PGNSI)(LPSYSTEM_INFO);
typedef BOOL (WINAPI *PGPI)(DWORD, DWORD, DWORD, DWORD, PDWORD);

namespace webrtc
{









AudioDeviceUtilityWindows::AudioDeviceUtilityWindows(const WebRtc_Word32 id) :
    _critSect(*CriticalSectionWrapper::CreateCriticalSection()),
    _id(id),
    _lastError(AudioDeviceModule::kAdmErrNone)
{
    WEBRTC_TRACE(kTraceMemory, kTraceAudioDevice, id, "%s created", __FUNCTION__);
}





AudioDeviceUtilityWindows::~AudioDeviceUtilityWindows()
{
    WEBRTC_TRACE(kTraceMemory, kTraceAudioDevice, _id, "%s destroyed", __FUNCTION__);
    {
        CriticalSectionScoped lock(&_critSect);

        
    }

    delete &_critSect;
}









WebRtc_Word32 AudioDeviceUtilityWindows::Init()
{

    TCHAR szOS[STRING_MAX_SIZE];

    if (GetOSDisplayString(szOS))
    {
#ifdef _UNICODE
        char os[STRING_MAX_SIZE];
        if (WideCharToMultiByte(CP_UTF8, 0, szOS, -1, os, STRING_MAX_SIZE, NULL, NULL) == 0)
        {
            strncpy(os, "Could not get OS info", STRING_MAX_SIZE);
        }
        
        WEBRTC_TRACE(kTraceStateInfo, kTraceAudioDevice, _id, "  OS info: %s", os);
#else
        
        WEBRTC_TRACE(kTraceStateInfo, kTraceAudioDevice, _id, "  OS info: %s", szOS);
#endif
    }

    return 0;
}





BOOL AudioDeviceUtilityWindows::GetOSDisplayString(LPTSTR pszOS)
{
    OSVERSIONINFOEX osvi;
    SYSTEM_INFO si;
    PGNSI pGNSI;
    BOOL bOsVersionInfoEx;

    ZeroMemory(&si, sizeof(SYSTEM_INFO));
    ZeroMemory(&osvi, sizeof(OSVERSIONINFOEX));

    osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEX);

    
    
    if (!(bOsVersionInfoEx = GetVersionEx((OSVERSIONINFO *) &osvi)))
        return FALSE;

    
    
    if (VER_PLATFORM_WIN32_NT == osvi.dwPlatformId && osvi.dwMajorVersion > 4)
    {
        StringCchCopy(pszOS, STRING_MAX_SIZE, TEXT("Microsoft "));

        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        if (osvi.dwMajorVersion == 6)
        {
            if (osvi.dwMinorVersion == 0)
            {
                
                if (osvi.wProductType == VER_NT_WORKSTATION)
                    StringCchCat(pszOS, STRING_MAX_SIZE, TEXT("Windows Vista "));
                else 
                    StringCchCat(pszOS, STRING_MAX_SIZE, TEXT("Windows Server 2008 " ));
            }

            if (osvi.dwMinorVersion == 1)
            {
                
                if (osvi.wProductType == VER_NT_WORKSTATION)
                    StringCchCat(pszOS, STRING_MAX_SIZE, TEXT("Windows 7 "));
                else 
                    StringCchCat(pszOS, STRING_MAX_SIZE, TEXT("Windows Server 2008 R2 " ));
            }
        }

        if (osvi.dwMajorVersion == 5 && osvi.dwMinorVersion == 2)
        {
            StringCchCat(pszOS, STRING_MAX_SIZE, TEXT("Windows Server 2003"));
        }

        if (osvi.dwMajorVersion == 5 && osvi.dwMinorVersion == 1)
        {
            StringCchCat(pszOS, STRING_MAX_SIZE, TEXT("Windows XP "));
            if (osvi.wSuiteMask & VER_SUITE_PERSONAL)
                StringCchCat(pszOS, STRING_MAX_SIZE, TEXT( "Home Edition" ));
            else 
                StringCchCat(pszOS, STRING_MAX_SIZE, TEXT( "Professional" ));
        }

        if (osvi.dwMajorVersion == 5 && osvi.dwMinorVersion == 0)
        {
            StringCchCat(pszOS, STRING_MAX_SIZE, TEXT("Windows 2000 "));

            if (osvi.wProductType == VER_NT_WORKSTATION )
            {
                StringCchCat(pszOS, STRING_MAX_SIZE, TEXT( "Professional" ));
            }
            else 
            {
                if (osvi.wSuiteMask & VER_SUITE_DATACENTER)
                    StringCchCat(pszOS, STRING_MAX_SIZE, TEXT( "Datacenter Server" ));
                else if (osvi.wSuiteMask & VER_SUITE_ENTERPRISE)
                    StringCchCat(pszOS, STRING_MAX_SIZE, TEXT( "Advanced Server" ));
                else StringCchCat(pszOS, STRING_MAX_SIZE, TEXT( "Server" ));
            }
        }

        
        
        if (_tcslen(osvi.szCSDVersion) > 0)
        {
            StringCchCat(pszOS, STRING_MAX_SIZE, TEXT(" "));
            StringCchCat(pszOS, STRING_MAX_SIZE, osvi.szCSDVersion);
        }

        TCHAR buf[80];

        
        
        StringCchPrintf( buf, 80, TEXT(" (build %d)"), osvi.dwBuildNumber);
        StringCchCat(pszOS, STRING_MAX_SIZE, buf);

        
        
        pGNSI = (PGNSI) GetProcAddress(GetModuleHandle(TEXT("kernel32.dll")), "GetNativeSystemInfo");
        if (NULL != pGNSI)
            pGNSI(&si);
        else 
            GetSystemInfo(&si);

        
        
        if (osvi.dwMajorVersion >= 6)
        {
            if ((si.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_AMD64) || 
                (si.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_IA64))
                StringCchCat(pszOS, STRING_MAX_SIZE, TEXT( ", 64-bit" ));
            else if (si.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_INTEL )
                StringCchCat(pszOS, STRING_MAX_SIZE, TEXT(", 32-bit"));
        }
      
        return TRUE; 
    }
    else
    {  
        return FALSE;
   }
}

}  
