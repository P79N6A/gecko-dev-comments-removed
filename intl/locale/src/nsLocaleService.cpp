




































#include "nsCOMPtr.h"
#include "nsILocale.h"
#include "nsILocaleService.h"
#include "nsLocale.h"
#include "nsLocaleCID.h"
#include "nsServiceManagerUtils.h"
#include "nsReadableUtils.h"
#include "nsCRT.h"
#include "prprf.h"
#include "nsAutoBuffer.h"

#include <ctype.h>

#if defined(XP_WIN)
#  include "nsIWin32Locale.h"
#elif defined(XP_OS2)
#  include "unidef.h"
#  include "nsIOS2Locale.h"
#elif defined(XP_MACOSX)
#  include <Script.h>
#  include <Carbon/Carbon.h>
#  include "nsIMacLocale.h"
#elif defined(XP_UNIX) || defined(XP_BEOS)
#  include <locale.h>
#  include <stdlib.h>
#  include "nsIPosixLocale.h"
#endif



const int LocaleListLength = 6;
const char* LocaleList[LocaleListLength] = 
{
	NSILOCALE_COLLATE,
	NSILOCALE_CTYPE,
	NSILOCALE_MONETARY,
	NSILOCALE_NUMERIC,
	NSILOCALE_TIME,
	NSILOCALE_MESSAGE
};

#define NSILOCALE_MAX_ACCEPT_LANGUAGE	16
#define NSILOCALE_MAX_ACCEPT_LENGTH		18

#if (defined(XP_UNIX) && !defined(XP_MACOSX)) || defined(XP_BEOS) || defined(XP_OS2)
static int posix_locale_category[LocaleListLength] =
{
  LC_COLLATE,
  LC_CTYPE,
  LC_MONETARY,
  LC_NUMERIC,
  LC_TIME,
#ifdef HAVE_I18N_LC_MESSAGES
  LC_MESSAGES
#else
  LC_CTYPE
#endif
};
#endif




class nsLocaleService: public nsILocaleService {

public:
	
	
	
	
	NS_DECL_ISUPPORTS

	
	
	
    NS_DECL_NSILOCALESERVICE


	nsLocaleService(void);
	virtual ~nsLocaleService(void);

protected:

	nsresult SetSystemLocale(void);
	nsresult SetApplicationLocale(void);

	nsCOMPtr<nsILocale>				mSystemLocale;
	nsCOMPtr<nsILocale>				mApplicationLocale;

};




