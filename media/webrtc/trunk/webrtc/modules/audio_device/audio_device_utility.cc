









#include <assert.h>

#include "webrtc/modules/audio_device/audio_device_utility.h"

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

uint32_t AudioDeviceUtility::GetTimeInMS()
{
	return timeGetTime();
}

bool AudioDeviceUtility::StringCompare(
    const char* str1 , const char* str2,
    const uint32_t length)
{
	return ((_strnicmp(str1, str2, length) == 0) ? true : false);
}

}  

#elif defined(WEBRTC_LINUX) || defined(WEBRTC_MAC)





#include <stdio.h>      
#include <string.h>     
#include <sys/time.h>   
#include <termios.h>    
#include <time.h>       

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

uint32_t AudioDeviceUtility::GetTimeInMS()
{
    struct timeval tv;
    struct timezone tz;
    uint32_t val;

    gettimeofday(&tv, &tz);
    val = (uint32_t)(tv.tv_sec*1000 + tv.tv_usec/1000);
    return val;
}

bool AudioDeviceUtility::StringCompare(
    const char* str1 , const char* str2, const uint32_t length)
{
    return (strncasecmp(str1, str2, length) == 0)?true: false;
}

}  

#endif  
