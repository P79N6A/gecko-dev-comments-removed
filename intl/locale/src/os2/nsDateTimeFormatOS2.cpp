





































#include <unidef.h>
#include "nsDateTimeFormatOS2.h"

NS_IMPL_THREADSAFE_ISUPPORTS1(nsDateTimeFormatOS2,nsIDateTimeFormat)

#define NSDATETIME_FORMAT_BUFFER_LEN  80

#ifndef LOCI_iTime
#define LOCI_iTime   ((LocaleItem)73)
#endif

nsresult nsDateTimeFormatOS2::FormatTime(nsILocale* locale, 
                               const nsDateFormatSelector  dateFormatSelector, 
                               const nsTimeFormatSelector  timeFormatSelector, 
                               const time_t                timetTime,
                               nsAString                   &stringOut)
{
  return FormatTMTime(locale, dateFormatSelector, timeFormatSelector, localtime( &timetTime ), stringOut);
}


nsresult nsDateTimeFormatOS2::FormatTMTime(nsILocale* locale, 
                               const nsDateFormatSelector  dateFormatSelector, 
                               const nsTimeFormatSelector  timeFormatSelector, 
                               const struct tm*            tmTime, 
                               nsAString                   &stringOut)
{

  nsresult rc = NS_ERROR_FAILURE;
  UniChar uFmtD[NSDATETIME_FORMAT_BUFFER_LEN] = { 0 };
  UniChar uFmtT[NSDATETIME_FORMAT_BUFFER_LEN] = { 0 };
  UniChar *pString = NULL;
  LocaleObject locObj = NULL;
  int ret = UniCreateLocaleObject(UNI_UCS_STRING_POINTER, (UniChar *)L"", &locObj);
  if (ret != ULS_SUCCESS)
    UniCreateLocaleObject(UNI_UCS_STRING_POINTER, (UniChar *)L"C", &locObj);

  PRBool f24Hour = PR_FALSE;

  UniQueryLocaleItem(locObj, LOCI_iTime, &pString);

  if (pString[0] == '1') {
    f24Hour = PR_TRUE;
  }

  
  switch (dateFormatSelector) {
    case kDateFormatNone:
      UniStrcat( uFmtD, (UniChar*)L"");
      break; 
    case kDateFormatLong:
    case kDateFormatShort:
      UniStrcat( uFmtD, (UniChar*)L"%x");
      break; 
    case kDateFormatYearMonth:
      UniQueryLocaleItem( locObj, DATESEP, &pString);
      UniStrcat( uFmtD, (UniChar*)L"%y");
      UniStrcat( uFmtD, pString);
      UniStrcat( uFmtD, (UniChar*)L"%m");
      UniFreeMem(pString);
      break; 
    case kDateFormatWeekday:
      UniStrcat( uFmtD, (UniChar*)L"%a");
      break;
    default: 
      UniStrcat( uFmtD, (UniChar*)L"");
  }

  
  switch (timeFormatSelector) {
    case kTimeFormatNone: 
      UniStrcat( uFmtT, (UniChar*)L"");
      break;
   case kTimeFormatSeconds:
      UniQueryLocaleItem( locObj, TIMESEP, &pString);
      if (f24Hour)
        UniStrcat( uFmtT, (UniChar*)L"%H");
      else
        UniStrcat( uFmtT, (UniChar*)L"%I");
      UniStrcat( uFmtT, pString);
      UniStrcat( uFmtT, (UniChar*)L"%M");
      UniStrcat( uFmtT, pString);
      UniStrcat( uFmtT, (UniChar*)L"%S");
      if (!f24Hour)
        UniStrcat( uFmtT, (UniChar*)L" %p");
      UniFreeMem(pString);
      break;
    case kTimeFormatNoSeconds:
      UniQueryLocaleItem( locObj, TIMESEP, &pString);
      if (f24Hour)
        UniStrcat( uFmtT, (UniChar*)L"%H");
      else
        UniStrcat( uFmtT, (UniChar*)L"%I");
      UniStrcat( uFmtT, pString);
      UniStrcat( uFmtT, (UniChar*)L"%M");
      if (!f24Hour)
        UniStrcat( uFmtT, (UniChar*)L" %p");
      UniFreeMem(pString);
      break;
    case kTimeFormatSecondsForce24Hour:
      UniQueryLocaleItem( locObj, TIMESEP, &pString);
      UniStrcat( uFmtT, (UniChar*)L"%H");
      UniStrcat( uFmtT, pString);
      UniStrcat( uFmtT, (UniChar*)L"%M");
      UniStrcat( uFmtT, pString);
      UniStrcat( uFmtT, (UniChar*)L"%S");
      UniFreeMem(pString);
      break;
    case kTimeFormatNoSecondsForce24Hour:
      UniQueryLocaleItem( locObj, TIMESEP, &pString);
      UniStrcat( uFmtT, (UniChar*)L"%H");
      UniStrcat( uFmtT, pString);
      UniStrcat( uFmtT, (UniChar*)L"%M");
      UniFreeMem(pString);
      break;  
    default: 
      UniStrcat( uFmtT, (UniChar*)L"");
  }

  PRUnichar buffer[NSDATETIME_FORMAT_BUFFER_LEN] = {0};
  if ((dateFormatSelector != kDateFormatNone) && (timeFormatSelector != kTimeFormatNone)) {
    UniStrcat( uFmtD, (UniChar*)L" ");
  }
  UniStrcat( uFmtD, uFmtT);
  int length = UniStrftime(locObj, NS_REINTERPRET_CAST(UniChar *, buffer),
                           NSDATETIME_FORMAT_BUFFER_LEN, uFmtD, tmTime);
  UniFreeLocaleObject(locObj);

  if ( length != 0) {
    stringOut.Assign(buffer, length);
    rc = NS_OK;
  }
  
  return rc;
}


nsresult nsDateTimeFormatOS2::FormatPRTime(nsILocale* locale, 
                                           const nsDateFormatSelector  dateFormatSelector, 
                                           const nsTimeFormatSelector timeFormatSelector, 
                                           const PRTime  prTime, 
                                           nsAString& stringOut)
{
  PRExplodedTime explodedTime;
  PR_ExplodeTime(prTime, PR_LocalTimeParameters, &explodedTime);

  return FormatPRExplodedTime(locale, dateFormatSelector, timeFormatSelector, &explodedTime, stringOut);
}


nsresult nsDateTimeFormatOS2::FormatPRExplodedTime(nsILocale* locale, 
                                                   const nsDateFormatSelector  dateFormatSelector, 
                                                   const nsTimeFormatSelector timeFormatSelector, 
                                                   const PRExplodedTime*  explodedTime, 
                                                   nsAString& stringOut)
{
  struct tm  tmTime;
  










  memset( &tmTime, 0, sizeof(tmTime) );

  tmTime.tm_yday = explodedTime->tm_yday;
  tmTime.tm_wday = explodedTime->tm_wday;
  tmTime.tm_year = explodedTime->tm_year;
  tmTime.tm_year -= 1900;
  tmTime.tm_mon = explodedTime->tm_month;
  tmTime.tm_mday = explodedTime->tm_mday;
  tmTime.tm_hour = explodedTime->tm_hour;
  tmTime.tm_min = explodedTime->tm_min;
  tmTime.tm_sec = explodedTime->tm_sec;

  return FormatTMTime(locale, dateFormatSelector, timeFormatSelector, &tmTime, stringOut);
}

