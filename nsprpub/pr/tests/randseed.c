















































#include "plgetopt.h"
#include "nspr.h" 
#include "prrng.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>




PRLogModuleInfo *lm;
PRLogModuleLevel msgLevel = PR_LOG_NONE;
PRIntn  debug = 0;
PRUint32  failed_already = 0;


PRIntn  optRandCount = 30;
char    buf[40];
PRSize  bufSize = sizeof(buf);
PRSize  rSize;
PRIntn  i;




static void Help( void )
{
    printf("Template: Help(): display your help message(s) here");
    exit(1);
} 

static void PrintRand( void *buf, PRIntn size )
{
    PRUint32 *rp = buf;
    PRIntn   i;

    printf("%4.4d--\n", size );
    while (size > 0 ) {
        switch( size )  {
            case 1 :
                printf("%2.2X\n", *(rp++) );
                size -= 4;    
                break;
            case 2 :
                printf("%4.4X\n", *(rp++) );
                size -= 4;    
                break;
            case 3 :
                printf("%6.6X\n", *(rp++) );
                size -= 4;    
                break;
            default:
                while ( size >= 4) {
                    PRIntn i = 3;
                    do {
                        printf("%8.8X ", *(rp++) );
                        size -= 4;    
                    } while( i-- );
                    i = 3;
                    printf("\n");
                }
                break;
        } 
    } 
} 


int main(int argc, char **argv)
{
    {
        


        PLOptStatus os;
        PLOptState *opt = PL_CreateOptState(argc, argv, "hdv");

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
    for ( i = 0; i < optRandCount ; i++ ) {
        memset( buf, 0, bufSize );
        rSize = PR_GetRandomNoise( buf, bufSize );
        if (!rSize) {
            fprintf(stderr, "Not implemented\n" );
            failed_already = PR_TRUE;
            break;
        }
        if (debug) PrintRand( buf, rSize );
    }

    if (debug) printf("%s\n", (failed_already)? "FAIL" : "PASS");
    return( (failed_already == PR_TRUE )? 1 : 0 );
}  


