




































#include <Carbon/Carbon.h>

#include "nsISupports.h"
#include "nscore.h"
#include "nsString.h"
#include "nsReadableUtils.h"
#include "nsMacLocale.h"
#include "nsLocaleCID.h"
#include "prprf.h"
#include "nsReadableUtils.h"
#include "nsXPCOMStrings.h"

struct iso_lang_map
{
  char*  iso_code;
  short  mac_lang_code;
  short  mac_script_code;

};
typedef struct iso_lang_map iso_lang_map;

const iso_lang_map lang_list[] = {
  { "sq", langAlbanian, smRoman },
  { "am", langAmharic, smEthiopic  },
  { "ar", langArabic, smArabic },
  { "hy", langArmenian, smArmenian},
  { "as", langAssamese, smBengali },
  { "ay", langAymara, smRoman},
  { "eu", langBasque, smRoman},
  { "bn", langBengali, smBengali },
  { "dz", langDzongkha, smTibetan },
  { "br", langBreton, smRoman },
  { "bg", langBulgarian, smCyrillic },
  { "my", langBurmese, smBurmese },
  { "km", langKhmer, smKhmer },
  { "ca", langCatalan, smRoman },
  { "zh", langTradChinese, smTradChinese },
  { "hr", langCroatian, smRoman },
  { "cs", langCzech, smCentralEuroRoman },
  { "da", langDanish, smRoman },
  { "nl", langDutch, smRoman },
  { "en", langEnglish, smRoman },
  { "eo", langEsperanto, smRoman },
  { "et", langEstonian, smCentralEuroRoman},
  { "fo", langFaeroese, smRoman },
  { "fa", langFarsi, smArabic },
  { "fi", langFinnish, smRoman },
  { "fr", langFrench, smRoman },
  { "ka", langGeorgian, smGeorgian },
  { "de", langGerman, smRoman },
  { "el", langGreek, smGreek },
  { "gn", langGuarani, smRoman },
  { "gu", langGujarati, smGujarati },
  { "he", langHebrew, smHebrew },
  { "iw", langHebrew, smHebrew },
  { "hu", langHungarian, smCentralEuroRoman }, 
  { "is", langIcelandic, smRoman },
  { "in", langIndonesian, smRoman },
  { "id", langIndonesian,  smRoman },
  { "iu", langInuktitut, smEthiopic },
  { "ga", langIrish, smRoman }, 
  { "hi", langHindi, smDevanagari },
  { "it", langItalian, smRoman },
  { "ja", langJapanese, smJapanese },
  { "jw", langJavaneseRom, smRoman },
  { "kn", langKannada, smKannada },
  { "ks", langKashmiri, smArabic },
  { "kk", langKazakh, smCyrillic },
  { "ky", langKirghiz, smCyrillic },
  { "ko", langKorean, smKorean },
  { "ku", langKurdish, smArabic },
  { "lo", langLao, smLao },
  { "la", langLatin, smRoman },
  { "lv", langLatvian, smCentralEuroRoman },
  { "lt", langLithuanian, smCentralEuroRoman },
  { "mk", langMacedonian, smCyrillic },
  { "mg", langMalagasy, smRoman },
  { "ml", langMalayalam, smMalayalam },
  { "mt", langMaltese, smRoman },
  { "mr", langMarathi, smDevanagari },
  { "mo", langMoldavian, smCyrillic },
  { "ne", langNepali, smDevanagari },
  { "nb", langNorwegian, smRoman }, 
  { "no", langNorwegian, smRoman },
  { "nn", langNynorsk, smRoman },
  { "or", langOriya, smOriya },
  { "om", langOromo, smEthiopic },
  { "ps", langPashto, smArabic },
  { "pl", langPolish, smCentralEuroRoman },
  { "pt", langPortuguese, smRoman },
  { "pa", langPunjabi, smGurmukhi },
  { "ro", langRomanian, smRoman },
  { "ru", langRussian, smCyrillic },
  { "sa", langSanskrit, smDevanagari },
  { "sr", langSerbian, smCyrillic },
  { "sd", langSindhi, smArabic },
  { "si", langSinhalese, smSinhalese },
  { "sk", langSlovak, smCentralEuroRoman },
  { "sl", langSlovenian, smRoman },
  { "so", langSomali, smRoman },
  { "es", langSpanish, smRoman },
  { "su", langSundaneseRom, smRoman },
  { "sw", langSwahili, smRoman },
  { "sv", langSwedish, smRoman }, 
  { "tl", langTagalog, smRoman },
  { "tg", langTajiki, smCyrillic },
  { "ta", langTamil, smTamil },
  { "tt", langTatar, smCyrillic },
  { "te", langTelugu, smTelugu },
  { "th", langThai, smThai },
  { "bo", langTibetan, smTibetan },
  { "ti", langTigrinya, smEthiopic },
  { "tr", langTurkish, smRoman },
  { "tk", langTurkmen, smCyrillic },
  { "ug", langUighur, smCyrillic },
  { "uk", langUkrainian, smCyrillic },
  { "ur", langUrdu, smArabic },
  { "uz", langUzbek, smCyrillic },
  { "vi", langVietnamese, smVietnamese },
  { "cy", langWelsh, smRoman },
  { "ji", langYiddish, smHebrew },
  { "yi", langYiddish, smHebrew },
  { nsnull, 0, 0}
};

