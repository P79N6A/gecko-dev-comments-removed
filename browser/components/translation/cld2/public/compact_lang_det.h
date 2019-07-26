





























































#ifndef I18N_ENCODINGS_CLD2_PUBLIC_COMPACT_LANG_DET_H_
#define I18N_ENCODINGS_CLD2_PUBLIC_COMPACT_LANG_DET_H_

#include <vector>
#include "../internal/lang_script.h"  

namespace CLD2 {

  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  


  
  
  
  
  
  
  
  
  
  
  
  
  

  typedef struct {
    const char* content_language_hint;      
    const char* tld_hint;                   
    int encoding_hint;                      
    Language language_hint;                 
  } CLDHints;

  static const int kMaxResultChunkBytes = 65535;

  
  
  typedef struct {
    int offset;                 
    uint16 bytes;               
    uint16 lang1;               
                                
  } ResultChunk;
  typedef std::vector<ResultChunk> ResultChunkVector;


  
  Language DetectLanguage(
                          const char* buffer,
                          int buffer_length,
                          bool is_plain_text,
                          bool* is_reliable);

  
  
  Language DetectLanguageSummary(
                          const char* buffer,
                          int buffer_length,
                          bool is_plain_text,
                          Language* language3,
                          int* percent3,
                          int* text_bytes,
                          bool* is_reliable);

  
  
  
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
                          bool* is_reliable);

  
  
  
  
  
  
  
  Language ExtDetectLanguageSummary(
                          const char* buffer,
                          int buffer_length,
                          bool is_plain_text,
                          Language* language3,
                          int* percent3,
                          int* text_bytes,
                          bool* is_reliable);

  
  
  
  
  
  
  
  
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
                          bool* is_reliable);

  
  
  
  
  
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
                          bool* is_reliable);


  
  
  
  
  
  
  
  
  
  
  
  
  
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
                          bool* is_reliable);

  
  
  const char* DetectLanguageVersion();


  
  static const int kCLDFlagScoreAsQuads = 0x0100;  
  static const int kCLDFlagHtml =         0x0200;  
  static const int kCLDFlagCr =           0x0400;  
  static const int kCLDFlagVerbose =      0x0800;  
  static const int kCLDFlagQuiet =        0x1000;  
  static const int kCLDFlagEcho =         0x2000;  

























void DumpResultChunkVector(FILE* f, const char* src,
                           ResultChunkVector* resultchunkvector);

#ifdef CLD2_DYNAMIC_MODE






void loadData(const char* fileName);




void unloadData();



bool isDataLoaded();

#endif 

};      

#endif  
