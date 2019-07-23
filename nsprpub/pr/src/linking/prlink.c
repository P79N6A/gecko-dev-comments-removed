





































#include "primpl.h"

#include <string.h>

#ifdef XP_BEOS
#include <image.h>
#endif

#if defined(XP_MACOSX) && defined(USE_MACH_DYLD)
#include <CodeFragments.h>
#include <TextUtils.h>
#include <Types.h>
#include <Aliases.h>
#include <CFURL.h>
#include <CFBundle.h>
#include <CFString.h>
#include <CFDictionary.h>
#include <CFData.h>
#endif

#ifdef XP_UNIX
#ifdef USE_DLFCN
#include <dlfcn.h>

#ifndef RTLD_NOW
#define RTLD_NOW 0
#endif
#ifndef RTLD_LAZY
#define RTLD_LAZY RTLD_NOW
#endif
#ifndef RTLD_GLOBAL
#define RTLD_GLOBAL 0
#endif
#ifndef RTLD_LOCAL
#define RTLD_LOCAL 0
#endif
#ifdef AIX
#include <sys/ldr.h>
#endif
#ifdef OSF1
#include <loader.h>
#include <rld_interface.h>
#endif
#elif defined(USE_HPSHL)
#include <dl.h>
#elif defined(USE_MACH_DYLD)
#include <mach-o/dyld.h>
#endif
#endif 

#define _PR_DEFAULT_LD_FLAGS PR_LD_LAZY




#if defined(SUNOS4) || (defined(DARWIN) && defined(USE_MACH_DYLD)) \
    || defined(NEXTSTEP) || defined(XP_OS2) \
    || ((defined(OPENBSD) || defined(NETBSD)) && !defined(__ELF__))
#define NEED_LEADING_UNDERSCORE
#endif

#define PR_LD_PATHW 0x8000  /* for PR_LibSpec_PathnameU */



struct PRLibrary {
    char*                       name;  
    PRLibrary*                  next;
    int                         refCount;
    const PRStaticLinkTable*    staticTable;

#ifdef XP_PC
#ifdef XP_OS2
    HMODULE                     dlh;
#else
    HINSTANCE                   dlh;
#endif
#endif

#if defined(XP_MACOSX) && defined(USE_MACH_DYLD)
    CFragConnectionID           connection;
    CFBundleRef                 bundle;
    Ptr                         main;
    CFMutableDictionaryRef      wrappers;
    const struct mach_header*   image;
#endif

#ifdef XP_UNIX
#if defined(USE_HPSHL)
    shl_t                       dlh;
#elif defined(USE_MACH_DYLD)
    NSModule                    dlh;
#else
    void*                       dlh;
#endif 
#endif 

#ifdef XP_BEOS
    void*                       dlh;
    void*                       stub_dlh;
#endif
};

static PRLibrary *pr_loadmap;
static PRLibrary *pr_exe_loadmap;
static PRMonitor *pr_linker_lock;
static char* _pr_currentLibPath = NULL;

static PRLibrary *pr_LoadLibraryByPathname(const char *name, PRIntn flags);



#if !defined(USE_DLFCN) && !defined(HAVE_STRERROR)
#define ERR_STR_BUF_LENGTH    20
#endif

static void DLLErrorInternal(PRIntn oserr)





{
    const char *error = NULL;
#ifdef USE_DLFCN
    error = dlerror();  
#elif defined(HAVE_STRERROR)
    error = strerror(oserr);  
#else
    char errStrBuf[ERR_STR_BUF_LENGTH];
    PR_snprintf(errStrBuf, sizeof(errStrBuf), "error %d", oserr);
    error = errStrBuf;
#endif
    if (NULL != error)
        PR_SetErrorText(strlen(error), error);
}  

void _PR_InitLinker(void)
{
    PRLibrary *lm = NULL;
#if defined(XP_UNIX)
    void *h;
#endif

    if (!pr_linker_lock) {
        pr_linker_lock = PR_NewNamedMonitor("linker-lock");
    }
    PR_EnterMonitor(pr_linker_lock);

#if defined(XP_PC)
    lm = PR_NEWZAP(PRLibrary);
    lm->name = strdup("Executable");
#if defined(XP_OS2)
    lm->dlh = NULLHANDLE;
#else
    
    lm->dlh = GetModuleHandle(NULL);
#endif 

    lm->refCount    = 1;
    lm->staticTable = NULL;
    pr_exe_loadmap  = lm;
    pr_loadmap      = lm;

#elif defined(XP_UNIX)
#ifdef HAVE_DLL
#ifdef USE_DLFCN
    h = dlopen(0, RTLD_LAZY);
    if (!h) {
        char *error;
        
        DLLErrorInternal(_MD_ERRNO());
        error = (char*)PR_MALLOC(PR_GetErrorTextLength());
        (void) PR_GetErrorText(error);
        fprintf(stderr, "failed to initialize shared libraries [%s]\n",
            error);
        PR_DELETE(error);
        abort();
    }
#elif defined(USE_HPSHL)
    h = NULL;
    
#elif defined(USE_MACH_DYLD)
    h = NULL; 
#else
#error no dll strategy
#endif 

    lm = PR_NEWZAP(PRLibrary);
    if (lm) {
        lm->name = strdup("a.out");
        lm->refCount = 1;
        lm->dlh = h;
        lm->staticTable = NULL;
    }
    pr_exe_loadmap = lm;
    pr_loadmap = lm;
#endif 
#endif 

    if (lm) {
        PR_LOG(_pr_linker_lm, PR_LOG_MIN,
            ("Loaded library %s (init)", lm->name));
    }

    PR_ExitMonitor(pr_linker_lock);
}








