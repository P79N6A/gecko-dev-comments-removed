






















#include "lang_script.h"

#include <stdlib.h>
#include <string.h>

#include "generated_language.h"
#include "generated_ulscript.h"

namespace CLD2 {



extern const int kLanguageToNameSize;
extern const char* const kLanguageToName[];
extern const int kLanguageToCodeSize;
extern const char* const kLanguageToCode[];
extern const int kLanguageToCNameSize;
extern const char* const kLanguageToCName[];
extern const int kLanguageToScriptsSize;
extern const FourScripts kLanguageToScripts[];


extern const int kLanguageToPLangSize;
extern const uint8 kLanguageToPLang[];

extern const uint16 kPLangToLanguageLatn[];
extern const uint16 kPLangToLanguageOthr[];


extern const int kNameToLanguageSize;
extern const CharIntPair kNameToLanguage[];
extern const int kCodeToLanguageSize;
extern const CharIntPair kCodeToLanguage[];



extern const int kULScriptToNameSize;
extern const char* const kULScriptToName[];
extern const int kULScriptToCodeSize;
extern const char* const kULScriptToCode[];
extern const int kULScriptToCNameSize;
extern const char* const kULScriptToCName[];
extern const int kULScriptToRtypeSize;
extern const ULScriptRType kULScriptToRtype[];
extern const int kULScriptToDefaultLangSize;
extern const Language kULScriptToDefaultLang[];


extern const int kNameToULScriptSize;
extern const CharIntPair kNameToULScript[];
extern const int kCodeToULScriptSize;
extern const CharIntPair kCodeToULScript[];

























































const char* ULScriptName(ULScript ulscript) {
  int i_ulscript = ulscript;
  if (i_ulscript < 0) {i_ulscript = UNKNOWN_ULSCRIPT;}
  if (i_ulscript >= NUM_ULSCRIPTS) {i_ulscript = UNKNOWN_ULSCRIPT;}
  return kULScriptToName[i_ulscript];
}

const char* ULScriptCode(ULScript ulscript) {
  int i_ulscript = ulscript;
  if (i_ulscript < 0) {i_ulscript = UNKNOWN_ULSCRIPT;}
  if (i_ulscript >= NUM_ULSCRIPTS) {i_ulscript = UNKNOWN_ULSCRIPT;}
  return kULScriptToCode[i_ulscript];
}

const char* ULScriptDeclaredName(ULScript ulscript) {
  int i_ulscript = ulscript;
  if (i_ulscript < 0) {i_ulscript = UNKNOWN_ULSCRIPT;}
  if (i_ulscript >= NUM_ULSCRIPTS) {i_ulscript = UNKNOWN_ULSCRIPT;}
  return kULScriptToCName[i_ulscript];
}

ULScriptRType ULScriptRecognitionType(ULScript ulscript) {
  int i_ulscript = ulscript;
  if (i_ulscript < 0) {i_ulscript = UNKNOWN_ULSCRIPT;}
  if (i_ulscript >= NUM_ULSCRIPTS) {i_ulscript = UNKNOWN_ULSCRIPT;}
  return kULScriptToRtype[i_ulscript];
}














































const char* LanguageName(Language lang) {
  int i_lang = lang;
  if (i_lang < 0) {i_lang = UNKNOWN_LANGUAGE;}
  if (i_lang >= NUM_LANGUAGES) {i_lang = UNKNOWN_LANGUAGE;}
  return kLanguageToName[i_lang];
}
const char* LanguageCode(Language lang) {
  int i_lang = lang;
  if (i_lang < 0) {i_lang = UNKNOWN_LANGUAGE;}
  if (i_lang >= NUM_LANGUAGES) {i_lang = UNKNOWN_LANGUAGE;}
  return kLanguageToCode[i_lang];
}

const char* LanguageDeclaredName(Language lang) {
  int i_lang = lang;
  if (i_lang < 0) {i_lang = UNKNOWN_LANGUAGE;}
  if (i_lang >= NUM_LANGUAGES) {i_lang = UNKNOWN_LANGUAGE;}
  return kLanguageToCName[i_lang];
}



ULScript LanguageRecognizedScript(Language lang, int n) {
  int i_lang = lang;
  if (i_lang < 0) {i_lang = UNKNOWN_LANGUAGE;}
  if (i_lang >= NUM_LANGUAGES) {i_lang = UNKNOWN_LANGUAGE;}
  return static_cast<ULScript>(kLanguageToScripts[i_lang][n]);
}






const char* ExtLanguageName(const Language lang) {
  return LanguageName(lang);
}


const char* ExtLanguageCode(const Language lang) {
  return LanguageCode(lang);
}





const char* ExtLanguageDeclaredName(const Language lang) {
  return LanguageDeclaredName(lang);
}


extern const int kCloseSetSize = 10;


int LanguageCloseSet(Language lang) {
  
  
  
  
  
  
  
  
  
  
  

  if (lang == INDONESIAN) {return 1;}
  if (lang == MALAY) {return 1;}

  if (lang == TIBETAN) {return 2;}
  if (lang == DZONGKHA) {return 2;}

  if (lang == CZECH) {return 3;}
  if (lang == SLOVAK) {return 3;}

  if (lang == ZULU) {return 4;}
  if (lang == XHOSA) {return 4;}

  if (lang == BOSNIAN) {return 5;}
  if (lang == CROATIAN) {return 5;}
  if (lang == SERBIAN) {return 5;}
  if (lang == MONTENEGRIN) {return 5;}

  if (lang == HINDI) {return 6;}
  if (lang == MARATHI) {return 6;}
  if (lang == BIHARI) {return 6;}
  if (lang == NEPALI) {return 6;}

  if (lang == NORWEGIAN) {return 7;}
  if (lang == NORWEGIAN_N) {return 7;}
  if (lang == DANISH) {return 7;}

  if (lang == GALICIAN) {return 8;}
  if (lang == SPANISH) {return 8;}
  if (lang == PORTUGUESE) {return 8;}

  if (lang == KINYARWANDA) {return 9;}
  if (lang == RUNDI) {return 9;}

  return 0;
}





Language DefaultLanguage(ULScript ulscript) {
  if (ulscript < 0) {return UNKNOWN_LANGUAGE;}
  if (ulscript >= NUM_ULSCRIPTS) {return UNKNOWN_LANGUAGE;}
  return kULScriptToDefaultLang[ulscript];
}

uint8 PerScriptNumber(ULScript ulscript, Language lang) {
  if (ulscript < 0) {return 0;}
  if (ulscript >= NUM_ULSCRIPTS) {return 0;}
  if (kULScriptToRtype[ulscript] == RTypeNone) {return 1;}
  if (lang >= kLanguageToPLangSize) {return 0;}
  return kLanguageToPLang[lang];
}

Language FromPerScriptNumber(ULScript ulscript, uint8 perscript_number) {
  if (ulscript < 0) {return UNKNOWN_LANGUAGE;}
  if (ulscript >= NUM_ULSCRIPTS) {return UNKNOWN_LANGUAGE;}
  if ((kULScriptToRtype[ulscript] == RTypeNone) ||
      (kULScriptToRtype[ulscript] == RTypeOne)) {
    return kULScriptToDefaultLang[ulscript];
  }

  if (ulscript == ULScript_Latin) {
     return static_cast<Language>(kPLangToLanguageLatn[perscript_number]);
  } else {
     return static_cast<Language>(kPLangToLanguageOthr[perscript_number]);
  }
}


bool IsLatnLanguage(Language lang) {
  if (lang >= kLanguageToPLangSize) {return false;}
  return (lang == kPLangToLanguageLatn[kLanguageToPLang[lang]]);
}


bool IsOthrLanguage(Language lang) {
  if (lang >= kLanguageToPLangSize) {return false;}
  return (lang == kPLangToLanguageOthr[kLanguageToPLang[lang]]);
}







int BinarySearch(const char* key, int lo, int hi, const CharIntPair* cipair) {
  
  while (lo < hi) {
    int mid = (lo + hi) >> 1;
    if (strcmp(key, cipair[mid].s) < 0) {
      hi = mid;
    } else if (strcmp(key, cipair[mid].s) > 0) {
      lo = mid + 1;
    } else {
      return mid;
    }
  }
  return -1;
}

Language MakeLang(int i) {return static_cast<Language>(i);}



Language GetLanguageFromName(const char* src) {
  const char* hyphen1 = strchr(src, '-');
  const char* hyphen2 = NULL;
  if (hyphen1 != NULL) {hyphen2 = strchr(hyphen1 + 1, '-');}

  int match = -1;
  if (hyphen1 == NULL) {
    
    match = BinarySearch(src, 0, kNameToLanguageSize, kNameToLanguage);
    if (match >= 0) {return MakeLang(kNameToLanguage[match].i);}    
    match = BinarySearch(src, 0, kCodeToLanguageSize, kCodeToLanguage);
    if (match >= 0) {return MakeLang(kCodeToLanguage[match].i);}    
    return UNKNOWN_LANGUAGE;
  }

  if (hyphen2 == NULL) {
    
    match = BinarySearch(src, 0, kCodeToLanguageSize, kCodeToLanguage);
    if (match >= 0) {return MakeLang(kCodeToLanguage[match].i);}    

    int len = strlen(src);
    if (len >= 16) {return UNKNOWN_LANGUAGE;}   

    char temp[16];
    int hyphen1_offset = hyphen1 - src;
    
    memcpy(temp, src, len);
    temp[hyphen1_offset] = '\0';
    match = BinarySearch(temp, 0, kCodeToLanguageSize, kCodeToLanguage);
    if (match >= 0) {return MakeLang(kCodeToLanguage[match].i);}    

    return UNKNOWN_LANGUAGE;
  }

  
  match = BinarySearch(src, 0, kCodeToLanguageSize, kCodeToLanguage);
  if (match >= 0) {return MakeLang(kCodeToLanguage[match].i);}    


  int len = strlen(src);
  if (len >= 16) {return UNKNOWN_LANGUAGE;}   

  char temp[16];
  int hyphen1_offset = hyphen1 - src;
  int hyphen2_offset = hyphen2 - src;
  
  memcpy(temp, src, len);
  temp[hyphen2_offset] = '\0';
  match = BinarySearch(temp, 0, kCodeToLanguageSize, kCodeToLanguage);
  if (match >= 0) {return MakeLang(kCodeToLanguage[match].i);}    


  
  int len2 = len - hyphen2_offset;
  memcpy(temp, src, len);
  memcpy(&temp[hyphen1_offset], hyphen2, len2);
  temp[hyphen1_offset + len2] = '\0';
  match = BinarySearch(temp, 0, kCodeToLanguageSize, kCodeToLanguage);
  if (match >= 0) {return MakeLang(kCodeToLanguage[match].i);}    


  
  memcpy(temp, src, len);
  temp[hyphen1_offset] = '\0';
  match = BinarySearch(temp, 0, kCodeToLanguageSize, kCodeToLanguage);
  if (match >= 0) {return MakeLang(kCodeToLanguage[match].i);}    


  return UNKNOWN_LANGUAGE;
}












ULScript MakeULScr(int i) {return static_cast<ULScript>(i);}

ULScript GetULScriptFromName(const char* src) {
  const char* hyphen1 = strchr(src, '-');
  const char* hyphen2 = NULL;
  if (hyphen1 != NULL) {hyphen2 = strchr(hyphen1 + 1, '-');}

  int match = -1;
  if (hyphen1 == NULL) {
    
    match = BinarySearch(src, 0, kNameToULScriptSize, kNameToULScript);
    if (match >= 0) {return MakeULScr(kNameToULScript[match].i);}    
    match = BinarySearch(src, 0, kCodeToULScriptSize, kCodeToULScript);
    if (match >= 0) {return MakeULScr(kCodeToULScript[match].i);}    

    Language backmap_me = GetLanguageFromName(src);
    if (backmap_me != UNKNOWN_LANGUAGE) {
      return static_cast<ULScript>(kLanguageToScripts[backmap_me][0]);
    }
    return ULScript_Latin;
  }

  if (hyphen2 == NULL) {
    
    if (strcmp(src, "zh-TW") == 0) {return ULScript_Hani;}
    if (strcmp(src, "zh-CN") == 0) {return ULScript_Hani;}
    if (strcmp(src, "sit-NP") == 0) {return ULScript_Limbu;}
    if (strcmp(src, "sit-Limb") == 0) {return ULScript_Limbu;}
    if (strcmp(src, "sr-ME") == 0) {return ULScript_Latin;}
    match = BinarySearch(src, 0, kCodeToULScriptSize, kCodeToULScript);
    if (match >= 0) {return MakeULScr(kCodeToULScript[match].i);}    

    int len = strlen(src);
    if (len >= 16) {return ULScript_Latin;}   

    char temp[16];
    int hyphen1_offset = hyphen1 - src;
    int len1 = len - hyphen1_offset - 1;    
    
    memcpy(temp, hyphen1 + 1, len1);
    temp[len1] = '\0';
    match = BinarySearch(temp, 0, kCodeToULScriptSize, kCodeToULScript);
    if (match >= 0) {return MakeULScr(kCodeToULScript[match].i);}    

    
    memcpy(temp, src, len);
    temp[hyphen1_offset] = '\0';
    match = BinarySearch(temp, 0, kCodeToULScriptSize, kCodeToULScript);
    if (match >= 0) {return MakeULScr(kCodeToULScript[match].i);}    

    return ULScript_Latin;
  }

  
  if (strcmp(src, "sit-NP-Limb") == 0) {return ULScript_Limbu;}
  if (strcmp(src, "sr-ME-Latn") == 0) {return ULScript_Latin;}
  if (strcmp(src, "sr-ME-Cyrl") == 0) {return ULScript_Cyrillic;}
  match = BinarySearch(src, 0, kCodeToULScriptSize, kCodeToULScript);
  if (match >= 0) {return MakeULScr(kCodeToULScript[match].i);}    

  int len = strlen(src);
  if (len >= 16) {return ULScript_Latin;}   

  char temp[16];
  int hyphen1_offset = hyphen1 - src;
  int hyphen2_offset = hyphen2 - src;
  int len2 = len - hyphen2_offset - 1;                
  int lenmid = hyphen2_offset - hyphen1_offset - 1;   
  
  memcpy(temp, hyphen1 + 1, lenmid);
  temp[lenmid] = '\0';
  match = BinarySearch(temp, 0, kCodeToULScriptSize, kCodeToULScript);
  if (match >= 0) {return MakeULScr(kCodeToULScript[match].i);}    

  
  memcpy(temp, hyphen2 + 1, len2);
  temp[len2] = '\0';
  match = BinarySearch(temp, 0, kCodeToULScriptSize, kCodeToULScript);
  if (match >= 0) {return MakeULScr(kCodeToULScript[match].i);}    

  
  memcpy(temp, src, len);
  temp[hyphen1_offset] = '\0';
  match = BinarySearch(temp, 0, kCodeToULScriptSize, kCodeToULScript);
  if (match >= 0) {return MakeULScr(kCodeToULScript[match].i);}    

  return ULScript_Latin;
}


int LScript4(ULScript ulscript) {
  if (ulscript == ULScript_Latin) {return 0;}
  if (ulscript == ULScript_Cyrillic) {return 1;}
  if (ulscript == ULScript_Arabic) {return 2;}
  return 3;
}

}  

