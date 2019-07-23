




































#include <stdio.h>
#include <stdlib.h>

#if defined(XP_UNIX) || defined(XP_OS2) || defined(XP_BEOS)
#include <sys/time.h>
#elif defined(_WIN32)
#include <windows.h>
#else
#error "Architecture not supported"
#endif


int main(int argc, char **argv)
{
#if defined(OMIT_LIB_BUILD_TIME)
    









#elif defined(XP_UNIX) || defined(XP_OS2) || defined(XP_BEOS)
    long long now;
    struct timeval tv;
#ifdef HAVE_SVID_GETTOD
    gettimeofday(&tv);
#else
    gettimeofday(&tv, NULL);
#endif
    now = ((1000000LL) * tv.tv_sec) + (long long)tv.tv_usec;
#if defined(OSF1)
    fprintf(stdout, "%ld", now);
#elif defined(BEOS) && defined(__POWERPC__)
    fprintf(stdout, "%Ld", now);  
#else
    fprintf(stdout, "%lld", now);
#endif

#elif defined(_WIN32)
    __int64 now;
    FILETIME ft;
    GetSystemTimeAsFileTime(&ft);
    CopyMemory(&now, &ft, sizeof(now));
    



#ifdef __GNUC__
    now = (now - 116444736000000000LL) / 10LL;
    fprintf(stdout, "%lld", now);
#else
    now = (now - 116444736000000000i64) / 10i64;
    fprintf(stdout, "%I64d", now);
#endif

#else
#error "Architecture not supported"
#endif

    return 0;
}  