void _PR_ShutdownLinker(void)
{
    
    
    PR_DestroyMonitor(pr_linker_lock);
    pr_linker_lock = NULL;

    if (_pr_currentLibPath) {
        free(_pr_currentLibPath);
        _pr_currentLibPath = NULL;
    }
}



PR_IMPLEMENT(PRStatus) PR_SetLibraryPath(const char *path)
{
    PRStatus rv = PR_SUCCESS;

    if (!_pr_initialized) _PR_ImplicitInitialization();
    PR_EnterMonitor(pr_linker_lock);
    if (_pr_currentLibPath) {
        free(_pr_currentLibPath);
    }
    if (path) {
        _pr_currentLibPath = strdup(path);
        if (!_pr_currentLibPath) {
            PR_SetError(PR_OUT_OF_MEMORY_ERROR, 0);
        rv = PR_FAILURE;
        }
    } else {
        _pr_currentLibPath = 0;
    }
    PR_ExitMonitor(pr_linker_lock);
    return rv;
}




PR_IMPLEMENT(char *) 
PR_GetLibraryPath(void)
{
    char *ev;
    char *copy = NULL;  

    if (!_pr_initialized) _PR_ImplicitInitialization();
    PR_EnterMonitor(pr_linker_lock);
    if (_pr_currentLibPath != NULL) {
        goto exit;
    }

    

#ifdef XP_PC
    ev = getenv("LD_LIBRARY_PATH");
    if (!ev) {
    ev = ".;\\lib";
    }
    ev = strdup(ev);
#endif

#if defined(XP_UNIX) || defined(XP_BEOS)
#if defined(USE_DLFCN) || defined(USE_MACH_DYLD) || defined(XP_BEOS)
    {
    char *p=NULL;
    int len;

#ifdef XP_BEOS
    ev = getenv("LIBRARY_PATH");
    if (!ev) {
        ev = "%A/lib:/boot/home/config/lib:/boot/beos/system/lib";
    }
#else
    ev = getenv("LD_LIBRARY_PATH");
    if (!ev) {
        ev = "/usr/lib:/lib";
    }
#endif
    len = strlen(ev) + 1;        

    p = (char*) malloc(len);
    if (p) {
        strcpy(p, ev);
    }   
    ev = p;
    PR_LOG(_pr_io_lm, PR_LOG_NOTICE, ("linker path '%s'", ev));

    }
#else
    
    ev = strdup("");
#endif
#endif

    


    _pr_currentLibPath = ev;

  exit:
    if (_pr_currentLibPath) {
        copy = strdup(_pr_currentLibPath);
    }
    PR_ExitMonitor(pr_linker_lock);
    if (!copy) {
        PR_SetError(PR_OUT_OF_MEMORY_ERROR, 0);
    }
    return copy;
}




PR_IMPLEMENT(char*) 
PR_GetLibraryName(const char *path, const char *lib)
{
    char *fullname;

#ifdef XP_PC
    if (strstr(lib, PR_DLL_SUFFIX) == NULL)
    {
        if (path) {
            fullname = PR_smprintf("%s\\%s%s", path, lib, PR_DLL_SUFFIX);
        } else {
            fullname = PR_smprintf("%s%s", lib, PR_DLL_SUFFIX);
        }
    } else {
        if (path) {
            fullname = PR_smprintf("%s\\%s", path, lib);
        } else {
            fullname = PR_smprintf("%s", lib);
        }
    }
#endif 
#if defined(XP_UNIX) || defined(XP_BEOS)
    if (strstr(lib, PR_DLL_SUFFIX) == NULL)
    {
        if (path) {
            fullname = PR_smprintf("%s/lib%s%s", path, lib, PR_DLL_SUFFIX);
        } else {
            fullname = PR_smprintf("lib%s%s", lib, PR_DLL_SUFFIX);
        }
    } else {
        if (path) {
            fullname = PR_smprintf("%s/%s", path, lib);
        } else {
            fullname = PR_smprintf("%s", lib);
        }
    }
#endif 
    return fullname;
}




PR_IMPLEMENT(void) 
PR_FreeLibraryName(char *mem)
{
    PR_smprintf_free(mem);
}

static PRLibrary* 
pr_UnlockedFindLibrary(const char *name)
{
    PRLibrary* lm = pr_loadmap;
    const char* np = strrchr(name, PR_DIRECTORY_SEPARATOR);
    np = np ? np + 1 : name;
    while (lm) {
    const char* cp = strrchr(lm->name, PR_DIRECTORY_SEPARATOR);
    cp = cp ? cp + 1 : lm->name;
#ifdef WIN32
        
    if (strcmpi(np, cp) == 0) 
#elif defined(XP_OS2)
    if (stricmp(np, cp) == 0)
#else
    if (strcmp(np, cp)  == 0) 
#endif
    {
        
        lm->refCount++;
        PR_LOG(_pr_linker_lm, PR_LOG_MIN,
           ("%s incr => %d (find lib)",
            lm->name, lm->refCount));
        return lm;
    }
    lm = lm->next;
    }
    return NULL;
}

PR_IMPLEMENT(PRLibrary*)
PR_LoadLibraryWithFlags(PRLibSpec libSpec, PRIntn flags)
{
    if (flags == 0) {
        flags = _PR_DEFAULT_LD_FLAGS;
    }
    switch (libSpec.type) {
        case PR_LibSpec_Pathname:
            return pr_LoadLibraryByPathname(libSpec.value.pathname, flags);
#ifdef WIN32
        case PR_LibSpec_PathnameU:
            



            return pr_LoadLibraryByPathname((const char*) 
                                            libSpec.value.pathname_u, 
                                            flags | PR_LD_PATHW);
#endif
        default:
            PR_SetError(PR_INVALID_ARGUMENT_ERROR, 0);
            return NULL;
    }
}
            
