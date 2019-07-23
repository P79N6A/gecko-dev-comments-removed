















































#include <prio.h>
#include <stdio.h>
#include <prerror.h>
#include <prlog.h>
#include "plgetopt.h"

#define DIRNAME "xxxBug80884/"
#define FILENAME "file80883"

PRBool  failed_already = PR_FALSE;
PRBool	debug_mode = PR_FALSE;

PRLogModuleInfo     *lm;




static void Help( void )  {
    fprintf(stderr, "template usage:\n"
                    "\t-d     debug mode\n"
                    );
} 

int main(int argc, char **argv)
{
    PLOptStatus os;
    PLOptState *opt = PL_CreateOptState(argc, argv, "dh");
    PRFileDesc* fd;
    PRErrorCode err;

    
    while (PL_OPT_EOL != (os = PL_GetNextOpt(opt))) {
        if (PL_OPT_BAD == os) continue;
        switch (opt->option) {
            case 'd':  
                debug_mode = PR_TRUE;
                break;
            case 'h':
            default:
                Help();
                return 2;
        }
    }
    PL_DestroyOptState(opt);

    lm = PR_NewLogModule( "testcase" );

    (void) PR_MkDir( DIRNAME, 0777);
    fd = PR_Open( DIRNAME FILENAME, PR_CREATE_FILE|PR_RDWR, 0666);
    if (fd == 0) {
        PRErrorCode err = PR_GetError();
        fprintf(stderr, "create file fails: %d: %s\n", err,
            PR_ErrorToString(err, PR_LANGUAGE_I_DEFAULT));
        failed_already = PR_TRUE;
        goto Finished;
    }
 
    PR_Close(fd);

    if (PR_RmDir( DIRNAME ) == PR_SUCCESS) {
        fprintf(stderr, "remove directory succeeds\n");
        failed_already = PR_TRUE;
        goto Finished;
    }
 
    err = PR_GetError();
    fprintf(stderr, "remove directory fails with: %d: %s\n", err,
        PR_ErrorToString(err, PR_LANGUAGE_I_DEFAULT));

    (void) PR_Delete( DIRNAME FILENAME);
    (void) PR_RmDir( DIRNAME );

    return 0;

Finished:
    if ( debug_mode ) printf("%s\n", ( failed_already ) ? "FAILED" : "PASS" );
    return( (failed_already)? 1 : 0 );
}  

