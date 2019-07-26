



#include "public/compact_lang_det.h"

extern "C" {

using namespace CLD2;

bool g_is_reliable;

const char* detectLangCode(const char* src) {
  return LanguageCode(DetectLanguage(src, strlen(src),
                                     true ,
                                     &g_is_reliable));
}

bool lastResultReliable(void) {
  return g_is_reliable;
}

}