PR_IMPLEMENT(PRLibrary*) 
PR_LoadLibrary(const char *name)
{
    PRLibSpec libSpec;

    libSpec.type = PR_LibSpec_Pathname;
    libSpec.value.pathname = name;
    return PR_LoadLibraryWithFlags(libSpec, 0);
}

#if defined(USE_MACH_DYLD)
static NSModule
pr_LoadMachDyldModule(const char *name)
{
    NSObjectFileImage ofi;
    NSModule h = NULL;
    if (NSCreateObjectFileImageFromFile(name, &ofi)
            == NSObjectFileImageSuccess) {
        h = NSLinkModule(ofi, name, NSLINKMODULE_OPTION_PRIVATE
                         | NSLINKMODULE_OPTION_RETURN_ON_ERROR);
        



        if (NSDestroyObjectFileImage(ofi) == FALSE) {
            if (h) {
                (void)NSUnLinkModule(h, NSUNLINKMODULE_OPTION_NONE);
                h = NULL;
            }
        }
    }
    return h;
}
#endif

#if defined(XP_MACOSX) && defined(USE_MACH_DYLD)









typedef PRStatus (*macLibraryLoadProc)(const char *name, PRLibrary *lm);

#ifdef __ppc__






static void* TV2FP(CFMutableDictionaryRef dict, const char* name, void *tvp)
{
    static uint32 glue[6] = { 0x3D800000, 0x618C0000, 0x800C0000, 0x804C0004, 0x7C0903A6, 0x4E800420 };
    uint32* newGlue = NULL;

    if (tvp != NULL) {
        CFStringRef nameRef = CFStringCreateWithCString(NULL, name, kCFStringEncodingASCII);
        if (nameRef) {
            CFMutableDataRef glueData = (CFMutableDataRef) CFDictionaryGetValue(dict, nameRef);
            if (glueData == NULL) {
                glueData = CFDataCreateMutable(NULL, sizeof(glue));
                if (glueData != NULL) {
                    newGlue = (uint32*) CFDataGetMutableBytePtr(glueData);
                    memcpy(newGlue, glue, sizeof(glue));
                    newGlue[0] |= ((UInt32)tvp >> 16);
                    newGlue[1] |= ((UInt32)tvp & 0xFFFF);
                    MakeDataExecutable(newGlue, sizeof(glue));
                    CFDictionaryAddValue(dict, nameRef, glueData);
                    CFRelease(glueData);

                    PR_LOG(_pr_linker_lm, PR_LOG_MIN, ("TV2FP: created wrapper for CFM function %s().", name));
                }
            } else {
                PR_LOG(_pr_linker_lm, PR_LOG_MIN, ("TV2FP: found wrapper for CFM function %s().", name));

                newGlue = (uint32*) CFDataGetMutableBytePtr(glueData);
            }
            CFRelease(nameRef);
        }
    }
    
    return newGlue;
}

static PRStatus
pr_LoadViaCFM(const char *name, PRLibrary *lm)
{
    OSErr err;
    Str255 errName;
    FSRef ref;
    FSSpec fileSpec;
    Boolean tempUnusedBool;

    



    
    err = FSPathMakeRef((const UInt8*)name, &ref, NULL);
    if (err != noErr)
        return PR_FAILURE;
    err = FSGetCatalogInfo(&ref, kFSCatInfoNone, NULL, NULL,
                           &fileSpec, NULL);
    if (err != noErr)
        return PR_FAILURE;

    
    err = ResolveAliasFile(&fileSpec, true, &tempUnusedBool,
                           &tempUnusedBool);
    if (err != noErr)
        return PR_FAILURE;

    
    err = GetDiskFragment(&fileSpec, 0, kCFragGoesToEOF, fileSpec.name,
                          kLoadCFrag, &lm->connection, &lm->main, errName);

    if (err == noErr && lm->connection) {
        




        lm->wrappers = CFDictionaryCreateMutable(NULL, 16,
                       &kCFTypeDictionaryKeyCallBacks,
                       &kCFTypeDictionaryValueCallBacks);
        if (lm->wrappers) {
            lm->main = TV2FP(lm->wrappers, "main", lm->main);
        } else
            err = memFullErr;
    }
    return (err == noErr) ? PR_SUCCESS : PR_FAILURE;
}
#endif 







static PRStatus
pr_LoadCFBundle(const char *name, PRLibrary *lm)
{
    CFURLRef bundleURL;
    CFBundleRef bundle = NULL;
    char pathBuf[PATH_MAX];
    const char *resolvedPath;
    CFStringRef pathRef;

    
    resolvedPath = realpath(name, pathBuf);
    if (!resolvedPath)
        return PR_FAILURE;
        
    pathRef = CFStringCreateWithCString(NULL, pathBuf, kCFStringEncodingUTF8);
    if (pathRef) {
        bundleURL = CFURLCreateWithFileSystemPath(NULL, pathRef,
                                                  kCFURLPOSIXPathStyle, true);
        if (bundleURL) {
            bundle = CFBundleCreate(NULL, bundleURL);
            CFRelease(bundleURL);
        }
        CFRelease(pathRef);
    }

    lm->bundle = bundle;
    return (bundle != NULL) ? PR_SUCCESS : PR_FAILURE;
}

