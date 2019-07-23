















































#include "prlog.h"
#include "prinit.h"
#include "prio.h"

#include <stdio.h>
#include <string.h>

static PRIntn PR_CALLBACK stdio(PRIntn argc, char **argv)
{
    PRInt32 rv;

    PRFileDesc *out = PR_GetSpecialFD(PR_StandardOutput);
    PRFileDesc *err = PR_GetSpecialFD(PR_StandardError);

    rv = PR_Write(
        out, "This to standard out\n",
        strlen("This to standard out\n"));
    PR_ASSERT((PRInt32)strlen("This to standard out\n") == rv);
    rv = PR_Write(
        err, "This to standard err\n",
        strlen("This to standard err\n"));
    PR_ASSERT((PRInt32)strlen("This to standard err\n") == rv);

    return 0;

}  

int main(int argc, char **argv)
{
    PR_STDIO_INIT();
    return PR_Initialize(stdio, argc, argv, 0);
}  



