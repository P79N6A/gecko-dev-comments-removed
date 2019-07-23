





































#ifndef nsIDateTimeFormat_h__
#define nsIDateTimeFormat_h__


#include "nsISupports.h"
#include "nscore.h"
#include "nsStringGlue.h"
#include "nsILocale.h"
#include "nsIScriptableDateFormat.h"
#include "prtime.h"
#include <time.h>



#define NS_IDATETIMEFORMAT_IID \
{ 0x2bbaa0b0, 0xa591, 0x11d2, \
{ 0x91, 0x19, 0x0, 0x60, 0x8, 0xa6, 0xed, 0xf6 } }




class nsIDateTimeFormat : public nsISupports {

public: 
  NS_DECLARE_STATIC_IID_ACCESSOR(NS_IDATETIMEFORMAT_IID)

  
  NS_IMETHOD FormatTime(nsILocale* locale, 
                        const nsDateFormatSelector  dateFormatSelector, 
                        const nsTimeFormatSelector timeFormatSelector, 
                        const time_t  timetTime,
                        nsAString& stringOut) = 0; 

  
  NS_IMETHOD FormatTMTime(nsILocale* locale, 
                          const nsDateFormatSelector  dateFormatSelector, 
                          const nsTimeFormatSelector timeFormatSelector, 
                          const struct tm*  tmTime, 
                          nsAString& stringOut) = 0; 

  
  NS_IMETHOD FormatPRTime(nsILocale* locale, 
                          const nsDateFormatSelector  dateFormatSelector, 
                          const nsTimeFormatSelector timeFormatSelector, 
                          const PRTime  prTime, 
                          nsAString& stringOut) = 0;

  
  NS_IMETHOD FormatPRExplodedTime(nsILocale* locale, 
                                  const nsDateFormatSelector  dateFormatSelector, 
                                  const nsTimeFormatSelector timeFormatSelector, 
                                  const PRExplodedTime*  explodedTime, 
                                  nsAString& stringOut) = 0; 
};

NS_DEFINE_STATIC_IID_ACCESSOR(nsIDateTimeFormat, NS_IDATETIMEFORMAT_IID)

#endif  
