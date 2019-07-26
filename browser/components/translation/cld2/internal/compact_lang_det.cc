

















#include <stdio.h>
#include <stdlib.h>

#include "../public/compact_lang_det.h"
#include "../public/encodings.h"
#include "compact_lang_det_impl.h"
#include "integral_types.h"
#include "lang_script.h"

namespace CLD2 {









Language DetectLanguage(
                          const char* buffer,
                          int buffer_length,
                          bool is_plain_text,
                          bool* is_reliable) {
  bool allow_extended_lang = false;
  Language language3[3];
  int percent3[3];
  double normalized_score3[3];
  int text_bytes;
  int flags = 0;
  Language plus_one = UNKNOWN_LANGUAGE;
  const char* tld_hint = "";
  int encoding_hint = UNKNOWN_ENCODING;
  Language language_hint = UNKNOWN_LANGUAGE;
  CLDHints cldhints = {NULL, tld_hint, encoding_hint, language_hint};

  Language lang = DetectLanguageSummaryV2(
                          buffer,
                          buffer_length,
                          is_plain_text,
                          &cldhints,
                          allow_extended_lang,
                          flags,
                          plus_one,
                          language3,
                          percent3,
                          normalized_score3,
                          NULL,
                          &text_bytes,
                          is_reliable);
  
  if (lang == UNKNOWN_LANGUAGE) {
    lang = ENGLISH;
  }
  return lang;
}


Language DetectLanguageSummary(
                          const char* buffer,
                          int buffer_length,
                          bool is_plain_text,
                          Language* language3,
                          int* percent3,
                          int* text_bytes,
                          bool* is_reliable) {
  double normalized_score3[3];
  bool allow_extended_lang = false;
  int flags = 0;
  Language plus_one = UNKNOWN_LANGUAGE;
  const char* tld_hint = "";
  int encoding_hint = UNKNOWN_ENCODING;
  Language language_hint = UNKNOWN_LANGUAGE;
  CLDHints cldhints = {NULL, tld_hint, encoding_hint, language_hint};

  Language lang = DetectLanguageSummaryV2(
                          buffer,
                          buffer_length,
                          is_plain_text,
                          &cldhints,
                          allow_extended_lang,
                          flags,
                          plus_one,
                          language3,
                          percent3,
                          normalized_score3,
                          NULL,
                          text_bytes,
                          is_reliable);
  
  if (lang == UNKNOWN_LANGUAGE) {
    lang = ENGLISH;
  }
  return lang;
}



Language DetectLanguageSummary(
                          const char* buffer,
                          int buffer_length,
                          bool is_plain_text,
                          const char* tld_hint,       
                          int encoding_hint,          
                          Language language_hint,     
                          Language* language3,
                          int* percent3,
                          int* text_bytes,
                          bool* is_reliable) {
  double normalized_score3[3];
  bool allow_extended_lang = false;
  int flags = 0;
  Language plus_one = UNKNOWN_LANGUAGE;
  CLDHints cldhints = {NULL, tld_hint, encoding_hint, language_hint};

  Language lang = DetectLanguageSummaryV2(
                          buffer,
                          buffer_length,
                          is_plain_text,
                          &cldhints,
                          allow_extended_lang,
                          flags,
                          plus_one,
                          language3,
                          percent3,
                          normalized_score3,
                          NULL,
                          text_bytes,
                          is_reliable);
  
  if (lang == UNKNOWN_LANGUAGE) {
    lang = ENGLISH;
  }
  return lang;
}






Language ExtDetectLanguageSummary(
                          const char* buffer,
                          int buffer_length,
                          bool is_plain_text,
                          Language* language3,
                          int* percent3,
                          int* text_bytes,
                          bool* is_reliable) {
  double normalized_score3[3];
  bool allow_extended_lang = true;
  int flags = 0;
  Language plus_one = UNKNOWN_LANGUAGE;
  const char* tld_hint = "";
  int encoding_hint = UNKNOWN_ENCODING;
  Language language_hint = UNKNOWN_LANGUAGE;
  CLDHints cldhints = {NULL, tld_hint, encoding_hint, language_hint};

  Language lang = DetectLanguageSummaryV2(
                          buffer,
                          buffer_length,
                          is_plain_text,
                          &cldhints,
                          allow_extended_lang,
                          flags,
                          plus_one,
                          language3,
                          percent3,
                          normalized_score3,
                          NULL,
                          text_bytes,
                          is_reliable);
  
  return lang;
}






Language ExtDetectLanguageSummary(
                          const char* buffer,
                          int buffer_length,
                          bool is_plain_text,
                          const char* tld_hint,       
                          int encoding_hint,          
                          Language language_hint,     
                          Language* language3,
                          int* percent3,
                          int* text_bytes,
                          bool* is_reliable) {
  double normalized_score3[3];
  bool allow_extended_lang = true;
  int flags = 0;
  Language plus_one = UNKNOWN_LANGUAGE;
  CLDHints cldhints = {NULL, tld_hint, encoding_hint, language_hint};

  Language lang = DetectLanguageSummaryV2(
                          buffer,
                          buffer_length,
                          is_plain_text,
                          &cldhints,
                          allow_extended_lang,
                          flags,
                          plus_one,
                          language3,
                          percent3,
                          normalized_score3,
                          NULL,
                          text_bytes,
                          is_reliable);
  
  return lang;
}






Language ExtDetectLanguageSummary(
                        const char* buffer,
                        int buffer_length,
                        bool is_plain_text,
                        const char* tld_hint,       
                        int encoding_hint,          
                        Language language_hint,     
                        Language* language3,
                        int* percent3,
                        double* normalized_score3,
                        int* text_bytes,
                        bool* is_reliable) {
  bool allow_extended_lang = true;
  int flags = 0;
  Language plus_one = UNKNOWN_LANGUAGE;
  CLDHints cldhints = {NULL, tld_hint, encoding_hint, language_hint};

  Language lang = DetectLanguageSummaryV2(
                          buffer,
                          buffer_length,
                          is_plain_text,
                          &cldhints,
                          allow_extended_lang,
                          flags,
                          plus_one,
                          language3,
                          percent3,
                          normalized_score3,
                          NULL,
                          text_bytes,
                          is_reliable);
  
  return lang;
}














Language ExtDetectLanguageSummary(
                        const char* buffer,
                        int buffer_length,
                        bool is_plain_text,
                        const CLDHints* cld_hints,
                        int flags,
                        Language* language3,
                        int* percent3,
                        double* normalized_score3,
                        ResultChunkVector* resultchunkvector,
                        int* text_bytes,
                        bool* is_reliable) {
  bool allow_extended_lang = true;
  Language plus_one = UNKNOWN_LANGUAGE;

  Language lang = DetectLanguageSummaryV2(
                          buffer,
                          buffer_length,
                          is_plain_text,
                          cld_hints,
                          allow_extended_lang,
                          flags,
                          plus_one,
                          language3,
                          percent3,
                          normalized_score3,
                          resultchunkvector,
                          text_bytes,
                          is_reliable);
  
  return lang;
}

}       