nsLocaleService::nsLocaleService(void) 
    : mSystemLocale(0), mApplicationLocale(0)
{
#ifdef XP_WIN
    nsCOMPtr<nsIWin32Locale> win32Converter = do_GetService(NS_WIN32LOCALE_CONTRACTID);

    NS_ASSERTION(win32Converter, "nsLocaleService: can't get win32 converter\n");

    nsAutoString        xpLocale;
    if (win32Converter) {
        
        nsresult result;
        
        
        
        LCID win_lcid = GetSystemDefaultLCID();
        if (win_lcid==0) { return;}
            result = win32Converter->GetXPLocale(win_lcid, xpLocale);
        if (NS_FAILED(result)) { return;}
            result = NewLocale(xpLocale, getter_AddRefs(mSystemLocale));
        if (NS_FAILED(result)) { return;}

        
        
        
        win_lcid = GetUserDefaultLCID();
        if (win_lcid==0) { return;}
            result = win32Converter->GetXPLocale(win_lcid, xpLocale);
        if (NS_FAILED(result)) { return;}
            result = NewLocale(xpLocale, getter_AddRefs(mApplicationLocale));
        if (NS_FAILED(result)) { return;}
    }
#endif
#if (defined(XP_UNIX) && !defined(XP_MACOSX)) || defined(XP_BEOS)
    nsCOMPtr<nsIPosixLocale> posixConverter = do_GetService(NS_POSIXLOCALE_CONTRACTID);

    nsAutoString xpLocale, platformLocale;
    if (posixConverter) {
        nsAutoString category, category_platform;
        nsLocale* resultLocale;
        int i;

        resultLocale = new nsLocale();
        if ( resultLocale == NULL ) { 
            return; 
        }
        for( i = 0; i < LocaleListLength; i++ ) {
            nsresult result;
            char* lc_temp = setlocale(posix_locale_category[i], "");
            CopyASCIItoUTF16(LocaleList[i], category);
            category_platform = category; 
            category_platform.AppendLiteral("##PLATFORM");
            if (lc_temp != nsnull) {
                result = posixConverter->GetXPLocale(lc_temp, xpLocale);
                CopyASCIItoUTF16(lc_temp, platformLocale);
            } else {
                char* lang = getenv("LANG");
                if ( lang == nsnull ) {
                    platformLocale.AssignLiteral("en_US");
                    result = posixConverter->GetXPLocale("en-US", xpLocale);
                }
                else {
                    CopyASCIItoUTF16(lang, platformLocale);
                    result = posixConverter->GetXPLocale(lang, xpLocale); 
                }
            }
            if (NS_FAILED(result)) {
                return;
            }
            resultLocale->AddCategory(category, xpLocale);
            resultLocale->AddCategory(category_platform, platformLocale);
        }
        mSystemLocale = do_QueryInterface(resultLocale);
        mApplicationLocale = do_QueryInterface(resultLocale);
    }  
       
#endif 
#ifdef XP_OS2
    nsCOMPtr<nsIOS2Locale> os2Converter = do_GetService(NS_OS2LOCALE_CONTRACTID);
    nsAutoString xpLocale;
    if (os2Converter) {
        nsAutoString category;
        nsLocale* resultLocale;
        int i;

        resultLocale = new nsLocale();
        if ( resultLocale == NULL ) { 
            return; 
        }

        LocaleObject locale_object = NULL;
        int result = UniCreateLocaleObject(UNI_UCS_STRING_POINTER,
                                           (UniChar *)L"", &locale_object);
        if (result != ULS_SUCCESS) {
            int result = UniCreateLocaleObject(UNI_UCS_STRING_POINTER,
                                               (UniChar *)L"en_US", &locale_object);
        }
        char* lc_temp;
        for( i = 0; i < LocaleListLength; i++ ) {
            lc_temp = nsnull;
            UniQueryLocaleObject(locale_object,
                                 posix_locale_category[i],
                                 UNI_MBS_STRING_POINTER,
                                 (void **)&lc_temp);
            category.AssignWithConversion(LocaleList[i]);
            nsresult result;
            if (lc_temp != nsnull)
                result = os2Converter->GetXPLocale(lc_temp, xpLocale);
            else {
                char* lang = getenv("LANG");
                if ( lang == nsnull ) {
                    result = os2Converter->GetXPLocale("en-US", xpLocale);
                }
                else
                    result = os2Converter->GetXPLocale(lang, xpLocale); 
            }
            if (NS_FAILED(result)) {
                UniFreeMem(lc_temp);
                UniFreeLocaleObject(locale_object);
                return;
            }
            resultLocale->AddCategory(category, xpLocale);
            UniFreeMem(lc_temp);
        }
        UniFreeLocaleObject(locale_object);
        mSystemLocale = do_QueryInterface(resultLocale);
        mApplicationLocale = do_QueryInterface(resultLocale);
    }  
#endif  

#ifdef XP_MACOSX
    
    CFLocaleRef userLocaleRef = ::CFLocaleCopyCurrent();
    CFStringRef userLocaleStr = ::CFLocaleGetIdentifier(userLocaleRef);
    ::CFRetain(userLocaleStr);

    nsAutoBuffer<UniChar, 32> buffer;
    int size = ::CFStringGetLength(userLocaleStr);
    if (buffer.EnsureElemCapacity(size))
    {
        CFRange range = ::CFRangeMake(0, size);
        ::CFStringGetCharacters(userLocaleStr, range, buffer.get());
        buffer.get()[size] = 0;

        
        nsAutoString xpLocale(buffer.get());
        xpLocale.ReplaceChar('_', '-');

        nsresult rv = NewLocale(xpLocale, getter_AddRefs(mSystemLocale));
        if (NS_SUCCEEDED(rv)) {
            mApplicationLocale = mSystemLocale;
        }
    }

    ::CFRelease(userLocaleStr);
    ::CFRelease(userLocaleRef);

    NS_ASSERTION(mApplicationLocale, "Failed to create locale objects");
#endif 
}

nsLocaleService::~nsLocaleService(void)
{
}

NS_IMPL_THREADSAFE_ISUPPORTS1(nsLocaleService, nsILocaleService)

NS_IMETHODIMP
nsLocaleService::NewLocale(const nsAString &aLocale, nsILocale **_retval)
{
    nsresult result;

    *_retval = nsnull;

    nsLocale* resultLocale = new nsLocale();
    if (!resultLocale) return NS_ERROR_OUT_OF_MEMORY;

    for (PRInt32 i = 0; i < LocaleListLength; i++) {
      nsString category; category.AssignWithConversion(LocaleList[i]);
      result = resultLocale->AddCategory(category, aLocale);
      if (NS_FAILED(result)) { delete resultLocale; return result;}
#if (defined(XP_UNIX) && !defined(XP_MACOSX)) || defined(XP_BEOS)
      category.AppendLiteral("##PLATFORM");
      result = resultLocale->AddCategory(category, aLocale);
      if (NS_FAILED(result)) { delete resultLocale; return result;}
#endif
    }

    NS_ADDREF(*_retval = resultLocale);
    return NS_OK;
}


NS_IMETHODIMP
nsLocaleService::GetSystemLocale(nsILocale **_retval)
{
	if (mSystemLocale) {
		NS_ADDREF(*_retval = mSystemLocale);
		return NS_OK;
	}

	*_retval = (nsILocale*)nsnull;
	return NS_ERROR_FAILURE;
}

