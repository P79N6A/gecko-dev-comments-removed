







































#include "mozce_internal.h"

extern "C" {
#if 0
}
#endif

MOZCE_SHUNT_API void setbuf(FILE *, char *)
{
}

MOZCE_SHUNT_API int chmod(const char* inFilename, int inMode)
{
#ifdef API_LOGGING
    mozce_printf("chmod called\n");
#endif
    
    int retval = -1;
    
    if(NULL != inFilename)
    {
        unsigned short buffer[MAX_PATH];
        
        int convRes = a2w_buffer(inFilename, -1, buffer, sizeof(buffer) / sizeof(unsigned short));
        if(0 != convRes)
        {
            DWORD attribs = 0;
            
            attribs = GetFileAttributesW(buffer);
            if(0 != attribs)
            {
                if(0 != (_S_IWRITE & inMode))
                {
                    attribs |= FILE_ATTRIBUTE_READONLY;
                }
                else
                {
                    attribs &= ~FILE_ATTRIBUTE_READONLY;
                }
                
                BOOL setRes = SetFileAttributesW(buffer, attribs);
                if(FALSE != setRes)
                {
                    retval = 0;
                }
            }
        }
    }
    
    return retval;
}


MOZCE_SHUNT_API int isatty(int inHandle)
{
#ifdef API_LOGGING
    mozce_printf("-- isatty called\n");
#endif
    
    int retval = 0;
    
    return retval;
}


#if 0
{
#endif
} 

