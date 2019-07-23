





















































#include "plgetopt.h"
#include "prttools.h"

#include "prinit.h"
#include "prlong.h"
#include "prprf.h"

#include <string.h>

#define BUF_SIZE 128


















static void Test_Result (int result)
{
	if (result == PASS)
		printf ("PASS\n");
	else
		printf ("FAIL\n");
}

int main(int argc, char **argv)
{
    PRInt16 i16;
    PRIntn n;
    PRInt32 i32;
    PRInt64 i64;
    char buf[BUF_SIZE];
    char answer[BUF_SIZE];
    int i;

	





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
	
    i16 = -1;
    n = -1;
    i32 = -1;
    LL_I2L(i64, i32);

    PR_snprintf(buf, BUF_SIZE, "%hx %x %lx %llx", i16, n, i32, i64);
    strcpy(answer, "ffff ");
    for (i = PR_BYTES_PER_INT * 2; i; i--) {
	strcat(answer, "f");
    }
    strcat(answer, " ffffffff ffffffffffffffff");

    if (!strcmp(buf, answer)) {
	if (debug_mode) printf("PR_snprintf test 1 passed\n");
	else Test_Result (PASS);
    } else {
		if (debug_mode) {
			printf("PR_snprintf test 1 failed\n");
			printf("Converted string is %s\n", buf);
			printf("Should be %s\n", answer);
		}
		else
			Test_Result (FAIL);
    }

    return 0;
}