static PRStatus
pr_LoadViaDyld(const char *name, PRLibrary *lm)
{
    lm->dlh = pr_LoadMachDyldModule(name);
    if (lm->dlh == NULL) {
        lm->image = NSAddImage(name, NSADDIMAGE_OPTION_RETURN_ON_ERROR
                               | NSADDIMAGE_OPTION_WITH_SEARCHING);
        if (lm->image == NULL) {
            NSLinkEditErrors linkEditError;
            int errorNum;
            const char *errorString;
            const char *fileName;
            NSLinkEditError(&linkEditError, &errorNum, &fileName, &errorString);
            PR_LOG(_pr_linker_lm, PR_LOG_MIN, 
                   ("LoadMachDyldModule error %d:%d for file %s:\n%s",
                    linkEditError, errorNum, fileName, errorString));
        }
    }
    return (lm->dlh != NULL || lm->image != NULL) ? PR_SUCCESS : PR_FAILURE;
}

#endif 





static PRLibrary*
pr_LoadLibraryByPathname(const char *name, PRIntn flags)
{
    PRLibrary *lm;
    PRLibrary* result = NULL;
    PRInt32 oserr;
#ifdef WIN32
    char utf8name_stack[MAX_PATH];
    char *utf8name_malloc = NULL;
    char *utf8name = utf8name_stack;
    PRUnichar wname_stack[MAX_PATH];
    PRUnichar *wname_malloc = NULL;
    PRUnichar *wname = wname_stack;
    int len;
#endif

    if (!_pr_initialized) _PR_ImplicitInitialization();

    
    PR_EnterMonitor(pr_linker_lock);

#ifdef WIN32
    if (flags & PR_LD_PATHW) {
        
        wname = (LPWSTR) name;
    } else {
        int wlen = MultiByteToWideChar(CP_ACP, 0, name, -1, NULL, 0);
        if (wlen > MAX_PATH)
            wname = wname_malloc = PR_Malloc(wlen * sizeof(PRUnichar));
        if (wname == NULL ||
            !MultiByteToWideChar(CP_ACP, 0,  name, -1, wname, wlen)) {
            oserr = _MD_ERRNO();
            goto unlock;
        }
    }
    len = WideCharToMultiByte(CP_UTF8, 0, wname, -1, NULL, 0, NULL, NULL);
    if (len > MAX_PATH)
        utf8name = utf8name_malloc = PR_Malloc(len);
    if (utf8name == NULL ||
        !WideCharToMultiByte(CP_UTF8, 0, wname, -1,
                             utf8name, len, NULL, NULL)) {
        oserr = _MD_ERRNO();
        goto unlock;
    }
    

    result = pr_UnlockedFindLibrary(utf8name);
#else
    result = pr_UnlockedFindLibrary(name);
#endif

    if (result != NULL) goto unlock;

    lm = PR_NEWZAP(PRLibrary);
    if (lm == NULL) {
        oserr = _MD_ERRNO();
        goto unlock;
    }
    lm->staticTable = NULL;

#ifdef XP_OS2  
    {
        HMODULE h;
        UCHAR pszError[_MAX_PATH];
        ULONG ulRc = NO_ERROR;

          ulRc = DosLoadModule(pszError, _MAX_PATH, (PSZ) name, &h);
          if (ulRc != NO_ERROR) {
              oserr = ulRc;
              PR_DELETE(lm);
              goto unlock;
          }
          lm->name = strdup(name);
          lm->dlh  = h;
          lm->next = pr_loadmap;
          pr_loadmap = lm;
    }
#endif 

#ifdef WIN32
    {
    HINSTANCE h;

    h = LoadLibraryExW(wname, NULL,
                       (flags & PR_LD_ALT_SEARCH_PATH) ?
                       LOAD_WITH_ALTERED_SEARCH_PATH : 0);
    if (h == NULL) {
        oserr = _MD_ERRNO();
        PR_DELETE(lm);
        goto unlock;
    }
    lm->name = strdup(utf8name);
    lm->dlh = h;
    lm->next = pr_loadmap;
    pr_loadmap = lm;
    }
#endif 

#if defined(XP_MACOSX) && defined(USE_MACH_DYLD)
    {
    int     i;
    PRStatus status;

    static const macLibraryLoadProc loadProcs[] = {
#ifdef __ppc__
        pr_LoadViaDyld, pr_LoadCFBundle, pr_LoadViaCFM
#else  
        pr_LoadViaDyld, pr_LoadCFBundle
#endif 
    };

    for (i = 0; i < sizeof(loadProcs) / sizeof(loadProcs[0]); i++) {
        if ((status = loadProcs[i](name, lm)) == PR_SUCCESS)
            break;
    }
    if (status != PR_SUCCESS) {
        oserr = cfragNoLibraryErr;
        PR_DELETE(lm);
        goto unlock;        
    }
    lm->name = strdup(name);
    lm->next = pr_loadmap;
    pr_loadmap = lm;
    }
#endif

#if defined(XP_UNIX) && !(defined(XP_MACOSX) && defined(USE_MACH_DYLD))
#ifdef HAVE_DLL
    {
#if defined(USE_DLFCN)
#ifdef NTO
    
    int dl_flags = RTLD_GROUP;
#elif defined(AIX)
    
    int dl_flags = RTLD_MEMBER;
#else
    int dl_flags = 0;
#endif
    void *h = NULL;

    if (flags & PR_LD_LAZY) {
        dl_flags |= RTLD_LAZY;
    }
    if (flags & PR_LD_NOW) {
        dl_flags |= RTLD_NOW;
    }
    if (flags & PR_LD_GLOBAL) {
        dl_flags |= RTLD_GLOBAL;
    }
    if (flags & PR_LD_LOCAL) {
        dl_flags |= RTLD_LOCAL;
    }
#if defined(DARWIN)
    
    
    
    if (strchr(name, PR_DIRECTORY_SEPARATOR) == NULL ||
        PR_Access(name, PR_ACCESS_EXISTS) == PR_SUCCESS) {
            h = dlopen(name, dl_flags);
        }
#else
    h = dlopen(name, dl_flags);
#endif
#elif defined(USE_HPSHL)
    int shl_flags = 0;
    shl_t h;

    




    if (strchr(name, PR_DIRECTORY_SEPARATOR) == NULL) {
        shl_flags |= DYNAMIC_PATH;
    }
    if (flags & PR_LD_LAZY) {
        shl_flags |= BIND_DEFERRED;
    }
    if (flags & PR_LD_NOW) {
        shl_flags |= BIND_IMMEDIATE;
    }
    
    h = shl_load(name, shl_flags, 0L);
#elif defined(USE_MACH_DYLD)
    NSModule h = pr_LoadMachDyldModule(name);
#else
#error Configuration error
#endif
    if (!h) {
        oserr = _MD_ERRNO();
        PR_DELETE(lm);
        goto unlock;
    }
    lm->name = strdup(name);
    lm->dlh = h;
    lm->next = pr_loadmap;
    pr_loadmap = lm;
    }
#endif 
#endif 

    lm->refCount = 1;

#ifdef XP_BEOS
    {
        image_info info;
        int32 cookie = 0;
        image_id imageid = B_ERROR;
        image_id stubid = B_ERROR;
        PRLibrary *p;

        for (p = pr_loadmap; p != NULL; p = p->next) {
            

            if (strcmp(name, p->name) == 0) {
                
                imageid = info.id;
                lm->refCount++;
                break;
            }
        }

        if(imageid == B_ERROR) {
            
            char stubName [B_PATH_NAME_LENGTH + 1];

            

















            strcpy(stubName, name);
            strcat(stubName, ".stub");

            

            if ((stubid = load_add_on(stubName)) > B_ERROR) {
                
                imageid = B_FILE_NOT_FOUND;

                cookie = 0;
                while (get_next_image_info(0, &cookie, &info) == B_OK) {
                    const char *endOfSystemName = strrchr(info.name, '/');
                    const char *endOfPassedName = strrchr(name, '/');
                    if( 0 == endOfSystemName ) 
                        endOfSystemName = info.name;
                    else
                        endOfSystemName++;
                    if( 0 == endOfPassedName )
                        endOfPassedName = name;
                    else
                        endOfPassedName++;
                    if (strcmp(endOfSystemName, endOfPassedName) == 0) {
                        
                        imageid = info.id;
                        break;
                    }
                }

            } else {
                

                stubid = B_ERROR;
                imageid = load_add_on(name);
            }
        }

        if (imageid <= B_ERROR) {
            oserr = imageid;
            PR_DELETE( lm );
            goto unlock;
        }
        lm->name = strdup(name);
        lm->dlh = (void*)imageid;
        lm->stub_dlh = (void*)stubid;
        lm->next = pr_loadmap;
        pr_loadmap = lm;
    }
#endif

    result = lm;    
    PR_LOG(_pr_linker_lm, PR_LOG_MIN, ("Loaded library %s (load lib)", lm->name));

  unlock:
    if (result == NULL) {
        PR_SetError(PR_LOAD_LIBRARY_ERROR, oserr);
        DLLErrorInternal(oserr);  
    }
#ifdef WIN32
    if (utf8name_malloc) 
        PR_Free(utf8name_malloc);
    if (wname_malloc)
        PR_Free(wname_malloc);
#endif
    PR_ExitMonitor(pr_linker_lock);
    return result;
}




