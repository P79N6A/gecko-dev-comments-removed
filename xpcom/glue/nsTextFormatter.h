










#ifndef nsTextFormatter_h___
#define nsTextFormatter_h___


















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

    




    static uint32_t snprintf(char16_t *out, uint32_t outlen, const char16_t *fmt, ...);

    



    static char16_t* smprintf(const char16_t *fmt, ...);

    static uint32_t ssprintf(nsAString& out, const char16_t* fmt, ...);

    


    static uint32_t vsnprintf(char16_t *out, uint32_t outlen, const char16_t *fmt, va_list ap);
    static char16_t* vsmprintf(const char16_t *fmt, va_list ap);
    static uint32_t vssprintf(nsAString& out, const char16_t *fmt, va_list ap);

    




    static void smprintf_free(char16_t *mem);

};

#endif 
