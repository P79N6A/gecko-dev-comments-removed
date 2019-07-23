




































#include "prinit.h"
#include "prio.h"
#include "prprf.h"
#include "prdtoa.h"
#include "plgetopt.h"

#include <stdlib.h>

static void Help(void)
{
    PRFileDesc *err = PR_GetSpecialFD(PR_StandardError);
    PR_fprintf(err, "Usage: /.strod [-n n] [-l n] [-h]\n");
    PR_fprintf(err, "\t-n n Number to translate    (default: 1234567890123456789)\n");
    PR_fprintf(err, "\t-l n Times to loop the test (default: 1)\n");
    PR_fprintf(err, "\t-h   This message and nothing else\n");
}  

static PRIntn PR_CALLBACK RealMain(PRIntn argc, char **argv)
{
    PLOptStatus os;
    PRIntn loops = 1;
    PRFloat64 answer;
    const char *number = "1234567890123456789";
    PRFileDesc *err = PR_GetSpecialFD(PR_StandardError);
    PLOptState *opt = PL_CreateOptState(argc, argv, "hc:l:");

    while (PL_OPT_EOL != (os = PL_GetNextOpt(opt)))
    {
        if (PL_OPT_BAD == os) continue;
        switch (opt->option)
        {
        case 'n':  
            number = opt->value;
            break;
        case 'l':  
            loops = atoi(opt->value);
            break;
        case 'h':  
            Help();  
            return 2;  
            break;
         default:
            break;
        }
    }
    PL_DestroyOptState(opt);

    PR_fprintf(err, "Settings\n");
    PR_fprintf(err, "\tNumber to translate %s\n", number);
    PR_fprintf(err, "\tLoops to run test: %d\n", loops);

    while (loops--)
    {
        answer = PR_strtod(number, NULL);
        PR_fprintf(err, "Translation = %20.0f\n", answer);
    }
    return 0;
}



int main(int argc, char **argv)
{
    PRIntn rv;
    
    PR_STDIO_INIT();
    rv = PR_Initialize(RealMain, argc, argv, 0);
    return rv;
}  