PR_IMPLEMENT(PRStatus) 
PR_UnloadLibrary(PRLibrary *lib)
{
    int result = 0;
    PRStatus status = PR_SUCCESS;

    if ((lib == 0) || (lib->refCount <= 0)) {
        PR_SetError(PR_INVALID_ARGUMENT_ERROR, 0);
        return PR_FAILURE;
    }

    PR_EnterMonitor(pr_linker_lock);
    if (--lib->refCount > 0) {
    PR_LOG(_pr_linker_lm, PR_LOG_MIN,
           ("%s decr => %d",
        lib->name, lib->refCount));
    goto done;
    }

#ifdef XP_BEOS
    if(((image_id)lib->stub_dlh) == B_ERROR)
        unload_add_on( (image_id) lib->dlh );
    else
        unload_add_on( (image_id) lib->stub_dlh);
#endif

#ifdef XP_UNIX
#ifdef HAVE_DLL
#ifdef USE_DLFCN
    result = dlclose(lib->dlh);
#elif defined(USE_HPSHL)
    result = shl_unload(lib->dlh);
#elif defined(USE_MACH_DYLD)
    if (lib->dlh)
        result = NSUnLinkModule(lib->dlh, NSUNLINKMODULE_OPTION_NONE) ? 0 : -1;
#else
#error Configuration error
#endif
#endif 
#endif 
#ifdef XP_PC
    if (lib->dlh) {
        FreeLibrary((HINSTANCE)(lib->dlh));
        lib->dlh = (HINSTANCE)NULL;
    }
#endif  

#if defined(XP_MACOSX) && defined(USE_MACH_DYLD)
    
    if (lib->connection)
        CloseConnection(&(lib->connection));
    if (lib->bundle)
        CFRelease(lib->bundle);
    if (lib->wrappers)
        CFRelease(lib->wrappers);
    
#endif

    
    if (pr_loadmap == lib)
        pr_loadmap = pr_loadmap->next;
    else if (pr_loadmap != NULL) {
        PRLibrary* prev = pr_loadmap;
        PRLibrary* next = pr_loadmap->next;
        while (next != NULL) {
            if (next == lib) {
                prev->next = next->next;
                goto freeLib;
            }
            prev = next;
            next = next->next;
        }
        



        PR_ASSERT(!"_pr_loadmap and lib->refCount inconsistent");
        if (result == 0) {
            PR_SetError(PR_INVALID_ARGUMENT_ERROR, 0);
            status = PR_FAILURE;
        }
    }
    




  freeLib:
    PR_LOG(_pr_linker_lm, PR_LOG_MIN, ("Unloaded library %s", lib->name));
    free(lib->name);
    lib->name = NULL;
    PR_DELETE(lib);
    if (result != 0) {
        PR_SetError(PR_UNLOAD_LIBRARY_ERROR, _MD_ERRNO());
        DLLErrorInternal(_MD_ERRNO());
        status = PR_FAILURE;
    }

done:
    PR_ExitMonitor(pr_linker_lock);
    return status;
}

