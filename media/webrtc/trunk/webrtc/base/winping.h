









#ifndef WEBRTC_BASE_WINPING_H__
#define WEBRTC_BASE_WINPING_H__

#if defined(WEBRTC_WIN)

#include "webrtc/base/win32.h"
#include "webrtc/base/basictypes.h"
#include "webrtc/base/IPAddress.h"

namespace rtc {






typedef struct ip_option_information {
    UCHAR   Ttl;                
    UCHAR   Tos;                
    UCHAR   Flags;              
    UCHAR   OptionsSize;        
    PUCHAR  OptionsData;        
} IP_OPTION_INFORMATION, * PIP_OPTION_INFORMATION;

typedef HANDLE (WINAPI *PIcmpCreateFile)();

typedef BOOL (WINAPI *PIcmpCloseHandle)(HANDLE icmp_handle);

typedef HANDLE (WINAPI *PIcmp6CreateFile)();

typedef BOOL (WINAPI *PIcmp6CloseHandle)(HANDLE icmp_handle);

typedef DWORD (WINAPI *PIcmpSendEcho)(
    HANDLE                   IcmpHandle,
    ULONG                    DestinationAddress,
    LPVOID                   RequestData,
    WORD                     RequestSize,
    PIP_OPTION_INFORMATION   RequestOptions,
    LPVOID                   ReplyBuffer,
    DWORD                    ReplySize,
    DWORD                    Timeout);

typedef DWORD (WINAPI *PIcmp6SendEcho2)(
    HANDLE IcmpHandle,
    HANDLE Event,
    FARPROC ApcRoutine,
    PVOID ApcContext,
    struct sockaddr_in6 *SourceAddress,
    struct sockaddr_in6 *DestinationAddress,
    LPVOID RequestData,
    WORD RequestSize,
    PIP_OPTION_INFORMATION RequestOptions,
    LPVOID ReplyBuffer,
    DWORD ReplySize,
    DWORD Timeout
);

class WinPing {
public:
    WinPing();
    ~WinPing();

    
    bool IsValid() { return valid_; }

    
    enum PingResult { PING_FAIL, PING_INVALID_PARAMS,
                      PING_TOO_LARGE, PING_TIMEOUT, PING_SUCCESS };
    PingResult Ping(
        IPAddress ip, uint32 data_size, uint32 timeout_millis, uint8 ttl,
        bool allow_fragments);

private:
    HMODULE dll_;
    HANDLE hping_;
    HANDLE hping6_;
    PIcmpCreateFile create_;
    PIcmpCloseHandle close_;
    PIcmpSendEcho send_;
    PIcmp6CreateFile create6_;
    PIcmp6SendEcho2 send6_;
    char* data_;
    uint32 dlen_;
    char* reply_;
    uint32 rlen_;
    bool valid_;
};

} 

#endif 

#endif 
