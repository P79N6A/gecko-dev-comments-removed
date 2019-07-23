







































#include "mozce_internal.h"


extern "C" {
#if 0
}
#endif


MOZCE_SHUNT_API int mozce_mkdir(const char* inDirname)
{
    MOZCE_PRECHECK

#ifdef DEBUG
    mozce_printf("mozce_mkdir called\n");
#endif
    
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


MOZCE_SHUNT_API int mozce_rmdir(const char* inDirname)
{
    MOZCE_PRECHECK

#ifdef DEBUG
    mozce_printf("mozce_rmdir called (%s)\n", inDirname);
#endif
    
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