static void* 
pr_FindSymbolInLib(PRLibrary *lm, const char *name)
{
    void *f = NULL;
#ifdef XP_OS2
    int rc;
#endif

    if (lm->staticTable != NULL) {
        const PRStaticLinkTable* tp;
        for (tp = lm->staticTable; tp->name; tp++) {
            if (strcmp(name, tp->name) == 0) {
                return (void*) tp->fp;
            }
        }
        



#if !defined(WIN16) && !defined(XP_BEOS)
        PR_SetError(PR_FIND_SYMBOL_ERROR, 0);
        return (void*)NULL;
#endif
    }
    
#ifdef XP_OS2
    rc = DosQueryProcAddr(lm->dlh, 0, (PSZ) name, (PFN *) &f);
#if defined(NEED_LEADING_UNDERSCORE)
    



    if (rc != NO_ERROR) {
        name++;
        DosQueryProcAddr(lm->dlh, 0, (PSZ) name, (PFN *) &f);
    }
#endif
#endif  

#ifdef WIN32
    f = GetProcAddress(lm->dlh, name);
#endif  

#if defined(XP_MACOSX) && defined(USE_MACH_DYLD)

#define SYM_OFFSET 1
    if (lm->bundle) {
        CFStringRef nameRef = CFStringCreateWithCString(NULL, name + SYM_OFFSET, kCFStringEncodingASCII);
        if (nameRef) {
            f = CFBundleGetFunctionPointerForName(lm->bundle, nameRef);
            CFRelease(nameRef);
        }
    }
    if (lm->connection) {
        Ptr                 symAddr;
        CFragSymbolClass    symClass;
        Str255              pName;
        
        PR_LOG(_pr_linker_lm, PR_LOG_MIN, ("Looking up symbol: %s", name + SYM_OFFSET));
        
        c2pstrcpy(pName, name + SYM_OFFSET);
        
        f = (FindSymbol(lm->connection, pName, &symAddr, &symClass) == noErr) ? symAddr : NULL;
        
#ifdef __ppc__
        
        if (f && symClass == kTVectorCFragSymbol) {
            f = TV2FP(lm->wrappers, name + SYM_OFFSET, f);
        }
#endif 
        
        if (f == NULL && strcmp(name + SYM_OFFSET, "main") == 0) f = lm->main;
    }
    if (lm->image) {
        NSSymbol symbol;
        symbol = NSLookupSymbolInImage(lm->image, name,
                 NSLOOKUPSYMBOLINIMAGE_OPTION_BIND
                 | NSLOOKUPSYMBOLINIMAGE_OPTION_RETURN_ON_ERROR);
        if (symbol != NULL)
            f = NSAddressOfSymbol(symbol);
        else
            f = NULL;
    }
#undef SYM_OFFSET
#endif 

#ifdef XP_BEOS
    if( B_NO_ERROR != get_image_symbol( (image_id)lm->dlh, name, B_SYMBOL_TYPE_TEXT, &f ) ) {
        f = NULL;
    }
#endif

#ifdef XP_UNIX
#ifdef HAVE_DLL
#ifdef USE_DLFCN
    f = dlsym(lm->dlh, name);
#elif defined(USE_HPSHL)
    if (shl_findsym(&lm->dlh, name, TYPE_PROCEDURE, &f) == -1) {
        f = NULL;
    }
#elif defined(USE_MACH_DYLD)
    if (lm->dlh) {
        NSSymbol symbol;
        symbol = NSLookupSymbolInModule(lm->dlh, name);
        if (symbol != NULL)
            f = NSAddressOfSymbol(symbol);
        else
            f = NULL;
    }
#endif
#endif 
#endif 
    if (f == NULL) {
        PR_SetError(PR_FIND_SYMBOL_ERROR, _MD_ERRNO());
        DLLErrorInternal(_MD_ERRNO());
    }
    return f;
}




PR_IMPLEMENT(void*) 
PR_FindSymbol(PRLibrary *lib, const char *raw_name)
{
    void *f = NULL;
#if defined(NEED_LEADING_UNDERSCORE)
    char *name;
#else
    const char *name;
#endif
    


#if defined(NEED_LEADING_UNDERSCORE)
    
    name = PR_smprintf("_%s", raw_name);
#elif defined(AIX)
    




    name = raw_name;
#else
    name = raw_name;
#endif

    PR_EnterMonitor(pr_linker_lock);
    PR_ASSERT(lib != NULL);
    f = pr_FindSymbolInLib(lib, name);

#if defined(NEED_LEADING_UNDERSCORE)
    PR_smprintf_free(name);
#endif

    PR_ExitMonitor(pr_linker_lock);
    return f;
}




PR_IMPLEMENT(PRFuncPtr) 
PR_FindFunctionSymbol(PRLibrary *lib, const char *raw_name)
{
    return ((PRFuncPtr) PR_FindSymbol(lib, raw_name));
}

