


















































#include "plgetopt.h"
#include "nspr.h"

#include <stdio.h>
#include <stdlib.h>

PRIntn  debug = 0;
PRIntn  verbose = 0;
PRBool  failedAlready = PR_FALSE;
const PRInt32 addedBytes = 1000;
const PRInt32   buf = 1; 
PRInt32         inBuf;   

int main(int argc, char **argv)
{
    PRStatus    rc;
    PRInt32     rv;
    PRFileDesc  *fd;
    PRIntn      i;
    PRInt32     sum = 0;

    {   
        PLOptStatus os;
        PLOptState *opt = PL_CreateOptState(argc, argv, "vd");

	    while (PL_OPT_EOL != (os = PL_GetNextOpt(opt)))
        {
		    if (PL_OPT_BAD == os) continue;
            switch (opt->option)
            {
            case 'd':  
                debug = 1;
                break;
            case 'v':  
                verbose = 1;
                break;
             default:
                break;
            }
        }
	    PL_DestroyOptState(opt);
    } 

    fd = PR_Open( "/tmp/nsprAppend", (PR_APPEND | PR_CREATE_FILE | PR_TRUNCATE | PR_WRONLY), 0666 );
    if ( NULL == fd )  {
        if (debug) printf("PR_Open() failed for writing: %d\n", PR_GetError());
        failedAlready = PR_TRUE;
        goto Finished;
    }

    for ( i = 0; i < addedBytes ; i++ ) {
        rv = PR_Write( fd, &buf, sizeof(buf));
        if ( sizeof(buf) != rv )  {
            if (debug) printf("PR_Write() failed: %d\n", PR_GetError());
            failedAlready = PR_TRUE;
            goto Finished;
        }
        rv = PR_Seek( fd, 0 , PR_SEEK_SET );
        if ( -1 == rv )  {
            if (debug) printf("PR_Seek() failed: %d\n", PR_GetError());
            failedAlready = PR_TRUE;
            goto Finished;
        }
    }
    rc = PR_Close( fd );
    if ( PR_FAILURE == rc ) {
        if (debug) printf("PR_Close() failed after writing: %d\n", PR_GetError());
        failedAlready = PR_TRUE;
        goto Finished;
    }

    fd = PR_Open( "/tmp/nsprAppend", PR_RDONLY, 0 );
    if ( NULL == fd )  {
        if (debug) printf("PR_Open() failed for reading: %d\n", PR_GetError());
        failedAlready = PR_TRUE;
        goto Finished;
    }

    for ( i = 0; i < addedBytes ; i++ ) {
        rv = PR_Read( fd, &inBuf, sizeof(inBuf));
        if ( sizeof(inBuf) != rv)  {
            if (debug) printf("PR_Write() failed: %d\n", PR_GetError());
            failedAlready = PR_TRUE;
            goto Finished;
        }
        sum += inBuf;
    }

    rc = PR_Close( fd );
    if ( PR_FAILURE == rc ) {
        if (debug) printf("PR_Close() failed after reading: %d\n", PR_GetError());
        failedAlready = PR_TRUE;
        goto Finished;
    }
    if ( sum != addedBytes )  {
        if (debug) printf("Uh Oh! addedBytes: %d. Sum: %d\n", addedBytes, sum);
        failedAlready = PR_TRUE;
        goto Finished;
    }


Finished:
    if (debug || verbose) printf("%s\n", (failedAlready)? "FAILED" : "PASSED" );
    return( (failedAlready)? 1 : 0 );
}  


