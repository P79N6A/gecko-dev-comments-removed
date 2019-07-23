






































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



MOZCE_SHUNT_API char *mozce_fullpath(char *absPath, const char *relPath, size_t maxLength)
{
    MOZCE_PRECHECK

#ifdef LOG_CALLS
#ifdef DEBUG
    mozce_printf("mozce_fullpath called\n");
#endif
#endif

    if (relPath[0] != '\\') 
    {
        int i;
                unsigned short dir[MAX_PATH];
        GetModuleFileName(GetModuleHandle (NULL), dir, MAX_PATH);
        for (i = _tcslen(dir); i && dir[i] != TEXT('\\'); i--) {}
        
        dir[i + 1] = TCHAR('\0');
        
        w2a_buffer(dir, -1, absPath, maxLength);
    }
    strcat(absPath, relPath);
    
    return absPath;
}

MOZCE_SHUNT_API void mozce_splitpath(const char* inPath, char* outDrive, char* outDir, char* outFname, char* outExt)
{
    MOZCE_PRECHECK

#ifdef LOG_CALLS
#ifdef DEBUG
    mozce_printf("mozce_splitpath called\n");
#endif
#endif
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


MOZCE_SHUNT_API void mozce_makepath(char* outPath, const char* inDrive, const char* inDir, const char* inFname, const char* inExt)
{
    MOZCE_PRECHECK

#ifdef LOG_CALLS
#ifdef DEBUG
    mozce_printf("mozce_makepath called\n");
#endif
#endif
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
    MOZCE_PRECHECK

#ifdef LOG_CALLS
#ifdef DEBUG
    mozce_printf("mozce_strcmpi called\n");
#endif
#endif
    int f,l;
    
    do {
        if ( ((f = (unsigned char)(*(dest++))) >= 'A') && (f <= 'Z') )
            f -= ('A' - 'a');
        
        if ( ((l = (unsigned char)(*(src++))) >= 'A') && (l <= 'Z') )
            l -= ('A' - 'a');
    } while ( f && (f == l) );

    return(f - l);
}

#if 0
{
#endif
} 
