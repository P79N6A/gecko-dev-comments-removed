





































#include "nsIServiceManager.h"
#include "nsDateTimeFormatMac.h"
#include <Resources.h>
#include <IntlResources.h>
#include <DateTimeUtils.h>
#include <Script.h>
#include <TextUtils.h>
#include "nsIComponentManager.h"
#include "nsLocaleCID.h"
#include "nsILocaleService.h"
#include "nsIPlatformCharset.h"
#include "nsIMacLocale.h"
#include "nsCRT.h"
#include "plstr.h"
#include "prmem.h"
#include "nsUnicharUtils.h"



static Intl1Hndl GetItl1Resource(short scriptcode, short regioncode)
{
	long itl1num;

	if (smRoman == scriptcode)
	{
		itl1num = regioncode;	
	} else {
		
		ItlbRecord **ItlbRecordHandle;
		ItlbRecordHandle = (ItlbRecord **)::GetResource('itlb', scriptcode);
		
		
		
		
		if(ItlbRecordHandle != NULL)
		{
			if(*ItlbRecordHandle == NULL)
				::LoadResource((Handle)ItlbRecordHandle);
				
			if(*ItlbRecordHandle != NULL)
				itl1num = (*ItlbRecordHandle)->itlbDate;
			else
				itl1num = ::GetScriptVariable(scriptcode, smScriptDate);
		} else {	
			itl1num = ::GetScriptVariable(scriptcode, smScriptDate);
		}
	}
	
	
	Intl1Hndl Itl1RecordHandle;
	Itl1RecordHandle = (Intl1Hndl)::GetResource('itl1', itl1num);
	NS_ASSERTION(Itl1RecordHandle, "failed to get itl1 handle");
	return Itl1RecordHandle;
}

static Intl0Hndl GetItl0Resource(short scriptcode, short regioncode)
{
	long itl0num;

	if (smRoman == scriptcode)
	{
		itl0num = regioncode;	
	} else {
		
		ItlbRecord **ItlbRecordHandle;
		ItlbRecordHandle = (ItlbRecord **)::GetResource('itlb', scriptcode);
		
		
		
		
		if(ItlbRecordHandle != NULL)
		{
			if(*ItlbRecordHandle == NULL)
				::LoadResource((Handle)ItlbRecordHandle);
				
			if(*ItlbRecordHandle != NULL)
				itl0num = (*ItlbRecordHandle)->itlbNumber;
			else
				itl0num = ::GetScriptVariable(scriptcode, smScriptNumber);
		} else {	
			itl0num = ::GetScriptVariable(scriptcode, smScriptNumber);
		}
	}
	
	
	Intl0Hndl Itl0RecordHandle;
	Itl0RecordHandle = (Intl0Hndl)::GetResource('itl0', itl0num);
	NS_ASSERTION(Itl0RecordHandle, "failed to get itl0 handle");
	return Itl0RecordHandle;
}

static void AbbrevWeekdayString(DateTimeRec &dateTime, Str255 weekdayString, Intl1Hndl Itl1RecordHandle )
{
	Boolean gotit = false;
	
	
	if(Itl1RecordHandle != NULL )
	{
		if(*Itl1RecordHandle == NULL)
			::LoadResource((Handle)Itl1RecordHandle);

		if(*Itl1RecordHandle == NULL)
		{
			weekdayString[0] = 0;
			return;
		}
		
		
		
		if((unsigned short)((*Itl1RecordHandle)->localRtn[0]) == 0xA89F)
		{	
			Itl1ExtRec **Itl1ExtRecHandle;
			Itl1ExtRecHandle = (Itl1ExtRec **) Itl1RecordHandle;
			
			
			if(((*Itl1ExtRecHandle)->abbrevDaysTableLength != 0) &&
			   ((*Itl1ExtRecHandle)->abbrevDaysTableOffset != 0))
			{	
				
				
				
				
				
				Ptr abTablePt;
				short itemlen;
				
				
				short weekday = dateTime.dayOfWeek - 1;	
				
				abTablePt = (Ptr)(*Itl1ExtRecHandle);
				abTablePt +=  (*Itl1ExtRecHandle)->abbrevDaysTableOffset;
				
				
				itemlen = (short) *((short*)abTablePt);	
				abTablePt += 2;	
				
				if(weekday < itemlen)
				{
					unsigned char len;
					short i;
					
					for(i = 0 ; i < weekday ; i++)	
					{
						len = *abTablePt;
						
						abTablePt += len + 1;	
					}
					
					len = *abTablePt;
					::BlockMoveData(abTablePt,&weekdayString[0] ,len+1);
					gotit = true;
				}
			}
		}
		
		
		
		
		if(! gotit)
		{	
			
			
			
			short abbrLen = (*Itl1RecordHandle)->abbrLen;
			
			if(((((*Itl1RecordHandle)->intl1Vers) >> 8) == verTaiwan ) &&
			   (abbrLen == 4) &&
			   ((*Itl1RecordHandle)->days[0][0] == 6) && 
			   ((*Itl1RecordHandle)->days[1][0] == 6) && 
			   ((*Itl1RecordHandle)->days[2][0] == 6) && 
			   ((*Itl1RecordHandle)->days[3][0] == 6) && 
			   ((*Itl1RecordHandle)->days[4][0] == 6) && 
			   ((*Itl1RecordHandle)->days[5][0] == 6) && 
			   ((*Itl1RecordHandle)->days[6][0] == 6))
			{
				abbrLen = 6;
			}
			weekdayString[0] = abbrLen;
			
			::BlockMoveData(&((*Itl1RecordHandle)->days[dateTime.dayOfWeek-1][1]),
				 &weekdayString[1] , abbrLen);
			gotit = true;
		}
	}
	else 
	{	
		weekdayString[0] = 0;
	}
}





