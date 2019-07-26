



#include "cpr_stdio.h"
#include "cpr_string.h"
#include "CSFLog.h"







#define LOG_MAX 1024




















int
buginf (const char *_format, ...)
{
    char fmt_buf[LOG_MAX + 1];
    va_list ap;
    int rc;

    va_start(ap, _format);
    rc = vsnprintf(fmt_buf, LOG_MAX, _format, ap);
    va_end(ap);
    if (rc <= 0) {
        return rc;
    }

  CSFLogDebug("cpr", "%s", fmt_buf);

  return rc;
}










int
buginf_msg (const char *str)
{
    char buf[LOG_MAX + 1];
    const char *p;
    int16_t len;

    
    buf[LOG_MAX] = NUL;

    len = (int16_t) strlen(str);

    if (len > LOG_MAX) {
        p = str;
        do {
            memcpy(buf, p, LOG_MAX);
            p += LOG_MAX;
            len -= LOG_MAX;

            printf("%s",buf);
        } while (len > LOG_MAX);

        if (len)
        {
          CSFLogDebug("cpr", "%s", (char *) p);
        }
    }
    else
    {
      CSFLogDebug("cpr", "%s", (char *) str);

    }

    return 0;
}











void
err_msg (const char *_format, ...)
{
    char fmt_buf[LOG_MAX + 1];
    va_list ap;
    int rc;

    va_start(ap, _format);
    rc = vsnprintf(fmt_buf, LOG_MAX, _format, ap);
    va_end(ap);
    if (rc <= 0) {
        return;
    }

  CSFLogError("cpr", "%s", fmt_buf);
}












void
notice_msg (const char *_format, ...)
{
    char fmt_buf[LOG_MAX + 1];
    va_list ap;
    int rc;

    va_start(ap, _format);
    rc = vsnprintf(fmt_buf, LOG_MAX, _format, ap);
    va_end(ap);
    if (rc <= 0) {
        return;
    }

  CSFLogInfo("cpr", "%s", fmt_buf);
}

