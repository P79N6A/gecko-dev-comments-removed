





























































#include <plgetopt.h> 
#include <nspr.h> 
#include <private/primpl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>




PRLogModuleInfo *lm;
PRLogModuleLevel msgLevel = PR_LOG_NONE;
PRUint32  failed_already = 0;

PRIntn  debug = 0;
PRIntn  client = 0; 
char    dirName[512] = "."; 
PRSize  fmSize = (100 * 1024 );
PRUint32 fmMode = 0600;
PRFileMapProtect fmProt = PR_PROT_READWRITE;
const char *fmEnvName = "nsprFileMapEnvVariable";




static void Help( void )
{
    printf("anonfm [options] [dirName]\n");
    printf("-d -- enable debug mode\n");
    printf("dirName is alternate directory name. Default: . (current directory)\n");
    exit(1);
} 





static void ClientOne( void )
{
    PRFileMap   *fm;
    char        *fmString;
    char        *addr;
    PRStatus    rc;

    PR_LOG(lm, msgLevel,
        ("ClientOne() starting"));
    
    fmString = PR_GetEnv( fmEnvName );
    if ( NULL == fmString ) {
        failed_already = 1;    
        PR_LOG(lm, msgLevel,
                ("ClientOne(): PR_Getenv() failed"));
        return;
    }
    PR_LOG(lm, msgLevel,
        ("ClientOne(): PR_Getenv(): found: %s", fmString));

    fm = PR_ImportFileMapFromString( fmString );
    if ( NULL == fm ) {
        failed_already = 1;    
        PR_LOG(lm, msgLevel,
                ("ClientOne(): PR_ImportFileMapFromString() failed"));
        return;
    }
    PR_LOG(lm, msgLevel,
        ("ClientOne(): PR_ImportFileMapFromString(): fm: %p", fm ));

    addr = PR_MemMap( fm, LL_ZERO, fmSize );
    if ( NULL == addr ) {
        failed_already = 1;    
        PR_LOG(lm, msgLevel,
            ("ClientOne(): PR_MemMap() failed, OSError: %d", PR_GetOSError() ));
        return;
    }
    PR_LOG(lm, msgLevel,
        ("ClientOne(): PR_MemMap(): addr: %p", addr ));

    
    *addr = 1;

    rc = PR_MemUnmap( addr, fmSize );
    PR_ASSERT( rc == PR_SUCCESS );
    PR_LOG(lm, msgLevel,
        ("ClientOne(): PR_MemUnap(): success" ));

    rc = PR_CloseFileMap( fm );
    if ( PR_FAILURE == rc ) {
        failed_already = 1;    
        PR_LOG(lm, msgLevel,
            ("ClientOne(): PR_MemUnap() failed, OSError: %d", PR_GetOSError() ));
        return;
    }
    PR_LOG(lm, msgLevel,
        ("ClientOne(): PR_CloseFileMap(): success" ));

    return;
} 




static void ClientTwo( void )
{
    failed_already = 1;
} 




static void ServerOne( void )
{
    PRFileMap   *fm;
    PRStatus    rc;
    PRIntn      i;
    char        *addr;
    char        fmString[256];
    char        envBuf[256];
    char        *child_argv[8];
    PRProcess   *proc;
    PRInt32     exit_status;

    PR_LOG(lm, msgLevel,
        ("ServerOne() starting"));
    
    fm = PR_OpenAnonFileMap( dirName, fmSize, fmProt );
    if ( NULL == fm )      {
        failed_already = 1;    
        PR_LOG(lm, msgLevel,
                ("PR_OpenAnonFileMap() failed"));
        return;
    }
    PR_LOG(lm, msgLevel,
        ("ServerOne(): FileMap: %p", fm ));
    
    rc = PR_ExportFileMapAsString( fm, sizeof(fmString), fmString );
    if ( PR_FAILURE == rc )  {
        failed_already = 1;    
        PR_LOG(lm, msgLevel,
            ("PR_ExportFileMap() failed"));
        return;
    }

    


    PR_snprintf( envBuf, sizeof(envBuf), "%s=%s", fmEnvName, fmString);
    putenv( envBuf );
    
    addr = PR_MemMap( fm, LL_ZERO, fmSize );
    if ( NULL == addr ) {
        failed_already = 1;    
        PR_LOG(lm, msgLevel,
            ("PR_MemMap() failed"));
        return;
    }

    
    for (i = 0; i < (PRIntn)fmSize ; i++ )
        *(addr+i) = 0x00;  

    PR_LOG(lm, msgLevel,
        ("ServerOne(): PR_MemMap(): addr: %p", addr ));
    
    


    child_argv[0] = "anonfm";
    child_argv[1] = "-C";
    child_argv[2] = "1";
    child_argv[3] = NULL;

    proc = PR_CreateProcess(child_argv[0], child_argv, NULL, NULL);
    PR_ASSERT( proc );
    PR_LOG(lm, msgLevel,
        ("ServerOne(): PR_CreateProcess(): proc: %x", proc ));

    


    PR_LOG(lm, msgLevel,
        ("ServerOne(): waiting on Client, *addr: %x", *addr ));
    while( *addr == 0x00 ) {
        if ( debug )
            fprintf(stderr, ".");
        PR_Sleep(PR_MillisecondsToInterval(300));
    }
    if ( debug )
        fprintf(stderr, "\n");
    PR_LOG(lm, msgLevel,
        ("ServerOne(): Client responded" ));

    rc = PR_WaitProcess( proc, &exit_status );
    PR_ASSERT( PR_FAILURE != rc );

    rc = PR_MemUnmap( addr, fmSize);
    if ( PR_FAILURE == rc ) {
        failed_already = 1;    
        PR_LOG(lm, msgLevel,
            ("PR_MemUnmap() failed"));
        return;
    }
    PR_LOG(lm, msgLevel,
        ("ServerOne(): PR_MemUnmap(): success" ));

    rc = PR_CloseFileMap(fm);
    if ( PR_FAILURE == rc ) {
        failed_already = 1;    
        PR_LOG(lm, msgLevel,
            ("PR_CloseFileMap() failed"));
        return;
    }
    PR_LOG(lm, msgLevel,
        ("ServerOne(): PR_CloseFileMap() success" ));

    return;
} 




static void ServerTwo( void )
{
    PR_LOG(lm, msgLevel,
        ("ServerTwo(): Not implemented yet" ));
} 


int main(int argc, char **argv)
{
    {
        


        PLOptStatus os;
        PLOptState *opt = PL_CreateOptState(argc, argv, "hdC:");

	    while (PL_OPT_EOL != (os = PL_GetNextOpt(opt)))
        {
		    if (PL_OPT_BAD == os) continue;
            switch (opt->option)
            {
            case 'C':  
                client = atol(opt->value);
                break;
            case 's':  
                fmSize = atol( opt->value ) * 1024;
                break;
            case 'd':  
                debug = 1;
			    msgLevel = PR_LOG_DEBUG;
                break;
            case 'h':  
			    Help();
                break;
             default:
                strcpy(dirName, opt->value);
                break;
            }
        }
	    PL_DestroyOptState(opt);
    }

    lm = PR_NewLogModule("Test");       

    if ( client == 1 ) {
        ClientOne();
    } else if ( client == 2 )  {
        ClientTwo();
    } else {
        ServerOne();
        if ( failed_already ) goto Finished;
        ServerTwo();
    }

Finished:
    if ( debug )
        printf("%s\n", (failed_already)? "FAIL" : "PASS");
    return( (failed_already == PR_TRUE )? 1 : 0 );
}  


