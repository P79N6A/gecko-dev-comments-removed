







































#include "mozce_internal.h"
#include "mozce_defs.h"
#include "time_conversions.h"

extern "C" {
#if 0
}
#endif


MOZCE_SHUNT_API int mozce_stat(const char* inPath, struct mozce_stat* outStats)
{
    MOZCE_PRECHECK
        
#ifdef DEBUG
        mozce_printf("mozce_stat called\n");
#endif
    
    int retval = -1;
    
    if(NULL != outStats)
    {
        memset(outStats, 0, sizeof(struct stat));
        
        if(NULL != inPath)
        {
            WCHAR wPath[MAX_PATH];
            
            int convRes = a2w_buffer(inPath, -1, wPath, sizeof(wPath) / sizeof(WCHAR));
            if(0 != convRes)
            {
                HANDLE readHandle;
                WIN32_FIND_DATA findData;
                readHandle = FindFirstFileW(wPath, &findData); 
                
                if (readHandle != INVALID_HANDLE_VALUE && readHandle != NULL)
                {
                    
                    retval = 0;
                    outStats->st_size = findData.nFileSizeLow;
                    
                    if(findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
                    {
                        outStats->st_mode = _S_IFDIR;
                    }
                    else
                    {
                        outStats->st_mode = _S_IFREG;
                    }
                    
                    
                    FILETIME_2_time_t(outStats->st_ctime, findData.ftCreationTime);
                    FILETIME_2_time_t(outStats->st_atime, findData.ftLastAccessTime);
                    FILETIME_2_time_t(outStats->st_mtime, findData.ftLastWriteTime);
                    
                    outStats->st_mode |= _S_IREAD;
                    if(0 == (FILE_ATTRIBUTE_READONLY & findData.dwFileAttributes))
                    {
                        outStats->st_mode |= _S_IWRITE;
                    }
                    if(FILE_ATTRIBUTE_DIRECTORY & findData.dwFileAttributes)
                    {
                        outStats->st_mode |= _S_IFDIR;
                    }
                    else
                    {
                        outStats->st_mode |= _S_IFREG;
                    }
                    
                }
                else
                {	
                    
                    extern int mozce_errno;
                    mozce_errno = ENOENT;
                }
            }
        }
    }
    
    return retval;
}


#if 0
{
#endif
} 

