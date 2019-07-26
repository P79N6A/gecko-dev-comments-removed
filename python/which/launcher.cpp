





































#ifdef WIN32
    #include <windows.h>
    #include <process.h>
    #include <direct.h>
    #include <shlwapi.h>
#else 
    #include <unistd.h>
#endif 
#include <sys/stat.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>



#define BUF_LENGTH 2048
#define MAX_PYTHON_ARGS 50
#define MAX_FILES 50
#define MAXPATHLEN 1024
#ifdef WIN32
    #define SEP '\\'
    #define ALTSEP '/'
    
    #define DELIM ';'
#else 
    #define SEP '/'
    
    #define DELIM ':'
#endif

#ifdef WIN32
    #define spawnvp _spawnvp
    #define snprintf _snprintf
    #define vsnprintf _vsnprintf
    
    #define stat _stat
#endif




char* programName = NULL;
char* programPath = NULL;
#ifndef WIN32 
    extern char **environ;   
#endif 



void _LogError(const char* format ...)
{
    va_list ap;
    va_start(ap, format);
#if defined(WIN32) && defined(_WINDOWS)
    
    char caption[BUF_LENGTH+1];
    snprintf(caption, BUF_LENGTH, "Error in %s", programName);
    char msg[BUF_LENGTH+1];
    vsnprintf(msg, BUF_LENGTH, format, ap);
    va_end(ap);
    MessageBox(NULL, msg, caption, MB_OK | MB_ICONEXCLAMATION);
#else
    fprintf(stderr, "%s: error: ", programName);
    vfprintf(stderr, format, ap);
    va_end(ap);
#endif 
}


void _LogWarning(const char* format ...)
{
    va_list ap;
    va_start(ap, format);
#if defined(WIN32) && defined(_WINDOWS)
    
    char caption[BUF_LENGTH+1];
    snprintf(caption, BUF_LENGTH, "Warning in %s", programName);
    char msg[BUF_LENGTH+1];
    vsnprintf(msg, BUF_LENGTH, format, ap);
    va_end(ap);
    MessageBox(NULL, msg, caption, MB_OK | MB_ICONWARNING);
#else
    fprintf(stderr, "%s: warning: ", programName);
    vfprintf(stderr, format, ap);
    va_end(ap);
#endif 
}






static int _IsDir(char *dirname)
{
#ifdef WIN32
    DWORD dwAttrib;
    dwAttrib = GetFileAttributes(dirname);
    if (dwAttrib == -1) {
        return 0;
    }
    if (dwAttrib & FILE_ATTRIBUTE_DIRECTORY) {
        return 1;
    }
    return 0;
#else 
    struct stat buf;
    if (stat(dirname, &buf) != 0)
        return 0;
    if (!S_ISDIR(buf.st_mode))
        return 0;
    return 1;
#endif
}



static int _IsLink(char *filename)
{
#ifdef WIN32
    return 0;
#else 
    struct stat buf;
    if (lstat(filename, &buf) != 0)
        return 0;
    if (!S_ISLNK(buf.st_mode))
        return 0;
    return 1;
#endif
}





static int _IsExecutableFile(char *filename)
{
#ifdef WIN32
    return (int)PathFileExists(filename);
#else 
    struct stat buf;
    if (stat(filename, &buf) != 0)
        return 0;
    if (!S_ISREG(buf.st_mode))
        return 0;
    if ((buf.st_mode & 0111) == 0)
        return 0;
    return 1;
#endif 
}







#ifdef WIN32
    static char* _GetProgramPath(void)
    {
        
        static char progPath[MAXPATHLEN+1];
        
        if (!GetModuleFileName(NULL, progPath, MAXPATHLEN)) {
            _LogError("could not get absolute program name from "\
                "GetModuleFileName\n");
            exit(1);
        }
        
        for (char* p = progPath+strlen(progPath);
             *p != SEP && *p != ALTSEP;
             --p)
        {
            *p = '\0';
        }
        *p = '\0';  

        return progPath;
    }
