























































#include "plgetopt.h"

#include "prlong.h"
#include "prinit.h"
#include "prprf.h"

#include <string.h>

#define BUF_SIZE 128

PRIntn failed_already=0;
PRIntn debug_mode;

int main(int argc, char **argv)
{
    PRInt16 i16;
    PRIntn n;
    PRInt32 i32;
    PRInt64 i64;
    char buf[BUF_SIZE];

	





	PLOptStatus os;
	PLOptState *opt = PL_CreateOptState(argc, argv, "d:");
	while (PL_OPT_EOL != (os = PL_GetNextOpt(opt)))
    {
		if (PL_OPT_BAD == os) continue;
        switch (opt->option)
        {
        case 'd':  
			debug_mode = 1;
            break;
         default:
            break;
        }
    }
	PL_DestroyOptState(opt);

	
	

    PR_STDIO_INIT();
    i16 = -32;
    n = 30;
    i32 = 64;
    LL_I2L(i64, 333);
    PR_snprintf(buf, BUF_SIZE, "%d %hd %lld %ld", n, i16, i64, i32);
    if (!strcmp(buf, "30 -32 333 64")) {
		if (debug_mode) printf("PR_snprintf test 2 passed\n");
    } else {
		if (debug_mode) {
			printf("PR_snprintf test 2 failed\n");
			printf("Converted string is %s\n", buf);
			printf("Should be 30 -32 333 64\n");
		}
		else failed_already=1;
    }
	if(failed_already)
	{
        printf("FAILED\n");
		return 1;
	}
	else
	{
        printf("PASSED\n");
		return 0;
	}
}
