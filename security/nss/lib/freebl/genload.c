















#ifdef XP_UNIX
#include <unistd.h>
#define BL_MAXSYMLINKS 20








static char* loader_GetOriginalPathname(const char* link)
{
#ifdef __GLIBC__
    char* tmp = realpath(link, NULL);
    char* resolved;
    if (! tmp)
    	return NULL;
    resolved = PR_Malloc(strlen(tmp) + 1);
    strcpy(resolved, tmp); 
    free(tmp);
    return resolved;
#else
    char* resolved = NULL;
    char* input = NULL;
    PRUint32 iterations = 0;
    PRInt32 len = 0, retlen = 0;
    if (!link) {
        PR_SetError(PR_INVALID_ARGUMENT_ERROR, 0);
        return NULL;
    }
    len = PR_MAX(1024, strlen(link) + 1);
    resolved = PR_Malloc(len);
    input = PR_Malloc(len);
    if (!resolved || !input) {
        if (resolved) {
            PR_Free(resolved);
        }
        if (input) {
            PR_Free(input);
        }
        return NULL;
    }
    strcpy(input, link);
    while ( (iterations++ < BL_MAXSYMLINKS) &&
            ( (retlen = readlink(input, resolved, len - 1)) > 0) ) {
        char* tmp = input;
        resolved[retlen] = '\0'; 
        input = resolved;
        resolved = tmp;
    }
    PR_Free(resolved);
    if (iterations == 1 && retlen < 0) {
        PR_Free(input);
        input = NULL;
    }
    return input;
#endif
}
#endif 





static PRLibrary *
loader_LoadLibInReferenceDir(const char *referencePath, const char *name)
{
    PRLibrary *dlh = NULL;
    char *fullName = NULL;
    char* c;
    PRLibSpec libSpec;

    
    c = strrchr(referencePath, PR_GetDirectorySeparator());
    if (c) {
        size_t referencePathSize = 1 + c - referencePath;
        fullName = (char*) PORT_Alloc(strlen(name) + referencePathSize + 1);
        if (fullName) {
            memcpy(fullName, referencePath, referencePathSize);
            strcpy(fullName + referencePathSize, name); 
#ifdef DEBUG_LOADER
            PR_fprintf(PR_STDOUT, "\nAttempting to load fully-qualified %s\n", 
                       fullName);
#endif
            libSpec.type = PR_LibSpec_Pathname;
            libSpec.value.pathname = fullName;
            dlh = PR_LoadLibraryWithFlags(libSpec, PR_LD_NOW | PR_LD_LOCAL);
            PORT_Free(fullName);
        }
    }
    return dlh;
}







static PRLibrary *
loader_LoadLibrary(const char *nameToLoad)
{
    PRLibrary *lib = NULL;
    char* fullPath = NULL;
    PRLibSpec libSpec;

    






    fullPath = PR_GetLibraryFilePathname(NameOfThisSharedLib,
                                         (PRFuncPtr)&loader_LoadLibrary);

    if (fullPath) {
        lib = loader_LoadLibInReferenceDir(fullPath, nameToLoad);
#ifdef XP_UNIX
        if (!lib) {
            



            char* originalfullPath = loader_GetOriginalPathname(fullPath);
            if (originalfullPath) {
                PR_Free(fullPath);
                fullPath = originalfullPath;
                lib = loader_LoadLibInReferenceDir(fullPath, nameToLoad);
            }
        }
#endif
        PR_Free(fullPath);
    }
    if (!lib) {
#ifdef DEBUG_LOADER
        PR_fprintf(PR_STDOUT, "\nAttempting to load %s\n", nameToLoad);
#endif
        libSpec.type = PR_LibSpec_Pathname;
        libSpec.value.pathname = nameToLoad;
        lib = PR_LoadLibraryWithFlags(libSpec, PR_LD_NOW | PR_LD_LOCAL);
    }
    if (NULL == lib) {
#ifdef DEBUG_LOADER
        PR_fprintf(PR_STDOUT, "\nLoading failed : %s.\n", nameToLoad);
#endif
    }
    return lib;
}

