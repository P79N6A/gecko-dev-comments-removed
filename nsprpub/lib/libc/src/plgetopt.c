









































#include "prmem.h"
#include "prlog.h"
#include "prerror.h"
#include "plstr.h"
#include "plgetopt.h"

#include <string.h>

static char static_Nul = 0;

struct PLOptionInternal
{
    const char *options;        
    PRIntn argc;                
    char **argv;                
    PRIntn xargc;               
    const char *xargv;          
    PRIntn minus;               
    const PLLongOpt *longOpts;  
    PRBool endOfOpts;           
    PRIntn optionsLen;          
};








PR_IMPLEMENT(PLOptState*) PL_CreateOptState(
    PRIntn argc, char **argv, const char *options)
{
    return PL_CreateLongOptState( argc, argv, options, NULL);
}  

PR_IMPLEMENT(PLOptState*) PL_CreateLongOptState(
    PRIntn argc, char **argv, const char *options, 
    const PLLongOpt *longOpts)
{
    PLOptState *opt = NULL;
    PLOptionInternal *internal;

    if (NULL == options) 
    {
        PR_SetError(PR_INVALID_ARGUMENT_ERROR, 0);
        return opt;
    }

    opt = PR_NEWZAP(PLOptState);
    if (NULL == opt) 
    {
        PR_SetError(PR_OUT_OF_MEMORY_ERROR, 0);
        return opt;
    }

    internal = PR_NEW(PLOptionInternal);
    if (NULL == internal)
    {
        PR_DELETE(opt);
        PR_SetError(PR_OUT_OF_MEMORY_ERROR, 0);
        return NULL;
    }

    opt->option = 0;
    opt->value = NULL;
    opt->internal = internal;
    opt->longOption   =  0;
    opt->longOptIndex = -1;

    internal->argc = argc;
    internal->argv = argv;
    internal->xargc = 0;
    internal->xargv = &static_Nul;
    internal->minus = 0;
    internal->options = options;
    internal->longOpts = longOpts;
    internal->endOfOpts = PR_FALSE;
    internal->optionsLen = PL_strlen(options);

    return opt;
}  




PR_IMPLEMENT(void) PL_DestroyOptState(PLOptState *opt)
{
    PR_DELETE(opt->internal);
    PR_DELETE(opt);
}  

PR_IMPLEMENT(PLOptStatus) PL_GetNextOpt(PLOptState *opt)
{
    PLOptionInternal *internal = opt->internal;

    opt->longOption   =  0;
    opt->longOptIndex = -1;
    






    while (0 == *internal->xargv)
    {
        internal->xargc += 1;
        if (internal->xargc >= internal->argc)
        {
            opt->option = 0;
            opt->value = NULL;
            return PL_OPT_EOL;
        }
        internal->xargv = internal->argv[internal->xargc];
        internal->minus = 0;
        if (!internal->endOfOpts && ('-' == *internal->xargv)) 
        {
            internal->minus++;
            internal->xargv++;  
            if ('-' == *internal->xargv && internal->longOpts) 
            {
                internal->minus++;
                internal->xargv++;
                if (0 == *internal->xargv) 
                {
                    internal->endOfOpts = PR_TRUE;
                }
            }
        }
    }

    




    if (internal->minus == 2) 
    {
        char * foundEqual = strchr(internal->xargv,'=');
        PRIntn optNameLen = foundEqual ? (foundEqual - internal->xargv) :
                            strlen(internal->xargv);
        const PLLongOpt *longOpt = internal->longOpts;
        PLOptStatus result = PL_OPT_BAD;

        opt->option = 0;
        opt->value  = NULL;

        for (; longOpt->longOptName; ++longOpt) 
        {
            if (strncmp(longOpt->longOptName, internal->xargv, optNameLen))
                continue;  
            if (strlen(longOpt->longOptName) != optNameLen)
                continue;  
            
            opt->longOptIndex = longOpt - internal->longOpts;
            opt->longOption   = longOpt->longOption;
            
            

            if (foundEqual) 
            {
                opt->value = foundEqual + 1;
            }
            else if (longOpt->valueRequired)
            {
                
                if (internal->xargc + 1 < internal->argc)
                {
                    opt->value = internal->argv[++(internal->xargc)];
                }
                
                else
                {
                    break; 
                }
            }
            result = PL_OPT_OK;
            break;
        }
        internal->xargv = &static_Nul; 
        return result;
    }
    if (internal->minus)
    {
        PRIntn cop;
        PRIntn eoo = internal->optionsLen;
        for (cop = 0; cop < eoo; ++cop)
        {
            if (internal->options[cop] == *internal->xargv)
            {
                opt->option = *internal->xargv++;
                opt->longOption = opt->option & 0xff;
                




                if (':' == internal->options[cop + 1])
                {
                    
                    if (0 != *internal->xargv)
                    {
                        opt->value = internal->xargv;
                    }
                    
                    else if (internal->xargc + 1 < internal->argc)
                    {
                        opt->value = internal->argv[++(internal->xargc)];
                    }
                    
                    else
                    {
                        return PL_OPT_BAD;
                    }

                    internal->xargv = &static_Nul;
                    internal->minus = 0;
                }
                else 
                    opt->value = NULL; 
                return PL_OPT_OK;
            }
        }
        internal->xargv += 1;  
        return PL_OPT_BAD;
    }

    


    opt->value = internal->argv[internal->xargc];
    internal->xargv = &static_Nul;
    opt->option = 0;
    return PL_OPT_OK;
}  


