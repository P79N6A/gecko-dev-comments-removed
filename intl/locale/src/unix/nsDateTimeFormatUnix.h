





#ifndef nsDateTimeFormatUnix_h__
#define nsDateTimeFormatUnix_h__


#include "nsCOMPtr.h"
#include "nsIDateTimeFormat.h"
#include "nsIUnicodeDecoder.h"

#define kPlatformLocaleLength 64

class nsDateTimeFormatUnix : public nsIDateTimeFormat {

public: 
  NS_DECL_THREADSAFE_ISUPPORTS 

  
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


  nsDateTimeFormatUnix() {mLocale.Truncate();mAppLocale.Truncate();}

  virtual ~nsDateTimeFormatUnix() {}

private:
  
  NS_IMETHOD Initialize(nsILocale* locale);

  void LocalePreferred24hour();

  nsString    mLocale;
  nsString    mAppLocale;
  nsCString   mCharset;        
  nsCString   mPlatformLocale; 
  bool        mLocalePreferred24hour;                       
  bool        mLocaleAMPMfirst;                             
  nsCOMPtr <nsIUnicodeDecoder>   mDecoder;
};

#endif  