NS_IMPL_THREADSAFE_ISUPPORTS1(nsDateTimeFormatMac, nsIDateTimeFormat)

nsresult nsDateTimeFormatMac::Initialize(nsILocale* locale)
{
  nsAutoString localeStr;
  nsAutoString category(NS_LITERAL_STRING("NSILOCALE_TIME"));
  nsresult res;

  
  if (nsnull == locale) {
    if (!mLocale.IsEmpty() &&
        mLocale.Equals(mAppLocale, nsCaseInsensitiveStringComparator())) {
      return NS_OK;
    }
  }
  else {
    res = locale->GetCategory(category, localeStr);
    if (NS_SUCCEEDED(res) && !localeStr.IsEmpty()) {
      if (!mLocale.IsEmpty() &&
          mLocale.Equals(localeStr,
                         nsCaseInsensitiveStringComparator())) {
        return NS_OK;
      }
    }
  }

  mScriptcode = smSystemScript;
  mLangcode = langEnglish;
  mRegioncode = verUS;
  mCharset.AssignLiteral("x-mac-roman");
  

  
  nsCOMPtr<nsILocaleService> localeService = 
           do_GetService(NS_LOCALESERVICE_CONTRACTID, &res);
  if (NS_SUCCEEDED(res)) {
    nsCOMPtr<nsILocale> appLocale;
    res = localeService->GetApplicationLocale(getter_AddRefs(appLocale));
    if (NS_SUCCEEDED(res)) {
      res = appLocale->GetCategory(category, localeStr);
      if (NS_SUCCEEDED(res) && !localeStr.IsEmpty()) {
        mAppLocale = localeStr; 
      }
    }
  }
  
  
  if (nsnull == locale) {
    mUseDefaultLocale = true;
  }
  else {
    mUseDefaultLocale = false;
    res = locale->GetCategory(category, localeStr);
  }
    
  
  if (NS_SUCCEEDED(res) && !localeStr.IsEmpty()) {
    mLocale.Assign(localeStr); 

    nsCOMPtr <nsIMacLocale> macLocale = do_GetService(NS_MACLOCALE_CONTRACTID, &res);
    if (NS_SUCCEEDED(res)) {
      res = macLocale->GetPlatformLocale(mLocale, &mScriptcode, &mLangcode, &mRegioncode);
    }

    nsCOMPtr <nsIPlatformCharset> platformCharset = do_GetService(NS_PLATFORMCHARSET_CONTRACTID, &res);
    if (NS_SUCCEEDED(res)) {
      nsCAutoString  mappedCharset;
      res = platformCharset->GetDefaultCharsetForLocale(mLocale, mappedCharset);
      if (NS_SUCCEEDED(res)) {
        mCharset = mappedCharset;
      }
    }
  }

  
  nsCOMPtr <nsIAtom>                      charsetAtom;
  nsCOMPtr <nsICharsetConverterManager>  charsetConverterManager;
  charsetConverterManager = do_GetService(NS_CHARSETCONVERTERMANAGER_CONTRACTID, &res);
  if (NS_SUCCEEDED(res)) {
    res = charsetConverterManager->GetUnicodeDecoder(mCharset.get(),
                                                     getter_AddRefs(mDecoder));
  }
  
  return res;
}


nsresult nsDateTimeFormatMac::FormatTime(nsILocale* locale, 
                                      const nsDateFormatSelector  dateFormatSelector, 
                                      const nsTimeFormatSelector timeFormatSelector, 
                                      const time_t  timetTime, 
                                      nsAString& stringOut)
{
  return FormatTMTime(locale, dateFormatSelector, timeFormatSelector, localtime(&timetTime), stringOut);
}

#if !TARGET_CARBON
static void CopyPascalStringToC(StringPtr aSrc, char* aDest)
{
  int len = aSrc[0];
  ::BlockMoveData(aSrc + 1, aDest, len);
  aDest[len] = '\0';
}
#endif


