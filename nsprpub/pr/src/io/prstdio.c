




































#include "primpl.h"

#include <string.h>




PR_IMPLEMENT(PRUint32) PR_fprintf(PRFileDesc* fd, const char *fmt, ...)
{
    va_list ap;
    PRUint32 rv;

    va_start(ap, fmt);
    rv = PR_vfprintf(fd, fmt, ap);
    va_end(ap);
    return rv;
}

PR_IMPLEMENT(PRUint32) PR_vfprintf(PRFileDesc* fd, const char *fmt, va_list ap)
{
    
    PRUint32 rv, len;
    char* msg = PR_vsmprintf(fmt, ap);
    if (NULL == msg) {
        return -1;
    }
    len = strlen(msg);
#ifdef XP_OS2
    




    if (isatty(PR_FileDesc2NativeHandle(fd))) {
        PRUint32 last = 0, idx;
        PRInt32 tmp;
        rv = 0;
        for (idx = 0; idx < len+1; idx++) {
            if ((idx - last > 0) && (('\n' == msg[idx]) || (idx == len))) {
                tmp = PR_Write(fd, msg + last, idx - last);
                if (tmp >= 0) {
                    rv += tmp;
                }
                last = idx;
            }
            




            if (('\n' == msg[idx]) &&
                ((0 == idx) || ('\r' != msg[idx-1])) &&
                ('\r' != msg[idx+1])) {
                
                tmp = PR_Write(fd, "\r", 1);
                if (tmp >= 0) {
                    rv += tmp;
                }
            }
        }
    } else {
        rv = PR_Write(fd, msg, len);
    }
#else
    rv = PR_Write(fd, msg, len);
#endif
    PR_DELETE(msg);
    return rv;
}