struct iso_country_map
{
  char*  iso_code;
  short  mac_region_code;
};

typedef struct iso_country_map iso_country_map;

const iso_country_map country_list[] = {
  { "US", verUS},
  { "EG", verArabic},
  { "DZ", verArabic},
  { "AU", verAustralia},
  { "BE", verFrBelgium },
  { "CA", verEngCanada },
  { "CN", verChina },
  { "HR", verYugoCroatian },
  { "CY", verCyprus },
  { "DK", verDenmark },
  { "EE", verEstonia },
  { "FI", verFinland },
  { "FR", verFrance },
  { "DE", verGermany },
  { "EL", verGreece },
  { "HU", verHungary },
  { "IS", verIceland },
  { "IN", verIndiaHindi},
  { "IR", verIran },
  { "IQ", verArabic },
  { "IE", verIreland },
  { "IL", verIsrael },
  { "IT", verItaly },
  { "JP", verJapan },
  { "KR", verKorea },
  { "LV", verLatvia },
  { "LY", verArabic },
  { "LT", verLithuania },
  { "LU", verFrBelgiumLux },
  { "MT", verMalta },
  { "MA", verArabic },
  { "NL", verNetherlands },
  { "NO", verNorway },
  { "PK", verPakistan },
  { "PL", verPoland },
  { "PT", verPortugal },
  { "RU", verRussia },
  { "SA", verArabic },
  { "ES", verSpain },
  { "SE", verSweden },
  { "CH", verFrSwiss },
  { "TW", verTaiwan},
  { "TH", verThailand },
  { "TN", verArabic},
  { "TR", verTurkey },
  { "GB", verBritain },
  { nsnull, 0 }
};
  


NS_IMPL_ISUPPORTS1(nsMacLocale,nsIMacLocale)

nsMacLocale::nsMacLocale(void)
{
}

nsMacLocale::~nsMacLocale(void)
{

}

NS_IMETHODIMP 
nsMacLocale::GetPlatformLocale(const nsAString& locale, short* scriptCode, short* langCode, short* regionCode)
{
  char    locale_string[9] = {'\0','\0','\0','\0','\0','\0','\0','\0','\0'};
  char*   language_code;
  char*   country_code;
  bool  validCountryFound;
  int  i, j;

  
  const PRUnichar* data;
  j = NS_StringGetData(locale, &data);
  for (i = 0; i < 7 && i < j; i++) {
    locale_string[i] = data[i] == '-' ? '\0' : data[i];
  }

  language_code = locale_string;
  country_code = locale_string + strlen(locale_string) + 1;

  
  if (country_code[0]!=0) 
  {
    validCountryFound=false;
    for(i=0;country_list[i].iso_code;i++) {
      if (strcmp(country_list[i].iso_code,country_code)==0) {
        *regionCode = country_list[i].mac_region_code;
        validCountryFound=true;
        break;
      }
    }
    if (!validCountryFound) {
      *scriptCode = smRoman;
      *langCode = langEnglish;
      *regionCode = verUS;
      return NS_ERROR_FAILURE;
    }
  }
    
  for(i=0;lang_list[i].iso_code;i++) {
    if (strcmp(lang_list[i].iso_code, language_code)==0) {
      *scriptCode = lang_list[i].mac_script_code;
      *langCode = lang_list[i].mac_lang_code;
      return NS_OK;
    }
  }
  *scriptCode = smRoman;
  *langCode = langEnglish;
  *regionCode = verUS;
  return NS_ERROR_FAILURE;
}
  
NS_IMETHODIMP
nsMacLocale::GetXPLocale(short scriptCode, short langCode, short regionCode, nsAString& locale)
{

  int i;
  bool validResultFound = false;

  locale.Truncate();
  
  
  
  
  for(i=0;lang_list[i].iso_code;i++) {
    if (langCode==lang_list[i].mac_lang_code && scriptCode==lang_list[i].mac_script_code) {
      CopyASCIItoUTF16(nsDependentCString(lang_list[i].iso_code), locale);
      validResultFound = true;
      break;
    }
  }
  
  if (!validResultFound) {
    return NS_ERROR_FAILURE;
  }
  
  
  
  
  for(i=0;country_list[i].iso_code;i++) {
    if (regionCode==country_list[i].mac_region_code) {
      locale.Append(PRUnichar('-'));
      AppendASCIItoUTF16(country_list[i].iso_code, locale);
      validResultFound = true;
      break;
    }
  }
  
  if (validResultFound) {
    return NS_OK;
  }
  
  return NS_ERROR_FAILURE;
}
