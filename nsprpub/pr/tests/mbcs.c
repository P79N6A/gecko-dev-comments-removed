



































































#include <plgetopt.h> 
#include <nspr.h> 
#include <stdio.h>
#include <stdlib.h>
#include <string.h>




PRLogModuleInfo *lm;
PRLogModuleLevel msgLevel = PR_LOG_NONE;
PRIntn  debug = 0;
PRUint32  failed_already = 0;


char *dirName =  NULL;  




static void TraverseDirectory( unsigned char *dir )
{
    PRDir *cwd;
    PRDirEntry *dirEntry;
    PRFileInfo info;
    PRStatus rc;
    PRInt32 err;
    PRFileDesc *fd;
    char    nextDir[256];
    char    file[256];

    printf("Directory: %s\n", dir );
    cwd = PR_OpenDir( dir );
    if ( NULL == cwd )  {
        printf("PR_OpenDir() failed on directory: %s, with error: %d, %d\n", 
            dir, PR_GetError(), PR_GetOSError());
        exit(1);
    }
    while( NULL != (dirEntry = PR_ReadDir( cwd, PR_SKIP_BOTH | PR_SKIP_HIDDEN )))  {
        sprintf( file, "%s/%s", dir, dirEntry->name );
        rc = PR_GetFileInfo( file, &info );
        if ( PR_FAILURE == rc ) {
            printf("PR_GetFileInfo() failed on file: %s, with error: %d, %d\n", 
                dirEntry->name, PR_GetError(), PR_GetOSError());
            exit(1);
        }
        if ( PR_FILE_FILE == info.type )  {
            printf("File: %s \tsize: %ld\n", dirEntry->name, info.size );
            fd = PR_Open( file, PR_RDONLY, 0 );
            if ( NULL == fd )  {
                printf("PR_Open() failed. Error: %ld, OSError: %ld\n", 
                    PR_GetError(), PR_GetOSError());
            }
            rc = PR_Close( fd );
            if ( PR_FAILURE == rc )  {
                printf("PR_Close() failed. Error: %ld, OSError: %ld\n", 
                    PR_GetError(), PR_GetOSError());
            }
        } else if ( PR_FILE_DIRECTORY == info.type ) {
            sprintf( nextDir, "%s/%s", dir, dirEntry->name );
            TraverseDirectory(nextDir);
        } else {
            printf("type is not interesting for file: %s\n", dirEntry->name );
            
        }
    }
    

    rc = PR_CloseDir( cwd );
    if ( PR_FAILURE == rc ) {
        printf("PR_CloseDir() failed on directory: %s, with error: %d, %d\n", 
            dir, PR_GetError(), PR_GetOSError());
    }

} 

int main(int argc, char **argv)
{
    { 
        


        PLOptStatus os;
        PLOptState *opt = PL_CreateOptState(argc, argv, "dv");

	    while (PL_OPT_EOL != (os = PL_GetNextOpt(opt)))
        {
		    if (PL_OPT_BAD == os) continue;
            switch (opt->option)
            {
            case 'd':  
                debug = 1;
			    msgLevel = PR_LOG_ERROR;
                break;
            case 'v':  
			    msgLevel = PR_LOG_DEBUG;
                break;
             default:
                dirName = strdup(opt->value); 
                break; 
            }
        }
	    PL_DestroyOptState(opt);
    } 

    lm = PR_NewLogModule("Test");       

    
    if ( dirName == NULL )  {
        printf("you gotta specify a directory as an operand!\n");
        exit(1);
    }

    TraverseDirectory( dirName );

    if (debug) printf("%s\n", (failed_already)? "FAIL" : "PASS");
    return( (failed_already == PR_TRUE )? 1 : 0 );
}  

