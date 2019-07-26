




#include <locale.h>
#include "plstr.h"
#include "nsIServiceManager.h"
#include "nsDateTimeFormatUnix.h"
#include "nsIComponentManager.h"
#include "nsILocaleService.h"
#include "nsIPlatformCharset.h"
#include "nsPosixLocale.h"
#include "nsCRT.h"
#include "nsReadableUtils.h"
#include "nsUnicharUtils.h"
#include "mozilla/dom/EncodingUtils.h"

using mozilla::dom::EncodingUtils;

NS_IMPL_ISUPPORTS(nsDateTimeFormatUnix, nsIDateTimeFormat)


nsresult nsDateTimeFormatUnix::Initialize(nsILocale* locale)
{
  nsAutoString localeStr;
  NS_NAMED_LITERAL_STRING(aCategory, "NSILOCALE_TIME##PLATFORM");
  nsresult res = NS_OK;

  
  if (!locale) {
    if (!mLocale.IsEmpty() &&
        mLocale.Equals(mAppLocale, nsCaseInsensitiveStringComparator())) {
      return NS_OK;
    }
  }
  else {
    res = locale->GetCategory(aCategory, localeStr);
    if (NS_SUCCEEDED(res) && !localeStr.IsEmpty()) {
      if (!mLocale.IsEmpty() &&
          mLocale.Equals(localeStr,
                         nsCaseInsensitiveStringComparator())) {
        return NS_OK;
      }
    }
  }

  mCharset.AssignLiteral("windows-1252");
  mPlatformLocale.AssignLiteral("en_US");

  
  if (!locale) {
    nsCOMPtr<nsILocaleService> localeService = 
             do_GetService(NS_LOCALESERVICE_CONTRACTID, &res);
    if (NS_SUCCEEDED(res)) {
      nsCOMPtr<nsILocale> appLocale;
      res = localeService->GetApplicationLocale(getter_AddRefs(appLocale));
      if (NS_SUCCEEDED(res)) {
        res = appLocale->GetCategory(aCategory, localeStr);
        if (NS_SUCCEEDED(res) && !localeStr.IsEmpty()) {
          NS_ASSERTION(NS_SUCCEEDED(res), "failed to get app locale info");
          mAppLocale = localeStr; 
        }
      }
    }
  }
  else {
    res = locale->GetCategory(aCategory, localeStr);
    NS_ASSERTION(NS_SUCCEEDED(res), "failed to get locale info");
  }

  if (NS_SUCCEEDED(res) && !localeStr.IsEmpty()) {
    mLocale = localeStr; 

    nsPosixLocale::GetPlatformLocale(mLocale, mPlatformLocale);

    nsCOMPtr <nsIPlatformCharset> platformCharset = do_GetService(NS_PLATFORMCHARSET_CONTRACTID, &res);
    if (NS_SUCCEEDED(res)) {
      nsAutoCString mappedCharset;
      res = platformCharset->GetDefaultCharsetForLocale(mLocale, mappedCharset);
      if (NS_SUCCEEDED(res)) {
        mCharset = mappedCharset;
      }
    }
  }

  mDecoder = EncodingUtils::DecoderForEncoding(mCharset);

  LocalePreferred24hour();

  return res;
}

void nsDateTimeFormatUnix::LocalePreferred24hour()
{
  char str[100];
  time_t tt;
  struct tm *tmc;
  int i;

  tt = time(nullptr);
  tmc = localtime(&tt);

  tmc->tm_hour=22;    
  tmc->tm_min=0;      
  tmc->tm_sec=0;

  char *temp = setlocale(LC_TIME, mPlatformLocale.get());
  strftime(str, (size_t)99, "%X", (struct tm *)tmc);

  (void) setlocale(LC_TIME, temp);

  mLocalePreferred24hour = false;
  for (i=0; str[i]; i++) {
    if (str[i] == '2') {    
        mLocalePreferred24hour = true;
        break;
    }
  }

  mLocaleAMPMfirst = true;
  if (mLocalePreferred24hour == false) {
    if (str[0] && str[0] == '1') { 
			           
      mLocaleAMPMfirst = false;
    }
  }
}

nsresult nsDateTimeFormatUnix::FormatTime(nsILocale* locale, 
                                      const nsDateFormatSelector  dateFormatSelector, 
                                      const nsTimeFormatSelector timeFormatSelector, 
                                      const time_t  timetTime, 
                                      nsAString& stringOut) 
{
  struct tm tmTime;
  memcpy(&tmTime, localtime(&timetTime), sizeof(struct tm));
  return FormatTMTime(locale, dateFormatSelector, timeFormatSelector, &tmTime, stringOut);
}