NS_IMETHODIMP
nsLocaleService::GetApplicationLocale(nsILocale **_retval)
{
	if (mApplicationLocale) {
		NS_ADDREF(*_retval = mApplicationLocale);
		return NS_OK;
	}

	*_retval=(nsILocale*)nsnull;
	return NS_ERROR_FAILURE;
}

NS_IMETHODIMP
nsLocaleService::GetLocaleFromAcceptLanguage(const char *acceptLanguage, nsILocale **_retval)
{
  char* input;
  char* cPtr;
  char* cPtr1;
  char* cPtr2;
  int i;
  int j;
  int countLang = 0;
  char	acceptLanguageList[NSILOCALE_MAX_ACCEPT_LANGUAGE][NSILOCALE_MAX_ACCEPT_LENGTH];
  nsresult	result;

  input = new char[strlen(acceptLanguage)+1];
  NS_ASSERTION(input!=nsnull,"nsLocaleFactory::GetLocaleFromAcceptLanguage: memory allocation failed.");
  if (input == (char*)NULL){ return NS_ERROR_OUT_OF_MEMORY; }

  strcpy(input, acceptLanguage);
  cPtr1 = input-1;
  cPtr2 = input;

  
  while (*(++cPtr1)) {
    if      (isalpha(*cPtr1))  *cPtr2++ = tolower(*cPtr1); 
    else if (isspace(*cPtr1))  ;                           
    else if (*cPtr1=='-')      *cPtr2++ = '_';             
    else if (*cPtr1=='*')      ;                           
    else                       *cPtr2++ = *cPtr1;          
  }
  *cPtr2 = '\0';

  countLang = 0;

  if (strchr(input,';')) {
    

    float qvalue[NSILOCALE_MAX_ACCEPT_LANGUAGE];
    float qSwap;
    float bias = 0.0f;
    char* ptrLanguage[NSILOCALE_MAX_ACCEPT_LANGUAGE];
    char* ptrSwap;

    cPtr = nsCRT::strtok(input,",",&cPtr2);
    while (cPtr) {
      qvalue[countLang] = 1.0f;
      
      if ((cPtr1 = strchr(cPtr,';')) != nsnull) {
        PR_sscanf(cPtr1,";q=%f",&qvalue[countLang]);
        *cPtr1 = '\0';
      }
      if (strlen(cPtr)<NSILOCALE_MAX_ACCEPT_LANGUAGE) {     
        qvalue[countLang] -= (bias += 0.0001f); 
        ptrLanguage[countLang++] = cPtr;
        if (countLang>=NSILOCALE_MAX_ACCEPT_LANGUAGE) break; 
      }
      cPtr = nsCRT::strtok(cPtr2,",",&cPtr2);
    }

    
    
    for ( i=0 ; i<countLang-1 ; i++ ) {
      for ( j=i+1 ; j<countLang ; j++ ) {
        if (qvalue[i]<qvalue[j]) {
          qSwap     = qvalue[i];
          qvalue[i] = qvalue[j];
          qvalue[j] = qSwap;
          ptrSwap        = ptrLanguage[i];
          ptrLanguage[i] = ptrLanguage[j];
          ptrLanguage[j] = ptrSwap;
        }
      }
    }
    for ( i=0 ; i<countLang ; i++ ) {
      PL_strncpyz(acceptLanguageList[i],ptrLanguage[i],NSILOCALE_MAX_ACCEPT_LENGTH);
    }

  } else {
    

    cPtr = nsCRT::strtok(input,",",&cPtr2);
    while (cPtr) {
      if (strlen(cPtr)<NSILOCALE_MAX_ACCEPT_LENGTH) {        
        PL_strncpyz(acceptLanguageList[countLang++],cPtr,NSILOCALE_MAX_ACCEPT_LENGTH);
        if (countLang>=NSILOCALE_MAX_ACCEPT_LENGTH) break; 
      }
      cPtr = nsCRT::strtok(cPtr2,",",&cPtr2);
    }
  }

  
  
  
  result = NS_ERROR_FAILURE;
  if (countLang>0) {
	  result = NewLocale(NS_ConvertASCIItoUTF16(acceptLanguageList[0]), _retval);
  }

  
  
  
  delete[] input;
  return result;
}


nsresult
nsLocaleService::GetLocaleComponentForUserAgent(nsAString& retval)
{
    nsCOMPtr<nsILocale>     system_locale;
    nsresult                result;

    result = GetSystemLocale(getter_AddRefs(system_locale));
    if (NS_SUCCEEDED(result))
    {
        result = system_locale->
                 GetCategory(NS_LITERAL_STRING(NSILOCALE_MESSAGE), retval);
        return result;
    }

    return result;
}



nsresult
NS_NewLocaleService(nsILocaleService** result)
{
  if(!result)
    return NS_ERROR_NULL_POINTER;
  *result = new nsLocaleService();
  if (! *result)
    return NS_ERROR_OUT_OF_MEMORY;
  NS_ADDREF(*result);
  return NS_OK;
}