nsresult nsDateTimeFormatMac::FormatTMTime(nsILocale* locale, 
                                           const nsDateFormatSelector  dateFormatSelector, 
                                           const nsTimeFormatSelector timeFormatSelector, 
                                           const struct tm*  tmTime, 
                                           nsAString& stringOut)
{
  DateTimeRec macDateTime;
  Str255 timeString, dateString;
  int32 dateTime;	
  nsresult res;

  
  (void) Initialize(locale);
  
  
  if (dateFormatSelector == kDateFormatNone && timeFormatSelector == kTimeFormatNone) {
    stringOut.Truncate();
    return NS_OK;
  }

  
  CopyASCIItoUTF16(nsDependentCString(asctime(tmTime)), stringOut);
  
  
  NS_ASSERTION(tmTime->tm_mon >= 0, "tm is not set correctly");
  NS_ASSERTION(tmTime->tm_mday >= 1, "tm is not set correctly");
  NS_ASSERTION(tmTime->tm_hour >= 0, "tm is not set correctly");
  NS_ASSERTION(tmTime->tm_min >= 0, "tm is not set correctly");
  NS_ASSERTION(tmTime->tm_sec >= 0, "tm is not set correctly");
  NS_ASSERTION(tmTime->tm_wday >= 0, "tm is not set correctly");

  macDateTime.year = tmTime->tm_year + 1900;	

  
  
  macDateTime.month = tmTime->tm_mon + 1;	
  macDateTime.day = tmTime->tm_mday;
  macDateTime.hour = tmTime->tm_hour;
  macDateTime.minute = tmTime->tm_min;
  macDateTime.second = tmTime->tm_sec;

  
  
  macDateTime.dayOfWeek = tmTime->tm_wday +1 ; 

  ::DateToSeconds( &macDateTime, (unsigned long *) &dateTime);
  
  
  Handle itl1Handle = mUseDefaultLocale ? nil : (Handle) GetItl1Resource(mScriptcode, mRegioncode);
  Handle itl0Handle = mUseDefaultLocale ? nil : (Handle) GetItl0Resource(mScriptcode, mRegioncode);

  
  if (timeFormatSelector != kTimeFormatNone) {
    
    if ( itl0Handle &&
       (timeFormatSelector == kTimeFormatSecondsForce24Hour || 
        timeFormatSelector == kTimeFormatNoSecondsForce24Hour)) {
      Intl0Hndl itl0HandleToModify = (Intl0Hndl) itl0Handle;
      UInt8 timeCycle = (**itl0HandleToModify).timeCycle;
      (**itl0HandleToModify).timeCycle = timeCycle24;
      ::TimeString(dateTime, (timeFormatSelector == kTimeFormatSeconds), timeString, itl0Handle);
      (**itl0HandleToModify).timeCycle = timeCycle;
    }
    else {
      ::TimeString(dateTime, (timeFormatSelector == kTimeFormatSeconds), timeString, itl0Handle);
    }
  }
  
  
  switch (dateFormatSelector) {
    case kDateFormatLong:
      ::DateString(dateTime, abbrevDate, dateString, itl1Handle);
      break;
    case kDateFormatShort:
      ::DateString(dateTime, shortDate, dateString, itl0Handle);
      break;
    case kDateFormatYearMonth:
      dateString[0] =  strftime((char*)&dateString[1],254,"%y/%m",tmTime);
      break;
    case kDateFormatWeekday:
      AbbrevWeekdayString(macDateTime, dateString, (Intl1Hndl)itl1Handle);
      
      if(dateString[0] == 0) {	
        dateString[0] =  strftime((char*)&dateString[1],254,"%a",tmTime);
      }
      break;
  }
  
  
  char localBuffer[2 * 255 + 2 + 1]; 
                                     
  if (dateFormatSelector != kDateFormatNone && timeFormatSelector != kTimeFormatNone) {
    CopyPascalStringToC(dateString, localBuffer);
    localBuffer[dateString[0]] = ' ';
    CopyPascalStringToC(timeString, &(localBuffer[dateString[0] + 1]));
  }
  else if (dateFormatSelector != kDateFormatNone) {
    CopyPascalStringToC(dateString, localBuffer);
  }
  else if (timeFormatSelector != kTimeFormatNone) {
    CopyPascalStringToC(timeString, localBuffer);
  }

  
  if (mDecoder) {
    PRInt32 srcLength = (PRInt32) PL_strlen(localBuffer);
    PRInt32 unicharLength = sizeof(Str255)*2;
    PRUnichar unichars[sizeof(Str255)*2];   

    res = mDecoder->Convert(localBuffer, &srcLength, unichars, &unicharLength);
    if (NS_SUCCEEDED(res)) {
      stringOut.Assign(unichars, unicharLength);
    }
  }
  
  return res;
}


nsresult nsDateTimeFormatMac::FormatPRTime(nsILocale* locale, 
                                           const nsDateFormatSelector  dateFormatSelector, 
                                           const nsTimeFormatSelector timeFormatSelector, 
                                           const PRTime  prTime, 
                                           nsAString& stringOut)
{
  PRExplodedTime explodedTime;
  PR_ExplodeTime(prTime, PR_LocalTimeParameters, &explodedTime);

  return FormatPRExplodedTime(locale, dateFormatSelector, timeFormatSelector, &explodedTime, stringOut);
}


nsresult nsDateTimeFormatMac::FormatPRExplodedTime(nsILocale* locale, 
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

