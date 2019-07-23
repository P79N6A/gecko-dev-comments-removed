









































#include "nspr.h"

#include <stdio.h>
#include <stdlib.h>

static void Noop(void) { }

static void Fail(void)
{
    printf("FAIL\n");
    exit(1);
}

int main(int argc, char **argv)
{
    int foo = 1;
    char *ptr = NULL;

    
    if (foo)
        PR_DELETE(ptr);
    else
        Noop();

    
    if (foo)
        PR_FREEIF(ptr);
    else
        Fail();

    printf("PASS\n");
    return 0;
}
