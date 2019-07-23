



































#include <stdio.h>
#include <stdlib.h>

#include "nss.h"
#include "secerr.h"








int main()
{
    SECStatus status;
    int error;

    status = NSS_InitReadWrite("sql:/no/such/db/dir");
    if (status == SECSuccess) {
        fprintf(stderr, "NSS_InitReadWrite succeeded unexpectedly\n");
        exit(1);
    }
    error = PORT_GetError();
    if (error != SEC_ERROR_BAD_DATABASE) {
        fprintf(stderr, "NSS_InitReadWrite failed with the wrong error code: "
                "%d\n", error);
        exit(1);
    }
    printf("PASS\n");
    return 0;
}