nsresult nsDateTimeFormatUnix::FormatTMTime(nsILocale* locale, 
                                        const nsDateFormatSelector  dateFormatSelector, 
                                        const nsTimeFormatSelector timeFormatSelector, 
                                        const struct tm*  tmTime, 
                                        nsAString& stringOut) 
{
#define NSDATETIME_FORMAT_BUFFER_LEN  80
  char strOut[NSDATETIME_FORMAT_BUFFER_LEN*2];  
  char fmtD[NSDATETIME_FORMAT_BUFFER_LEN], fmtT[NSDATETIME_FORMAT_BUFFER_LEN];
  nsresult rv;

  
  
  (void) Initialize(locale);
  NS_ENSURE_TRUE(mDecoder, NS_ERROR_NOT_INITIALIZED);

  
  if (dateFormatSelector == kDateFormatLong && timeFormatSelector == kTimeFormatSeconds) {
    PL_strncpy(fmtD, "%c", NSDATETIME_FORMAT_BUFFER_LEN);
    PL_strncpy(fmtT, "", NSDATETIME_FORMAT_BUFFER_LEN); 
  } else {

    switch (dateFormatSelector) {
      case kDateFormatNone:
        PL_strncpy(fmtD, "", NSDATETIME_FORMAT_BUFFER_LEN);
        break; 
      case kDateFormatLong:
      case kDateFormatShort:
        PL_strncpy(fmtD, "%x", NSDATETIME_FORMAT_BUFFER_LEN);
        break; 
      case kDateFormatYearMonth:
        PL_strncpy(fmtD, "%Y/%m", NSDATETIME_FORMAT_BUFFER_LEN);
        break; 
      case kDateFormatWeekday:
        PL_strncpy(fmtD, "%a", NSDATETIME_FORMAT_BUFFER_LEN);
        break;
      default:
        PL_strncpy(fmtD, "", NSDATETIME_FORMAT_BUFFER_LEN); 
    }

    
    switch (timeFormatSelector) {
      case kTimeFormatNone:
        PL_strncpy(fmtT, "", NSDATETIME_FORMAT_BUFFER_LEN); 
        break;
      case kTimeFormatSeconds:
        PL_strncpy(fmtT, "%X", NSDATETIME_FORMAT_BUFFER_LEN);
        break;
      case kTimeFormatNoSeconds:
        PL_strncpy(fmtT, 
                   mLocalePreferred24hour ? "%H:%M" : mLocaleAMPMfirst ? "%p %I:%M" : "%I:%M %p", 
                   NSDATETIME_FORMAT_BUFFER_LEN);
        break;
      case kTimeFormatSecondsForce24Hour:
        PL_strncpy(fmtT, "%H:%M:%S", NSDATETIME_FORMAT_BUFFER_LEN);
        break;
      case kTimeFormatNoSecondsForce24Hour:
        PL_strncpy(fmtT, "%H:%M", NSDATETIME_FORMAT_BUFFER_LEN);
        break;
      default:
        PL_strncpy(fmtT, "", NSDATETIME_FORMAT_BUFFER_LEN); 
    }
  }

  
  char *old_locale = setlocale(LC_TIME, nullptr);
  (void) setlocale(LC_TIME, mPlatformLocale.get());
  if (strlen(fmtD) && strlen(fmtT)) {
    PL_strncat(fmtD, " ", NSDATETIME_FORMAT_BUFFER_LEN);
    PL_strncat(fmtD, fmtT, NSDATETIME_FORMAT_BUFFER_LEN);
    strftime(strOut, NSDATETIME_FORMAT_BUFFER_LEN, fmtD, tmTime);
  }
  else if (strlen(fmtD) && !strlen(fmtT)) {
    strftime(strOut, NSDATETIME_FORMAT_BUFFER_LEN, fmtD, tmTime);
  }
  else if (!strlen(fmtD) && strlen(fmtT)) {
    strftime(strOut, NSDATETIME_FORMAT_BUFFER_LEN, fmtT, tmTime);
  }
  else {
    PL_strncpy(strOut, "", NSDATETIME_FORMAT_BUFFER_LEN);
  }
  (void) setlocale(LC_TIME, old_locale);

  
  int32_t srcLength = (int32_t) strlen(strOut);
  int32_t unicharLength = NSDATETIME_FORMAT_BUFFER_LEN*2;
  char16_t unichars[NSDATETIME_FORMAT_BUFFER_LEN*2];   

  rv = mDecoder->Convert(strOut, &srcLength, unichars, &unicharLength);
  if (NS_FAILED(rv))
    return rv;
  stringOut.Assign(unichars, unicharLength);

  return rv;
}


nsresult nsDateTimeFormatUnix::FormatPRTime(nsILocale* locale, 
                                           const nsDateFormatSelector  dateFormatSelector, 
                                           const nsTimeFormatSelector timeFormatSelector, 
                                           const PRTime  prTime, 
                                           nsAString& stringOut)
{
  PRExplodedTime explodedTime;
  PR_ExplodeTime(prTime, PR_LocalTimeParameters, &explodedTime);

  return FormatPRExplodedTime(locale, dateFormatSelector, timeFormatSelector, &explodedTime, stringOut);
}


nsresult nsDateTimeFormatUnix::FormatPRExplodedTime(nsILocale* locale, 
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

