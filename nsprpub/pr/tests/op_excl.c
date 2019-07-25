














#include <plgetopt.h> 
#include <nspr.h> 
#include <stdio.h>
#include <stdlib.h>




PRLogModuleInfo *lm;
PRLogModuleLevel msgLevel = PR_LOG_NONE;
PRIntn  debug = 0;
PRUint32  failed_already = 0;




static void Help( void )
{
    printf("op_excl: Help");
    printf("op_excl [-d]");
    printf("-d enables debug messages");
    exit(1);
} 



int main(int argc, char **argv)
{
    PRFileDesc  *fd;
    PRStatus    rv;
    PRInt32     written;
    char        outBuf[] = "op_excl.c test file";
#define OUT_SIZE sizeof(outBuf)
#define NEW_FILENAME "xxxExclNewFile"

    {
        


        PLOptStatus os;
        PLOptState *opt = PL_CreateOptState(argc, argv, "hd");

	    while (PL_OPT_EOL != (os = PL_GetNextOpt(opt)))
        {
		    if (PL_OPT_BAD == os) continue;
            switch (opt->option)
            {
            case 'd':  
                debug = 1;
			    msgLevel = PR_LOG_ERROR;
                break;
            case 'h':  
			    Help();
                break;
             default:
                break;
            }
        }
	    PL_DestroyOptState(opt);
    }

    lm = PR_NewLogModule("Test");       

    


    fd = PR_Open( NEW_FILENAME, PR_CREATE_FILE | PR_EXCL | PR_WRONLY, 0666 );
    if ( NULL == fd )  {
        if (debug) fprintf( stderr, "Open exclusive. Expected success, got failure\n");
        failed_already = 1;
        goto Finished;
    }

    written = PR_Write( fd, outBuf, OUT_SIZE );
    if ( OUT_SIZE != written )  {
        if (debug) fprintf( stderr, "Write after open exclusive failed\n");
        failed_already = 1;
        goto Finished;
    }

    rv = PR_Close(fd);
    if ( PR_FAILURE == rv )  {
        if (debug) fprintf( stderr, "Close after open exclusive failed\n");
        failed_already = 1;
        goto Finished;
    }

    


    fd = PR_Open( NEW_FILENAME, PR_CREATE_FILE | PR_EXCL | PR_WRONLY, 0666 );
    if ( NULL != fd )  {
        if (debug) fprintf( stderr, "Open exclusive. Expected failure, got success\n");
        failed_already = 1;
        PR_Close(fd);
    }

    rv = PR_Delete( NEW_FILENAME );
    if ( PR_FAILURE == rv ) {
        if (debug) fprintf( stderr, "PR_Delete() failed\n");
        failed_already = 1;
    }

Finished:
    if (debug) printf("%s\n", (failed_already)? "FAIL" : "PASS");
    return( (failed_already == PR_TRUE )? 1 : 0 );
}  


