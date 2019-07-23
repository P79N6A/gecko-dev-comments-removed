











































#ifndef nsTextFormatter_h___
#define nsTextFormatter_h___


















#include "prtypes.h"
#include "prio.h"
#include <stdio.h>
#include <stdarg.h>
#include "nscore.h"
#include "nsStringGlue.h"

#ifdef XPCOM_GLUE
#error "nsTextFormatter is not available in the standalone glue due to NSPR dependencies."
#endif

class NS_COM_GLUE nsTextFormatter {

  public:

    




    static PRUint32 snprintf(PRUnichar *out, PRUint32 outlen, const PRUnichar *fmt, ...);

    



    static PRUnichar* smprintf(const PRUnichar *fmt, ...);

    static PRUint32 ssprintf(nsAString& out, const PRUnichar* fmt, ...);

    


    static PRUint32 vsnprintf(PRUnichar *out, PRUint32 outlen, const PRUnichar *fmt, va_list ap);
    static PRUnichar* vsmprintf(const PRUnichar *fmt, va_list ap);
    static PRUint32 vssprintf(nsAString& out, const PRUnichar *fmt, va_list ap);

    




    static void smprintf_free(PRUnichar *mem);

};

#endif 