PR_IMPLEMENT(void*) 
PR_FindSymbolAndLibrary(const char *raw_name, PRLibrary* *lib)
{
    void *f = NULL;
#if defined(NEED_LEADING_UNDERSCORE)
    char *name;
#else
    const char *name;
#endif
    PRLibrary* lm;

    if (!_pr_initialized) _PR_ImplicitInitialization();
    


#if defined(NEED_LEADING_UNDERSCORE)
    
    name = PR_smprintf("_%s", raw_name);
#elif defined(AIX)
    




    name = raw_name;
#else
    name = raw_name;
#endif

    PR_EnterMonitor(pr_linker_lock);

    
    for (lm = pr_loadmap; lm != NULL; lm = lm->next) {
        f = pr_FindSymbolInLib(lm, name);
        if (f != NULL) {
            *lib = lm;
            lm->refCount++;
            PR_LOG(_pr_linker_lm, PR_LOG_MIN,
                       ("%s incr => %d (for %s)",
                    lm->name, lm->refCount, name));
            break;
        }
    }
#if defined(NEED_LEADING_UNDERSCORE)
    PR_smprintf_free(name);
#endif

    PR_ExitMonitor(pr_linker_lock);
    return f;
}

PR_IMPLEMENT(PRFuncPtr) 
PR_FindFunctionSymbolAndLibrary(const char *raw_name, PRLibrary* *lib)
{
    return ((PRFuncPtr) PR_FindSymbolAndLibrary(raw_name, lib));
}





PR_IMPLEMENT(PRLibrary*) 
PR_LoadStaticLibrary(const char *name, const PRStaticLinkTable *slt)
{
    PRLibrary *lm=NULL;
    PRLibrary* result = NULL;

    if (!_pr_initialized) _PR_ImplicitInitialization();

    
    PR_EnterMonitor(pr_linker_lock);

    
    result = pr_UnlockedFindLibrary(name);
    if (result != NULL) {
        PR_ASSERT( (result->staticTable == NULL) || (result->staticTable == slt) );
        result->staticTable = slt;
        goto unlock;
    }

    
    lm = PR_NEWZAP(PRLibrary);
    if (lm == NULL) goto unlock;

    lm->name = strdup(name);
    lm->refCount    = 1;
    lm->dlh         = pr_exe_loadmap ? pr_exe_loadmap->dlh : 0;
    lm->staticTable = slt;
    lm->next        = pr_loadmap;
    pr_loadmap      = lm;

    result = lm;    
    PR_ASSERT(lm->refCount == 1);
    PR_LOG(_pr_linker_lm, PR_LOG_MIN, ("Loaded library %s (static lib)", lm->name));
  unlock:
    PR_ExitMonitor(pr_linker_lock);
    return result;
}

PR_IMPLEMENT(char *)
PR_GetLibraryFilePathname(const char *name, PRFuncPtr addr)
{
#if defined(USE_DLFCN) && (defined(SOLARIS) || defined(FREEBSD) \
        || defined(LINUX) || defined(__GNU__) || defined(__GLIBC__) \
        || defined(DARWIN))
    Dl_info dli;
    char *result;

    if (dladdr((void *)addr, &dli) == 0) {
        PR_SetError(PR_LIBRARY_NOT_LOADED_ERROR, _MD_ERRNO());
        DLLErrorInternal(_MD_ERRNO());
        return NULL;
    }
    result = PR_Malloc(strlen(dli.dli_fname)+1);
    if (result != NULL) {
        strcpy(result, dli.dli_fname);
    }
    return result;
#elif defined(USE_MACH_DYLD)
    char *result;
    const char *image_name;
    int i, count = _dyld_image_count();

    for (i = 0; i < count; i++) {
        image_name = _dyld_get_image_name(i);
        if (strstr(image_name, name) != NULL) {
            result = PR_Malloc(strlen(image_name)+1);
            if (result != NULL) {
                strcpy(result, image_name);
            }
            return result;
        }
    }
    PR_SetError(PR_LIBRARY_NOT_LOADED_ERROR, 0);
    return NULL;
#elif defined(AIX)
    char *result;
#define LD_INFO_INCREMENT 64
    struct ld_info *info;
    unsigned int info_length = LD_INFO_INCREMENT * sizeof(struct ld_info);
    struct ld_info *infop;
    int loadflags = L_GETINFO | L_IGNOREUNLOAD;

    for (;;) {
        info = PR_Malloc(info_length);
        if (info == NULL) {
            return NULL;
        }
        
        if (loadquery(loadflags, info, info_length) != -1) {
            break;
        }
        






        if (errno == EINVAL && (loadflags & L_IGNOREUNLOAD)) {
            loadflags &= ~L_IGNOREUNLOAD;
            if (loadquery(loadflags, info, info_length) != -1) {
                break;
            }
        }
        PR_Free(info);
        if (errno != ENOMEM) {
            
            _PR_MD_MAP_DEFAULT_ERROR(_MD_ERRNO());
            return NULL;
        }
        
        info_length += LD_INFO_INCREMENT * sizeof(struct ld_info);
    }

    for (infop = info;
         ;
         infop = (struct ld_info *)((char *)infop + infop->ldinfo_next)) {
        unsigned long start = (unsigned long)infop->ldinfo_dataorg;
        unsigned long end = start + infop->ldinfo_datasize;
        if (start <= (unsigned long)addr && end > (unsigned long)addr) {
            result = PR_Malloc(strlen(infop->ldinfo_filename)+1);
            if (result != NULL) {
                strcpy(result, infop->ldinfo_filename);
            }
            break;
        }
        if (!infop->ldinfo_next) {
            PR_SetError(PR_LIBRARY_NOT_LOADED_ERROR, 0);
            result = NULL;
            break;
        }
    }
    PR_Free(info);
    return result;
