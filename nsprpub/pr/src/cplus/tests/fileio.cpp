






































#include "rcfileio.h"

#include <prlog.h>
#include <prprf.h>

#define DEFAULT_ITERATIONS 100

PRIntn main(PRIntn argc, char **argv)
{
    PRStatus rv;
    RCFileIO fd;
    RCFileInfo info;
    rv = fd.Open("filio.dat", PR_CREATE_FILE, 0666);
    PR_ASSERT(PR_SUCCESS == rv);
    rv = fd.FileInfo(&info);
    PR_ASSERT(PR_SUCCESS == rv);
    rv = fd.Delete("filio.dat");
    PR_ASSERT(PR_SUCCESS == rv);
    fd.Close();
    PR_ASSERT(PR_SUCCESS == rv);

    return 0;
}  