#else

    




    static void
    _JoinPath(char *buffer, char *stuff)
    {
        size_t n, k;
        if (stuff[0] == SEP)
            n = 0;
        else {
            n = strlen(buffer);
            if (n > 0 && buffer[n-1] != SEP && n < MAXPATHLEN)
                buffer[n++] = SEP;
        }
        k = strlen(stuff);
        if (n + k > MAXPATHLEN)
            k = MAXPATHLEN - n;
        strncpy(buffer+n, stuff, k);
        buffer[n+k] = '\0';
    }


    static char*
    _GetProgramPath(void)
    {
        
        char* path = getenv("PATH");
        static char progPath[MAXPATHLEN+1];

        




        if (strchr(programName, SEP)) {
            strncpy(progPath, programName, MAXPATHLEN);
        }
        else if (path) {
            int bufspace = MAXPATHLEN;
            while (1) {
                char *delim = strchr(path, DELIM);

                if (delim) {
                    size_t len = delim - path;
                    if (len > bufspace) {
                        len = bufspace;
                    }
                    strncpy(progPath, path, len);
                    *(progPath + len) = '\0';
                    bufspace -= len;
                }
                else {
                    strncpy(progPath, path, bufspace);
                }

                _JoinPath(progPath, programName);
                if (_IsExecutableFile(progPath)) {
                    break;
                }

                if (!delim) {
                    progPath[0] = '\0';
                    break;
                }
                path = delim + 1;
            }
        }
        else {
            progPath[0] = '\0';
        }

        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        if (_IsLink(progPath)) {
            char newProgPath[MAXPATHLEN+1];
            readlink(progPath, newProgPath, MAXPATHLEN);
            strncpy(progPath, newProgPath, MAXPATHLEN);
        }

        
        
        
        if (strlen(progPath) != 0 && progPath[0] != SEP) {
            char cwd[MAXPATHLEN+1];
            char tmp[MAXPATHLEN+1];
            
            getcwd(cwd, MAXPATHLEN);
            snprintf(tmp, MAXPATHLEN, "%s%c%s", cwd, SEP, progPath);
            strncpy(progPath, tmp, MAXPATHLEN);
        }
        
        
        
        char* pLetter = progPath + strlen(progPath);
        for (;pLetter != progPath && *pLetter != SEP; --pLetter) {
            
        }
        *pLetter = '\0';

        return progPath;
    }
#endif  




int main(int argc, char** argv)
{
    programName = argv[0];
    programPath = _GetProgramPath();

    
    
    
    char programNameNoExt[MAXPATHLEN+1];
    char *pStart, *pEnd;
    pStart = pEnd = programName + strlen(programName) - 1;
    while (pStart != programName && *(pStart-1) != SEP) {
	pStart--;
    }
    while (1) {
	if (pEnd == pStart) {
            pEnd = programName + strlen(programName) - 1;
	    break;
	}
	pEnd--;
	if (*(pEnd+1) == '.') {
	    break;
	}
    }
    strncpy(programNameNoExt, pStart, pEnd-pStart+1);
    *(programNameNoExt+(pEnd-pStart+1)) = '\0';

    
    char pyFile[MAXPATHLEN+1];
    snprintf(pyFile, MAXPATHLEN, "%s%c%s.py", programPath, SEP,
	     programNameNoExt);
    
    
    char* pythonArgs[MAX_PYTHON_ARGS+1];
    int nPythonArgs = 0;
    pythonArgs[nPythonArgs++] = "python";
    pythonArgs[nPythonArgs++] = "-tt";
    pythonArgs[nPythonArgs++] = pyFile;
    for (int i = 1; i < argc; ++i) {
        pythonArgs[nPythonArgs++] = argv[i];
    }
    pythonArgs[nPythonArgs++] = NULL;

    return _spawnvp(_P_WAIT, pythonArgs[0], pythonArgs);
}



#ifdef WIN32
    int WINAPI WinMain(
        HINSTANCE hInstance,      
        HINSTANCE hPrevInstance,  
        LPSTR lpCmdLine,          
        int nCmdShow              
    )
    {
        return main(__argc, __argv);
    }
#endif

