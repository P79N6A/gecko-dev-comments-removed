

















#ifndef I18N_ENCODINGS_COMPACT_LANG_DET_COMPACT_LANG_DET_IMPL_H_
#define I18N_ENCODINGS_COMPACT_LANG_DET_COMPACT_LANG_DET_IMPL_H_

#include <vector>

#include "../public/compact_lang_det.h"   
#include "integral_types.h"
#include "lang_script.h"

namespace CLD2 {


static const int kCLDFlagFinish = 1;
static const int kCLDFlagSqueeze = 2;
static const int kCLDFlagRepeats = 4;
static const int kCLDFlagTop40 = 8;
static const int kCLDFlagShort = 16;
static const int kCLDFlagHint = 32;
static const int kCLDFlagUseWords = 64;
static const int kCLDFlagUNUSED = 128;

























































  
  
  
  
  
  
  
  
  

  typedef struct {
    int perscript_count;
    const Language* perscript_lang;
  } PerScriptPair;

  typedef struct {
    
    const int kQuadHashB4Shift;
    const int kQuadHashB4bShift;
    const int kQuadHashB5Shift;
    const int kQuadHashB5bShift;
    
    const int kHashvalToSubShift;
    const uint32 kHashvalToSubMask;
    const int kHashvalToKeyShift;
    const uint32 kHashvalToKeyMask;
    const int kHashvalAssociativity;
    
    const PerScriptPair* kPerScriptPair;
    const uint16* kQuadKeyTable;
    const uint32* kQuadValueTable;
  } LangDetObj;

  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  

  Language DetectLanguageSummaryV2(
                        const char* buffer,
                        int buffer_length,
                        bool is_plain_text,
                        const CLDHints* cld_hints,
                        bool allow_extended_lang,
                        int flags,
                        Language plus_one,
                        Language* language3,
                        int* percent3,
                        double* normalized_score3,
                        ResultChunkVector* resultchunkvector,
                        int* text_bytes,
                        bool* is_reliable);

  
  
  
  
  
  int CheapSqueezeInplace(char* isrc, int srclen, int ichunksize);

}       

#endif  
