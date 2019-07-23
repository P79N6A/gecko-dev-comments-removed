





































#ifndef nsDateTimeFormatMac_h__
#define nsDateTimeFormatMac_h__


#include "nsCOMPtr.h"
#include "nsIDateTimeFormat.h"


class nsDateTimeFormatMac : public nsIDateTimeFormat {

public: 
  NS_DECL_ISUPPORTS 

  
  NS_IMETHOD FormatTime(nsILocale* locale, 
                        const nsDateFormatSelector  dateFormatSelector, 
                        const nsTimeFormatSelector timeFormatSelector, 
                        const time_t  timetTime, 
                        nsAString& stringOut); 

  
  NS_IMETHOD FormatTMTime(nsILocale* locale, 
                          const nsDateFormatSelector  dateFormatSelector, 
                          const nsTimeFormatSelector timeFormatSelector, 
                          const struct tm*  tmTime, 
                          nsAString& stringOut); 
  
  NS_IMETHOD FormatPRTime(nsILocale* locale, 
                          const nsDateFormatSelector  dateFormatSelector, 
                          const nsTimeFormatSelector timeFormatSelector, 
                          const PRTime  prTime, 
                          nsAString& stringOut);

  
  NS_IMETHOD FormatPRExplodedTime(nsILocale* locale, 
                                  const nsDateFormatSelector  dateFormatSelector, 
                                  const nsTimeFormatSelector timeFormatSelector, 
                                  const PRExplodedTime*  explodedTime, 
                                  nsAString& stringOut); 

  nsDateTimeFormatMac() {}
  
  virtual ~nsDateTimeFormatMac() {}
  
private:
  
  NS_IMETHOD Initialize(nsILocale* locale);

  nsString    mLocale;
  nsString    mAppLocale;
  bool        mUseDefaultLocale;
};

#endif  
