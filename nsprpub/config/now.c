




#include <stdio.h>
#include <time.h>

int main(int argc, char **argv)
{
#if defined(OMIT_LIB_BUILD_TIME)
    









#elif defined(_MSC_VER)
    __int64 now;
    time_t sec;

    sec = time(NULL);
    now = (1000000i64) * sec;
    fprintf(stdout, "%I64d", now);
#else
    long long now;
    time_t sec;

    sec = time(NULL);
    now = (1000000LL) * sec;
    fprintf(stdout, "%lld", now);
#endif

    return 0;
}  


