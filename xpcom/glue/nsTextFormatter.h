











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

class nsTextFormatter
{
public:

  




  static uint32_t snprintf(char16_t* aOut, uint32_t aOutLen,
                           const char16_t* aFmt, ...);

  



  static char16_t* smprintf(const char16_t* aFmt, ...);

  static uint32_t ssprintf(nsAString& aOut, const char16_t* aFmt, ...);

  


  static uint32_t vsnprintf(char16_t* aOut, uint32_t aOutLen, const char16_t* aFmt,
                            va_list aAp);
  static char16_t* vsmprintf(const char16_t* aFmt, va_list aAp);
  static uint32_t vssprintf(nsAString& aOut, const char16_t* aFmt, va_list aAp);

  




  static void smprintf_free(char16_t* aMem);

};

#endif 
