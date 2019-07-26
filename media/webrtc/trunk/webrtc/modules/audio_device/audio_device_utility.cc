









#include <cassert>

#include "audio_device_utility.h"

#if defined(_WIN32)





#include <windows.h>
#include <conio.h>
#include <ctype.h>
#include <stdio.h>
#include <mmsystem.h>

namespace webrtc
{

void AudioDeviceUtility::WaitForKey()
{
	_getch();
}

WebRtc_UWord32 AudioDeviceUtility::GetTimeInMS()
{
	return timeGetTime();
}

bool AudioDeviceUtility::StringCompare(
    const char* str1 , const char* str2,
    const WebRtc_UWord32 length)
{
	return ((_strnicmp(str1, str2, length) == 0) ? true : false);
}

}  

#elif defined(WEBRTC_LINUX) || defined(WEBRTC_MAC)





#include <sys/time.h>   
#include <time.h>       
#include <string.h>     
#include <stdio.h>      
#include <termios.h>    

#include <unistd.h>

namespace webrtc
{

void AudioDeviceUtility::WaitForKey()
{

    struct termios oldt, newt;

    tcgetattr( STDIN_FILENO, &oldt );

    

    newt = oldt;
    newt.c_lflag &= ~( ICANON | ECHO );
    tcsetattr( STDIN_FILENO, TCSANOW, &newt );

    

    

    

    if (getchar() == '\n')
    {
        getchar();
    }

    tcsetattr( STDIN_FILENO, TCSANOW, &oldt );
}

WebRtc_UWord32 AudioDeviceUtility::GetTimeInMS()
{
    struct timeval tv;
    struct timezone tz;
    WebRtc_UWord32 val;

    gettimeofday(&tv, &tz);
    val = (WebRtc_UWord32)(tv.tv_sec*1000 + tv.tv_usec/1000);
    return val;
}

bool AudioDeviceUtility::StringCompare(
    const char* str1 , const char* str2, const WebRtc_UWord32 length)
{
    return (strncasecmp(str1, str2, length) == 0)?true: false;
}

}  

#endif  


