







































#include <dlfcn.h>
#include <android/log.h>

int
main(int argc, char* argv[])
{
    
    
    if (argc < 2)
        return 1;

    void *mozloader_handle = dlopen("libmozutils.so", RTLD_LAZY);
    if (!mozloader_handle) {
      __android_log_print(ANDROID_LOG_ERROR, "GeckoChildLoad",
                          "Couldn't load mozloader because %s", dlerror());
        return 1;
    }

    typedef int (*ChildProcessInit_t)(int, char**);
    ChildProcessInit_t fChildProcessInit =
        (ChildProcessInit_t)dlsym(mozloader_handle, "ChildProcessInit");
    if (!fChildProcessInit) {
        __android_log_print(ANDROID_LOG_ERROR, "GeckoChildLoad",
                            "Couldn't load cpi_t because %s", dlerror());
        return 1;
    }

    return fChildProcessInit(argc, argv);
}
