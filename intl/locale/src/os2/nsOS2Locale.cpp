

















































#include "nsISupports.h"
#include "nscore.h"
#include "nsString.h"
#include "nsOS2Locale.h"
#include "nsLocaleCID.h"
#include "prprf.h"
#include "nsReadableUtils.h"
#include <unidef.h>

extern "C" {
#include <callconv.h>
int APIENTRY UniQueryLocaleValue ( const LocaleObject locale_object,
                                   LocaleItem item,
                                  int *info_item);
}


NS_IMPL_ISUPPORTS1(nsOS2Locale,nsIOS2Locale)

nsOS2Locale::nsOS2Locale(void)
{
}

nsOS2Locale::~nsOS2Locale(void)
{

}


#ifndef LOCI_sName
#define LOCI_sName ((LocaleItem)100)
#endif

NS_IMETHODIMP 
nsOS2Locale::GetPlatformLocale(const nsAString& locale, PULONG os2Codepage)
{
  LocaleObject locObj = NULL;
  int codePage;
  nsAutoString tempLocale(locale);
  tempLocale.ReplaceChar('-', '_');

 
  int  ret = UniCreateLocaleObject(UNI_UCS_STRING_POINTER, (UniChar *)PromiseFlatString(tempLocale).get(), &locObj);
  if (ret != ULS_SUCCESS)
    UniCreateLocaleObject(UNI_UCS_STRING_POINTER, (UniChar *)L"C", &locObj);

  ret = UniQueryLocaleValue(locObj, LOCI_iCodepage, &codePage);
  if (ret != ULS_SUCCESS)
    return NS_ERROR_FAILURE;

  if (codePage == 437) {
    *os2Codepage = 850;
  } else {
    *os2Codepage = codePage;
  }
  UniFreeLocaleObject(locObj);

  return NS_OK;
}

NS_IMETHODIMP
nsOS2Locale::GetXPLocale(const char* os2Locale, nsAString& locale)
{
  char  country_code[MAX_COUNTRY_CODE_LEN];
  char  lang_code[MAX_LANGUAGE_CODE_LEN];
  char  extra[MAX_EXTRA_LEN];
  char  os2_locale[MAX_LOCALE_LEN];

  if (os2Locale!=nsnull) {
    if (strcmp(os2Locale,"C")==0 || strcmp(os2Locale,"OS2")==0) {
      locale.AssignLiteral("en-US");
      return NS_OK;
    }
    if (!ParseLocaleString(os2Locale,lang_code,country_code,extra,'_')) {

      
      CopyASCIItoUTF16(nsDependentCString(os2Locale), locale);  
      return NS_OK;
    }

    if (*country_code) {
      if (*extra) {
        PR_snprintf(os2_locale,MAX_LOCALE_LEN,"%s-%s.%s",lang_code,country_code,extra);
      }
      else {
        PR_snprintf(os2_locale,MAX_LOCALE_LEN,"%s-%s",lang_code,country_code);
      }
    } 
    else {
      if (*extra) {
        PR_snprintf(os2_locale,MAX_LOCALE_LEN,"%s.%s",lang_code,extra);
      }
      else {
        PR_snprintf(os2_locale,MAX_LOCALE_LEN,"%s",lang_code);
      }
    }

    CopyASCIItoUTF16(nsDependentCString(os2_locale), locale);  
    return NS_OK;

  }

  return NS_ERROR_FAILURE;
}




PRBool
nsOS2Locale::ParseLocaleString(const char* locale_string, char* language, char* country, char* extra, char separator)
{
  const char *src = locale_string;
  char modifier[MAX_EXTRA_LEN+1];
  char *dest;
  int dest_space, len;

  *language = '\0';
  *country = '\0';
  *extra = '\0';
  if (strlen(locale_string) < 2) {
    return(PR_FALSE);
  }

  
  
  
  dest = language;
  dest_space = MAX_LANGUAGE_CODE_LEN;
  while ((*src) && (isalpha(*src)) && (dest_space--)) {
    *dest++ = tolower(*src++);
  }
  *dest = '\0';
  len = dest - language;
  if ((len != 2) && (len != 3)) {
    NS_ASSERTION((len == 2) || (len == 3), "language code too short");
    NS_ASSERTION(len < 3, "reminder: verify we can handle 3+ character language code in all parts of the system; eg: language packs");
    *language = '\0';
    return(PR_FALSE);
  }

  
  if (*src == '\0') {
    return(PR_TRUE);
  }

  if ((*src != '_') && (*src != '-') && (*src != '.') && (*src != '@')) {
    NS_ASSERTION(isalpha(*src), "language code too long");
    NS_ASSERTION(!isalpha(*src), "unexpected language/country separator");
    *language = '\0';
    return(PR_FALSE);
  }

  
  
  
  if ((*src == '_') || (*src == '-')) { 
    src++;
    dest = country;
    dest_space = MAX_COUNTRY_CODE_LEN;
    while ((*src) && (isalpha(*src)) && (dest_space--)) {
      *dest++ = toupper(*src++);
    }
    *dest = '\0';
    len = dest - country;
    if (len != 2) {
      NS_ASSERTION(len == 2, "unexpected country code length");
      *language = '\0';
      *country = '\0';
      return(PR_FALSE);
    }
  }

  
  if (*src == '\0') {
    return(PR_TRUE);
  }

  if ((*src != '.') && (*src != '@') && (*src != separator)) {
    NS_ASSERTION(isalpha(*src), "country code too long");
    NS_ASSERTION(!isalpha(*src), "unexpected country/extra separator");
    *language = '\0';
    *country = '\0';
    return(PR_FALSE);
  }

  
  
  
  if (*src == '.') { 
    src++;  
    dest = extra;
    dest_space = MAX_EXTRA_LEN;
    while ((*src) && (*src != '@') && (dest_space--)) {
      *dest++ = *src++;
    }
    *dest = '\0';
    len = dest - extra;
    if (len < 1) {
      NS_ASSERTION(len > 0, "found country/extra separator but no extra code");
      *language = '\0';
      *country = '\0';
      *extra = '\0';
      return(PR_FALSE);
    }
  }

  
  if (*src == '\0') {
    return(PR_TRUE);
  }

  
  
  
  if ((*src == '@') || (*src == separator)) { 
    src++;  
    NS_ASSERTION(stricmp("euro",src) == 0, "found non euro modifier");
    dest = modifier;
    dest_space = MAX_EXTRA_LEN;
    while ((*src) && (dest_space--)) {
      *dest++ = *src++;
    }
    *dest = '\0';
    len = dest - modifier;
    if (len < 1) {
      NS_ASSERTION(len > 0, "found modifier separator but no modifier code");
      *language = '\0';
      *country = '\0';
      *extra = '\0';
      *modifier = '\0';
      return(PR_FALSE);
    }
  }

  
  if (*src == '\0') {
    return(PR_TRUE);
  }

  NS_ASSERTION(*src == '\0', "extra/modifier code too long");
  *language = '\0';
  *country = '\0';
  *extra = '\0';

  return(PR_FALSE);
}
