







































#include "mozce_internal.h"


extern "C" {
#if 0
}
#endif


MOZCE_SHUNT_API int mkdir(const char* inDirname)
{
    WINCE_LOG_API_CALL("mkdir called\n");
    
    int retval = -1;
    
    if(NULL != inDirname)
    {
        unsigned short wDirname[MAX_PATH];
        
        if(0 != a2w_buffer(inDirname, -1, wDirname, sizeof(wDirname) / sizeof(unsigned short)))
        {
            if(FALSE != CreateDirectoryW(wDirname, NULL))
            {
                retval = 0;
            }
        }
    }
    
    return retval;
}


MOZCE_SHUNT_API int rmdir(const char* inDirname)
{
    WINCE_LOG_API_CALL_1("rmdir called (%s)\n", inDirname);
    
    int retval = -1;
    
    if(NULL != inDirname)
    {
        unsigned short wDirname[MAX_PATH];
        
        if(0 != a2w_buffer(inDirname, -1, wDirname, sizeof(wDirname) / sizeof(unsigned short)))
        {
            if(FALSE != RemoveDirectoryW(wDirname))
            {
                retval = 0;
            }
        }
    }
    
    return retval;
}


#if 0
{
#endif
} 

