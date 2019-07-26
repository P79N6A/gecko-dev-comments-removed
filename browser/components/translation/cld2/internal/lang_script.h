























#ifndef I18N_ENCODINGS_CLD2_LANG_SCRIPT_H__
#define I18N_ENCODINGS_CLD2_LANG_SCRIPT_H__

#include "generated_language.h"
#include "generated_ulscript.h"
#include "integral_types.h"








































namespace CLD2 {







const char* ULScriptName(ULScript ulscript);
const char* ULScriptCode(ULScript ulscript);
const char* ULScriptDeclaredName(ULScript ulscript);
ULScriptRType ULScriptRecognitionType(ULScript ulscript);



ULScript GetULScriptFromName(const char* src);


int LScript4(ULScript ulscript);









































const char* LanguageName(Language lang);
const char* LanguageCode(Language lang);
const char* LanguageShortCode(Language lang);
const char* LanguageDeclaredName(Language lang);



ULScript LanguageRecognizedScript(Language lang, int n);



Language GetLanguageFromName(const char* src);


int LanguageCloseSet(Language lang);






Language DefaultLanguage(ULScript ulscript);












uint8 PerScriptNumber(ULScript ulscript, Language lang);
Language FromPerScriptNumber(ULScript ulscript, uint8 perscript_number);







bool IsLatnLanguage(Language lang);
bool IsOthrLanguage(Language lang);







int BinarySearch(const char* key, int lo, int hi, const CharIntPair* cipair);

}  

#endif  
