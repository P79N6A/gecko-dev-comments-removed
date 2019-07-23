









































#if defined(PLGETOPT_H_)
#else
#define PLGETOPT_H_

#include "prtypes.h"

PR_BEGIN_EXTERN_C

typedef struct PLOptionInternal PLOptionInternal; 

typedef enum
{
        PL_OPT_OK,              
        PL_OPT_EOL,             
        PL_OPT_BAD              
} PLOptStatus;

typedef struct PLLongOpt
{
    const char * longOptName;   
    PRIntn       longOption;    
    PRBool       valueRequired; 
                                
} PLLongOpt;

typedef struct PLOptState
{
    char option;                
    const char *value;          

    PLOptionInternal *internal; 

    PRIntn   longOption;        
    PRIntn   longOptIndex;      
} PLOptState;








PR_EXTERN(PLOptState*) PL_CreateOptState(
        PRIntn argc, char **argv, const char *options);












PR_EXTERN(PLOptState*) PL_CreateLongOptState(
        PRIntn argc, char **argv, const char *options, 
        const PLLongOpt *longOpts);






PR_EXTERN(void) PL_DestroyOptState(PLOptState *opt);







































PR_EXTERN(PLOptStatus) PL_GetNextOpt(PLOptState *opt);

PR_END_EXTERN_C

#endif 



