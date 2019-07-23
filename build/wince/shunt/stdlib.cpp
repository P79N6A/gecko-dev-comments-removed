






































#include <string.h>
#include <stdio.h>
#include "mozce_internal.h"

#define _MAX_FNAME          256
#define _MAX_DIR            _MAX_FNAME
#define _MAX_EXT            _MAX_FNAME


extern "C" {
#if 0
}
#endif

MOZCE_SHUNT_API char *fullpath(char *absPath, const char *relPath, size_t maxLength)
{
    WINCE_LOG_API_CALL("fullpath called\n");

    if (relPath[0] != '\\') 
    {
        int i;
        unsigned short dir[MAX_PATH];
        GetModuleFileName(GetModuleHandle (NULL), dir, MAX_PATH);
        for (i = _tcslen(dir); i && dir[i] != TEXT('\\'); i--) {}
        
        dir[i + 1] = TCHAR('\0');
        
        w2a_buffer(dir, -1, absPath, maxLength);
    }
    strcpy(absPath, relPath);
    
    return absPath;
}

MOZCE_SHUNT_API void splitpath(const char* inPath, char* outDrive, char* outDir, char* outFname, char* outExt)
{
    WINCE_LOG_API_CALL("splitpath called\n");

    if(NULL != outDrive)
    {
        *outDrive = '\0';
    }
    if(NULL != outDir)
    {
        *outDir = '\0';
    }
    if(NULL != outFname)
    {
        *outFname = '\0';
    }
    if(NULL != outExt)
    {
        *outExt = '\0';
    }

    if(NULL != inPath && '\0' != *inPath)
    {
                char* dup = (char*) malloc(strlen(inPath));
                if(NULL != dup)
        {
            strcpy(dup, inPath);
                        


            char* convert = dup;
            do
            {
                if('/' == *convert)
                {
                    *convert = '\\';
                }
                convert++;
            }
            while(*convert);

            


            char* slash = strrchr(dup, '\\');

            


            char* ext = NULL;
            if(NULL == slash)
            {
                ext = strchr(dup, '.');
            }
            else
            {
                ext = strchr(slash, '.');
            }

            


            if(NULL != ext)
            {
                if(NULL != outExt)
                {
                    strncpy(outExt, ext, _MAX_EXT);
                }

                *ext = '\0';
            }

            


            char* fname = NULL;
            if(NULL == slash)
            {
                fname = dup;
            }
            else
            {
                fname = slash + 1;
            }

            if(NULL != outFname)
            {
                strncpy(outFname, fname, _MAX_FNAME);
            }

            *fname = '\0';

            


            if(NULL != slash && NULL != outDir)
            {
                strncpy(outDir, dup, _MAX_DIR);
            }

            free(dup);
        }
    }
}


MOZCE_SHUNT_API void makepath(char* outPath, const char* inDrive, const char* inDir, const char* inFname, const char* inExt)
{
    WINCE_LOG_API_CALL("makepath called\n");

    if(NULL != outPath)
    {
        int dirLen = 0;
        if(NULL != inDir)
        {
            dirLen = strlen(inDir);
            if(dirLen)
            {
                dirLen--;
            }
        }
        _snprintf(outPath, _MAX_PATH, "%s%s%s%s%s",
                  (NULL != inDir) ? inDir : "",
                  (NULL != inDir && '\\' != inDir[dirLen] && '/' != inDir[dirLen]) ? "\\" : "",
                  (NULL != inFname) ? inFname : "",
                  (NULL != inExt && '.' != inExt[0]) ? "." : "",
                  (NULL != inExt) ? inExt : ""
                  );
    }
}

MOZCE_SHUNT_API int mozce_strcmpi(const char *dest, const char *src)
{
    WINCE_LOG_API_CALL("mozce_strcmpi called\n");

    int f,l;
    
    do {
        if ( ((f = (unsigned char)(*(dest++))) >= 'A') && (f <= 'Z') )
            f -= ('A' - 'a');
        
        if ( ((l = (unsigned char)(*(src++))) >= 'A') && (l <= 'Z') )
            l -= ('A' - 'a');
    } while ( f && (f == l) );

    return(f - l);
}

MOZCE_SHUNT_API int _unlink(const char *filename )
{
    wchar_t wname[MAX_PATH];
    a2w_buffer(filename, MAX_PATH, wname, MAX_PATH);
    return ::DeleteFileW(wname);
}
#if 0
{
#endif
} 
