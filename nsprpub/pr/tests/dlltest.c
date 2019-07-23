
























































#include "prinit.h"
#include "prlink.h"
#include "prmem.h"
#include "prerror.h"

#include "plstr.h"

#include <stdio.h>
#include <stdlib.h>

typedef PRIntn (PR_CALLBACK *GetFcnType)(void);
typedef void (PR_CALLBACK *SetFcnType)(PRIntn);

PRIntn failed_already=0;
PRIntn debug_mode;

int main(int argc, char** argv)
{
    PRLibrary *lib, *lib2;  
    GetFcnType getFcn;
    SetFcnType setFcn;
    PRIntn value;
    PRStatus status;
    char *libName;

    if (argc >= 2 && PL_strcmp(argv[1], "-d") == 0) {
        debug_mode = 1;
    }

    PR_Init(PR_USER_THREAD, PR_PRIORITY_NORMAL, 0);
    PR_STDIO_INIT();

    




    libName = PR_GetLibraryName("dll", "my");
    if (debug_mode) printf("Loading library %s\n", libName);
    lib = PR_LoadLibrary(libName);
    PR_FreeLibraryName(libName);
    if (lib == NULL) {
        PRInt32 textLength = PR_GetErrorTextLength();
        char *text = (char*)PR_MALLOC(textLength + 1);
        text[0] = '\0';
        (void)PR_GetErrorText(text);
        fprintf(
            stderr, "PR_LoadLibrary failed (%d, %d, %s)\n",
            PR_GetError(), PR_GetOSError(), text);
        if (!debug_mode) failed_already=1;
    }
    getFcn = (GetFcnType) PR_FindSymbol(lib, "My_GetValue");
    setFcn = (SetFcnType) PR_FindFunctionSymbol(lib, "My_SetValue");
    (*setFcn)(888);
    value = (*getFcn)();
    if (value != 888) {
        fprintf(stderr, "Test 1 failed: set value to 888, but got %d\n", value);
        if (!debug_mode) failed_already=1;
    }
    if (debug_mode) printf("Test 1 passed\n");

    





    getFcn = (GetFcnType) PR_FindSymbolAndLibrary("My_GetValue", &lib2);
    if (NULL == getFcn || lib != lib2) {
        fprintf(stderr, "Test 2 failed: handles for the same library are not "
            "equal: handle 1: %p, handle 2: %p\n", lib, lib2);
        if (!debug_mode) failed_already=1;
    }
    setFcn = (SetFcnType) PR_FindSymbol(lib2, "My_SetValue");
    value = (*getFcn)();
    if (value != 888) {
        fprintf(stderr, "Test 2 failed: value should be 888, but got %d\n",
            value);
        if (!debug_mode) failed_already=1;
    }
    (*setFcn)(777);
    value = (*getFcn)();
    if (value != 777) {
        fprintf(stderr, "Test 2 failed: set value to 777, but got %d\n", value);
        if (!debug_mode) failed_already=1;
        goto exit_now;
    }
    if (debug_mode) printf("Test 2 passed\n");

    




    status = PR_UnloadLibrary(lib);
    if (PR_FAILURE == status) {
        fprintf(stderr, "Test 3 failed: cannot unload library: (%d, %d)\n",
            PR_GetError(), PR_GetOSError());
        if (!debug_mode) failed_already=1;
        goto exit_now;
    }
    getFcn = (GetFcnType) PR_FindFunctionSymbol(lib2, "My_GetValue");
    setFcn = (SetFcnType) PR_FindSymbol(lib2, "My_SetValue");
    (*setFcn)(666);
    value = (*getFcn)();
    if (value != 666) {
        fprintf(stderr, "Test 3 failed: set value to 666, but got %d\n", value);
        if (!debug_mode) failed_already=1;
        goto exit_now;
    }
    if (debug_mode) printf("Test 3 passed\n");

    



    status = PR_UnloadLibrary(lib2);
    if (PR_FAILURE == status) {
        fprintf(stderr, "Test 4 failed: cannot unload library: (%d, %d)\n",
            PR_GetError(), PR_GetOSError());
        if (!debug_mode) failed_already=1;
        goto exit_now;
    }
    getFcn = (GetFcnType) PR_FindFunctionSymbolAndLibrary("My_GetValue", &lib2);
    if (NULL != getFcn) {
        fprintf(stderr, "Test 4 failed: how can we find a symbol "
            "in an already unloaded library?\n");
        if (!debug_mode) failed_already=1;
        goto exit_now;
    }
    if (debug_mode) {
        printf("Test 4 passed\n");
    }

    


    {
        PRStaticLinkTable   slt[10];
        PRLibrary           *lib;
        
        lib = PR_LoadStaticLibrary( "my.dll", slt );
        if ( lib == NULL )
        {
            fprintf(stderr, "Test 5: LoadStatiLibrary() failed\n" );
            goto exit_now;
        }
        if (debug_mode)
        {
            printf("Test 5 passed\n");
        }
    }

    goto exit_now;
exit_now: 
    PR_Cleanup();

    if (failed_already) {
        printf("FAILED\n");
        return 1;
    } else {
        printf("PASSED\n");
        return 0;
    }
}
