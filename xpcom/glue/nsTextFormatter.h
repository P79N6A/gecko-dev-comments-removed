










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

    




    static uint32_t snprintf(PRUnichar *out, uint32_t outlen, const PRUnichar *fmt, ...);

    



    static PRUnichar* smprintf(const PRUnichar *fmt, ...);

    static uint32_t ssprintf(nsAString& out, const PRUnichar* fmt, ...);

    


    static uint32_t vsnprintf(PRUnichar *out, uint32_t outlen, const PRUnichar *fmt, va_list ap);
    static PRUnichar* vsmprintf(const PRUnichar *fmt, va_list ap);
    static uint32_t vssprintf(nsAString& out, const PRUnichar *fmt, va_list ap);

    




    static void smprintf_free(PRUnichar *mem);

};

#endif 
