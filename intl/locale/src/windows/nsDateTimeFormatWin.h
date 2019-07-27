





#ifndef nsDateTimeFormatWin_h__
#define nsDateTimeFormatWin_h__


#include "nsIDateTimeFormat.h"
#include <windows.h>




class nsDateTimeFormatWin : public nsIDateTimeFormat {
  virtual ~nsDateTimeFormatWin() {}

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

  nsDateTimeFormatWin() {mLocale.SetLength(0);mAppLocale.SetLength(0);}


private:
  
  NS_IMETHOD Initialize(nsILocale* locale);

  
  int nsGetTimeFormatW(DWORD dwFlags, const SYSTEMTIME *lpTime,
                    const char* format, char16_t *timeStr, int cchTime);

  
  int nsGetDateFormatW(DWORD dwFlags, const SYSTEMTIME *lpDate,
                       const char* format, char16_t *dateStr, int cchDate);

  nsString    mLocale;
  nsString    mAppLocale;
  uint32_t    mLCID;             
};

#endif  
