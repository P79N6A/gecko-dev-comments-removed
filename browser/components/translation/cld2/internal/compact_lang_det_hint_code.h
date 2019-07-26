

















#ifndef I18N_ENCODINGS_COMPACT_LANG_DET_COMPACT_LANG_DET_HINT_CODE_H__
#define I18N_ENCODINGS_COMPACT_LANG_DET_COMPACT_LANG_DET_HINT_CODE_H__


#include <string>
#include "integral_types.h"
#include "lang_script.h"
#include "../public/encodings.h"

namespace CLD2 {



typedef int16 OneCLDLangPrior;

const int kMaxOneCLDLangPrior = 14;
typedef struct {
  int32 n;
  OneCLDLangPrior prior[kMaxOneCLDLangPrior];
} CLDLangPriors;


inline int GetCLDPriorWeight(OneCLDLangPrior olp) {
  return olp >> 10;
}
inline Language GetCLDPriorLang(OneCLDLangPrior olp) {
  return static_cast<Language>(olp & 0x3ff);
}

inline int32 GetCLDLangPriorCount(CLDLangPriors* lps) {
  return lps->n;
}

inline void InitCLDLangPriors(CLDLangPriors* lps) {
  lps->n = 0;
}


void TrimCLDLangPriors(int max_entries, CLDLangPriors* lps);



std::string TrimCLDLangTagsHint(const std::string& langtags);



void SetCLDLangTagsHint(const std::string& langtags, CLDLangPriors* langpriors);



void SetCLDContentLangHint(const char* contentlang, CLDLangPriors* langpriors);



void SetCLDTLDHint(const char* tld, CLDLangPriors* langpriors);



void SetCLDEncodingHint(Encoding enc, CLDLangPriors* langpriors);



void SetCLDLanguageHint(Language lang, CLDLangPriors* langpriors);


std::string DumpCLDLangPriors(const CLDLangPriors* langpriors);




std::string GetLangTagsFromHtml(const char* utf8_body, int32 utf8_body_len,
                           int32 max_scan_bytes);

}       

#endif  

