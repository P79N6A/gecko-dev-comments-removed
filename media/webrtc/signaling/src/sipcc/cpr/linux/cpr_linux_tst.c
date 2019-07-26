






































#include "cpr.h"
#include "cpr_stdlib.h"
#include "cpr_linux_tst.h"
#include "cpr_errno.h"
#include <stdarg.h>
#include "plat_api.h"

void
err_exit (void)
{
    *(volatile int *) 0xdeadbeef = 0x12345678;
}



int
main (int argc, char **argv, char **env)
{
    int ret;
    char *q;

    buginf("CPR test...\n");
    

    cprTestCmd(argc, argv);

    return 0;
}


long
strlib_mem_used (void)
{
    return (0);
}

#define LOG_MAX 255

int
debugif_printf (const char *_format, ...)
{
    char fmt_buf[LOG_MAX + 1];
    int rc;
    va_list ap;

    va_start(ap, _format);
    rc = vsnprintf(fmt_buf, LOG_MAX, _format, ap);
    va_end(ap);

    if (rc <= 0) {
        return rc;
    }
    printf("%s", fmt_buf);

    return rc;
}

