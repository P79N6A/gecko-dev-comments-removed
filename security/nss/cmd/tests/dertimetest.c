



































#include <stdio.h>
#include <stdlib.h>

#include "secder.h"
#include "secerr.h"

int main()
{
    SECItem badTime;
    PRTime prtime;
    SECStatus rv;
    int error;
    PRBool failed = PR_FALSE;

    
    badTime.type = siBuffer;
    badTime.data = (unsigned char *)"091219000000Z\0junkjunkjunkjunkjunkjunk";
    badTime.len = 38;
    rv = DER_UTCTimeToTime(&prtime, &badTime);
    if (rv == SECSuccess) {
        fprintf(stderr, "DER_UTCTimeToTime should have failed but "
                "succeeded\n");
        failed = PR_TRUE;
    } else {
        error = PORT_GetError();
        if (error != SEC_ERROR_INVALID_TIME) {
            fprintf(stderr, "DER_UTCTimeToTime failed with error %d, "
                    "expected error %d\n", error, SEC_ERROR_INVALID_TIME);
            failed = PR_TRUE;
        }
    }

    
    badTime.type = siBuffer;
    badTime.data = (unsigned char *)"091219000000Zjunk";
    badTime.len = 17;
    rv = DER_UTCTimeToTime(&prtime, &badTime);
    if (rv == SECSuccess) {
        fprintf(stderr, "DER_UTCTimeToTime should have failed but "
                "succeeded\n");
        failed = PR_TRUE;
    } else {
        error = PORT_GetError();
        if (error != SEC_ERROR_INVALID_TIME) {
            fprintf(stderr, "DER_UTCTimeToTime failed with error %d, "
                    "expected error %d\n", error, SEC_ERROR_INVALID_TIME);
            failed = PR_TRUE;
        }
    }

    
    badTime.type = siBuffer;
    badTime.data = (unsigned char *)"20091219000000Z\0junkjunkjunkjunkjunkjunk";
    badTime.len = 40;
    rv = DER_GeneralizedTimeToTime(&prtime, &badTime);
    if (rv == SECSuccess) {
        fprintf(stderr, "DER_GeneralizedTimeToTime should have failed but "
                "succeeded\n");
        failed = PR_TRUE;
    } else {
        error = PORT_GetError();
        if (error != SEC_ERROR_INVALID_TIME) {
            fprintf(stderr, "DER_GeneralizedTimeToTime failed with error %d, "
                    "expected error %d\n", error, SEC_ERROR_INVALID_TIME);
            failed = PR_TRUE;
        }
    }

    
    badTime.type = siBuffer;
    badTime.data = (unsigned char *)"20091219000000Zjunk";
    badTime.len = 19;
    rv = DER_GeneralizedTimeToTime(&prtime, &badTime);
    if (rv == SECSuccess) {
        fprintf(stderr, "DER_GeneralizedTimeToTime should have failed but "
                "succeeded\n");
        failed = PR_TRUE;
    } else {
        error = PORT_GetError();
        if (error != SEC_ERROR_INVALID_TIME) {
            fprintf(stderr, "DER_GeneralizedTimeToTime failed with error %d, "
                    "expected error %d\n", error, SEC_ERROR_INVALID_TIME);
            failed = PR_TRUE;
        }
    }

    if (failed) {
        fprintf(stderr, "FAIL\n");
        return 1;
    }
    printf("PASS\n");
    return 0;
}
