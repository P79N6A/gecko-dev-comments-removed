













































#if !defined(XP_UNIX)





int main()
{
    return 0;
}

#else 

#include "nspr.h"

#include <sys/time.h>
#include <stdio.h>
#ifdef SYMBIAN
#include <sys/select.h>
#endif

int main(int argc, char **argv)
{
    struct timeval timeout;
    int rv;

    PR_SetError(0, 0);  
    PR_EnableClockInterrupts();

    
    timeout.tv_sec = 2;
    timeout.tv_usec = 0;
    rv = select(1, NULL, NULL, NULL, &timeout);
    printf("select returned %d\n", rv);
    return 0;
}

#endif 