#elif defined(OSF1)
    
    ldr_process_t process, ldr_my_process();
    ldr_module_t mod_id;
    ldr_module_info_t info;
    ldr_region_t regno;
    ldr_region_info_t reginfo;
    size_t retsize;
    int rv;
    char *result;

    

    process = ldr_my_process();

    

    rv = ldr_xattach(process);
    if (rv) {
        
        _PR_MD_MAP_DEFAULT_ERROR(_MD_ERRNO());
        return NULL;
    }

    

    mod_id = LDR_NULL_MODULE;

    for (;;) {

        

        ldr_next_module(process, &mod_id);
        if (ldr_inq_module(process, mod_id, &info, sizeof(info),
                           &retsize) != 0) {
            
            break;
        }
        if (retsize < sizeof(info)) {
            continue;
        }

        




        for (regno = 0; ; regno++) {
            if (ldr_inq_region(process, mod_id, regno, &reginfo,
                               sizeof(reginfo), &retsize) != 0) {
                
                break;
            }
            if (((unsigned long)reginfo.lri_mapaddr <=
                (unsigned long)addr) &&
                (((unsigned long)reginfo.lri_mapaddr + reginfo.lri_size) >
                (unsigned long)addr)) {
                
                result = PR_Malloc(strlen(info.lmi_name)+1);
                if (result != NULL) {
                    strcpy(result, info.lmi_name);
                }
                return result;
            }
        }
    }
    PR_SetError(PR_LIBRARY_NOT_LOADED_ERROR, 0);
    return NULL;
#elif defined(HPUX) && defined(USE_HPSHL)
    int index;
    struct shl_descriptor desc;
    char *result;

    for (index = 0; shl_get_r(index, &desc) == 0; index++) {
        if (strstr(desc.filename, name) != NULL) {
            result = PR_Malloc(strlen(desc.filename)+1);
            if (result != NULL) {
                strcpy(result, desc.filename);
            }
            return result;
        }
    }
    






    for (index--; index >= 0; index--) {
        if ((shl_get_r(index, &desc) == 0)
                && (strstr(desc.filename, name) != NULL)) {
            result = PR_Malloc(strlen(desc.filename)+1);
            if (result != NULL) {
                strcpy(result, desc.filename);
            }
            return result;
        }
    }
    PR_SetError(PR_LIBRARY_NOT_LOADED_ERROR, 0);
    return NULL;
#elif defined(HPUX) && defined(USE_DLFCN)
    struct load_module_desc desc;
    char *result;
    const char *module_name;

    if (dlmodinfo((unsigned long)addr, &desc, sizeof desc, NULL, 0, 0) == 0) {
        PR_SetError(PR_LIBRARY_NOT_LOADED_ERROR, _MD_ERRNO());
        DLLErrorInternal(_MD_ERRNO());
        return NULL;
    }
    module_name = dlgetname(&desc, sizeof desc, NULL, 0, 0);
    if (module_name == NULL) {
        
        _PR_MD_MAP_DEFAULT_ERROR(_MD_ERRNO());
        DLLErrorInternal(_MD_ERRNO());
        return NULL;
    }
    result = PR_Malloc(strlen(module_name)+1);
    if (result != NULL) {
        strcpy(result, module_name);
    }
    return result;
#elif defined(WIN32)
    PRUnichar wname[MAX_PATH];
    HMODULE handle = NULL;
    PRUnichar module_name[MAX_PATH];
    int len;
    char *result;

    if (MultiByteToWideChar(CP_ACP, 0, name, -1, wname, MAX_PATH)) {
        handle = GetModuleHandleW(wname);
    }
    if (handle == NULL) {
        PR_SetError(PR_LIBRARY_NOT_LOADED_ERROR, _MD_ERRNO());
        DLLErrorInternal(_MD_ERRNO());
        return NULL;
    }
    if (GetModuleFileNameW(handle, module_name, MAX_PATH) == 0) {
        
        _PR_MD_MAP_DEFAULT_ERROR(_MD_ERRNO());
        return NULL;
    }
    len = WideCharToMultiByte(CP_ACP, 0, module_name, -1,
                              NULL, 0, NULL, NULL);
    if (len == 0) {
        _PR_MD_MAP_DEFAULT_ERROR(_MD_ERRNO());
        return NULL;
    }
    result = PR_Malloc(len * sizeof(PRUnichar));
    if (result != NULL) {
        WideCharToMultiByte(CP_ACP, 0, module_name, -1,
                            result, len, NULL, NULL);
    }
    return result;
#elif defined(XP_OS2)
    HMODULE module = NULL;
    char module_name[_MAX_PATH];
    char *result;
    APIRET ulrc = DosQueryModFromEIP(&module, NULL, 0, NULL, NULL, (ULONG) addr);
    if ((NO_ERROR != ulrc) || (NULL == module) ) {
        PR_SetError(PR_LIBRARY_NOT_LOADED_ERROR, _MD_ERRNO());
        DLLErrorInternal(_MD_ERRNO());
        return NULL;
    }
    ulrc = DosQueryModuleName(module, sizeof module_name, module_name);
    if (NO_ERROR != ulrc) {
        
        _PR_MD_MAP_DEFAULT_ERROR(_MD_ERRNO());
        return NULL;
    }
    result = PR_Malloc(strlen(module_name)+1);
    if (result != NULL) {
        strcpy(result, module_name);
    }
    return result;
#else
    PR_SetError(PR_NOT_IMPLEMENTED_ERROR, 0);
    return NULL;
#endif
}
