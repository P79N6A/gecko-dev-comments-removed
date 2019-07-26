


















#include <stdio.h>
#include <string.h>
#include <string>
#include <vector>

#include "cldutil.h"
#include "debug.h"
#include "integral_types.h"
#include "lang_script.h"
#include "utf8statetable.h"

#ifdef CLD2_DYNAMIC_MODE
#include "cld2_dynamic_data.h"
#include "cld2_dynamic_data_loader.h"
#endif
#include "cld2tablesummary.h"
#include "compact_lang_det_impl.h"
#include "compact_lang_det_hint_code.h"
#include "getonescriptspan.h"
#include "tote.h"


namespace CLD2 {

using namespace std;












extern const int kLanguageToPLangSize;
extern const int kCloseSetSize;

extern const UTF8PropObj cld_generated_CjkUni_obj;
extern const CLD2TableSummary kCjkCompat_obj;
extern const CLD2TableSummary kCjkDeltaBi_obj;
extern const CLD2TableSummary kDistinctBiTable_obj;
extern const CLD2TableSummary kQuad_obj;
extern const CLD2TableSummary kQuad_obj2;     
extern const CLD2TableSummary kDeltaOcta_obj;
extern const CLD2TableSummary kDistinctOcta_obj;
extern const short kAvgDeltaOctaScore[];

#ifdef CLD2_DYNAMIC_MODE
  
  
  static ScoringTables kScoringtables = {
    NULL, 
    NULL, 
    NULL, 
    NULL, 
    NULL, 
    NULL, 
    NULL, 
    NULL, 
    NULL, 
  };
  static bool dynamicDataLoaded = false;
  static ScoringTables* dynamicTables = NULL;
  static void* mmapAddress = NULL;
  static int mmapLength = 0;

  bool isDataLoaded() { return dynamicDataLoaded; }

  void loadData(const char* fileName) {
    if (isDataLoaded()) {
      unloadData();
    }
    dynamicTables = CLD2DynamicDataLoader::loadDataFile(fileName, &mmapAddress, &mmapLength);
    kScoringtables = *dynamicTables;
    dynamicDataLoaded = true;
  };

  void unloadData() {
    if (!dynamicDataLoaded) return;
    dynamicDataLoaded = false;
    
    CLD2DynamicDataLoader::unloadData(&dynamicTables, &mmapAddress, &mmapLength);
  }
#else
  
  static const ScoringTables kScoringtables = {
    &cld_generated_CjkUni_obj,
    &kCjkCompat_obj,
    &kCjkDeltaBi_obj,
    &kDistinctBiTable_obj,

    &kQuad_obj,
    &kQuad_obj2,                              
    &kDeltaOcta_obj,
    &kDistinctOcta_obj,

    kAvgDeltaOctaScore,
  };
#endif 


static const bool FLAGS_cld_no_minimum_bytes = false;
static const bool FLAGS_cld_forcewords = true;
static const bool FLAGS_cld_showme = false;
static const bool FLAGS_cld_echotext = true;
static const int32 FLAGS_cld_textlimit = 160;
static const int32 FLAGS_cld_smoothwidth = 20;
static const bool FLAGS_cld_2011_hints = true;
static const int32 FLAGS_cld_max_lang_tag_scan_kb = 8;

static const bool FLAGS_dbgscore = false;


static const int kLangHintInitial = 12;  
static const int kLangHintBoost = 12;    

static const int kShortSpanThresh = 32;       
static const int kMaxSecondChanceLen = 1024;  

static const int kCheapSqueezeTestThresh = 4096;  
                                                  
static const int kCheapSqueezeTestLen = 256;  
static const int kSpacesTriggerPercent = 25;  
static const int kPredictTriggerPercent = 67; 

static const int kChunksizeDefault = 48;      
static const int kSpacesThreshPercent = 25;   
static const int kPredictThreshPercent = 40;  

static const int kMaxSpaceScan = 32;          

static const int kGoodLang1Percent = 70;
static const int kGoodLang1and2Percent = 93;
static const int kShortTextThresh = 256;      

static const int kMinChunkSizeQuads = 4;      
static const int kMaxChunkSizeQuads = 1024;   

static const int kDefaultWordSpan = 256;      
                                              
static const int kReallyBigWordSpan = 9999999;  

static const int kMinReliableSeq = 50;      

static const int kPredictionTableSize = 4096;   
                                                

static const int kNonEnBoilerplateMinPercent = 17;    
static const int kNonFIGSBoilerplateMinPercent = 20;  
static const int kGoodFirstMinPercent = 26;           
static const int kGoodFirstReliableMinPercent = 51;   
static const int kIgnoreMaxPercent = 20;              
static const int kKeepMinPercent = 2;                 












static const int kMinCorrPercent = 24;        
                                              
                                              
static Language Unknown = UNKNOWN_LANGUAGE;



static const Language kClosestAltLanguage[] = {
  (28 >= kMinCorrPercent) ? SCOTS : UNKNOWN_LANGUAGE,  
  (36 >= kMinCorrPercent) ? NORWEGIAN : UNKNOWN_LANGUAGE,  
  (31 >= kMinCorrPercent) ? AFRIKAANS : UNKNOWN_LANGUAGE,  
  (15 >= kMinCorrPercent) ? ESTONIAN : UNKNOWN_LANGUAGE,  
  (11 >= kMinCorrPercent) ? OCCITAN : UNKNOWN_LANGUAGE,  
  (17 >= kMinCorrPercent) ? LUXEMBOURGISH : UNKNOWN_LANGUAGE,  
  (27 >= kMinCorrPercent) ? YIDDISH : UNKNOWN_LANGUAGE,  
  (16 >= kMinCorrPercent) ? CORSICAN : UNKNOWN_LANGUAGE,  
  ( 0 >= kMinCorrPercent) ? Unknown : UNKNOWN_LANGUAGE,  
  ( 0 >= kMinCorrPercent) ? Unknown : UNKNOWN_LANGUAGE,  
  (41 >= kMinCorrPercent) ? NORWEGIAN_N : UNKNOWN_LANGUAGE,  
  ( 5 >= kMinCorrPercent) ? SLOVAK : UNKNOWN_LANGUAGE,  
  (23 >= kMinCorrPercent) ? SPANISH : UNKNOWN_LANGUAGE,  
  (33 >= kMinCorrPercent) ? BULGARIAN : UNKNOWN_LANGUAGE,  
  (28 >= kMinCorrPercent) ? GALICIAN : UNKNOWN_LANGUAGE,  
  (17 >= kMinCorrPercent) ? NORWEGIAN : UNKNOWN_LANGUAGE,  
  ( 0 >= kMinCorrPercent) ? Unknown : UNKNOWN_LANGUAGE,  
  (42 >= kMinCorrPercent) ? SLOVAK : UNKNOWN_LANGUAGE,  
  ( 0 >= kMinCorrPercent) ? Unknown : UNKNOWN_LANGUAGE,  
  (35 >= kMinCorrPercent) ? FAROESE : UNKNOWN_LANGUAGE,  
  ( 7 >= kMinCorrPercent) ? LITHUANIAN : UNKNOWN_LANGUAGE,  
  ( 7 >= kMinCorrPercent) ? LATVIAN : UNKNOWN_LANGUAGE,  
  ( 4 >= kMinCorrPercent) ? LATIN : UNKNOWN_LANGUAGE,  
  ( 4 >= kMinCorrPercent) ? SLOVAK : UNKNOWN_LANGUAGE,  
  (15 >= kMinCorrPercent) ? FINNISH : UNKNOWN_LANGUAGE,  
  ( 0 >= kMinCorrPercent) ? Unknown : UNKNOWN_LANGUAGE,  
  ( 0 >= kMinCorrPercent) ? Unknown : UNKNOWN_LANGUAGE,  
  (33 >= kMinCorrPercent) ? RUSSIAN : UNKNOWN_LANGUAGE,  
  ( 0 >= kMinCorrPercent) ? Unknown : UNKNOWN_LANGUAGE,  
  ( 0 >= kMinCorrPercent) ? Unknown : UNKNOWN_LANGUAGE,  
  (24 >= kMinCorrPercent) ? SCOTS_GAELIC : UNKNOWN_LANGUAGE,  
  (28 >= kMinCorrPercent) ? SPANISH : UNKNOWN_LANGUAGE,  
  ( 8 >= kMinCorrPercent) ? INDONESIAN : UNKNOWN_LANGUAGE,  
  (29 >= kMinCorrPercent) ? AZERBAIJANI : UNKNOWN_LANGUAGE,  
  (28 >= kMinCorrPercent) ? RUSSIAN : UNKNOWN_LANGUAGE,  
  (37 >= kMinCorrPercent) ? MARATHI : UNKNOWN_LANGUAGE,  
  (29 >= kMinCorrPercent) ? BULGARIAN : UNKNOWN_LANGUAGE,  
  (14 >= kMinCorrPercent) ? ASSAMESE : UNKNOWN_LANGUAGE,  
  (46 >= kMinCorrPercent) ? MALAY : UNKNOWN_LANGUAGE,  
  ( 9 >= kMinCorrPercent) ? INTERLINGUA : UNKNOWN_LANGUAGE,  
  (46 >= kMinCorrPercent) ? INDONESIAN : UNKNOWN_LANGUAGE,  
  ( 0 >= kMinCorrPercent) ? Unknown : UNKNOWN_LANGUAGE,  
  ( 4 >= kMinCorrPercent) ? BRETON : UNKNOWN_LANGUAGE,  
  ( 8 >= kMinCorrPercent) ? HINDI : UNKNOWN_LANGUAGE,  
  ( 0 >= kMinCorrPercent) ? Unknown : UNKNOWN_LANGUAGE,  
  ( 3 >= kMinCorrPercent) ? ESPERANTO : UNKNOWN_LANGUAGE,  
  ( 0 >= kMinCorrPercent) ? Unknown : UNKNOWN_LANGUAGE,  
  (22 >= kMinCorrPercent) ? UKRAINIAN : UNKNOWN_LANGUAGE,  
  (15 >= kMinCorrPercent) ? SUNDANESE : UNKNOWN_LANGUAGE,  
  (19 >= kMinCorrPercent) ? CATALAN : UNKNOWN_LANGUAGE,  
  (27 >= kMinCorrPercent) ? PERSIAN : UNKNOWN_LANGUAGE,  
  (36 >= kMinCorrPercent) ? HINDI : UNKNOWN_LANGUAGE,  
  ( 0 >= kMinCorrPercent) ? Unknown : UNKNOWN_LANGUAGE,  
  ( 0 >= kMinCorrPercent) ? Unknown : UNKNOWN_LANGUAGE,  
  (24 >= kMinCorrPercent) ? PERSIAN : UNKNOWN_LANGUAGE,  
  (19 >= kMinCorrPercent) ? OCCITAN : UNKNOWN_LANGUAGE,  
  ( 4 >= kMinCorrPercent) ? LATIN : UNKNOWN_LANGUAGE,  
  ( 3 >= kMinCorrPercent) ? GERMAN : UNKNOWN_LANGUAGE,  
  ( 9 >= kMinCorrPercent) ? LATIN : UNKNOWN_LANGUAGE,  
  ( 0 >= kMinCorrPercent) ? Unknown : UNKNOWN_LANGUAGE,  
  ( 0 >= kMinCorrPercent) ? Unknown : UNKNOWN_LANGUAGE,  
  (24 >= kMinCorrPercent) ? IRISH : UNKNOWN_LANGUAGE,  
  ( 7 >= kMinCorrPercent) ? KINYARWANDA : UNKNOWN_LANGUAGE,  
  (28 >= kMinCorrPercent) ? SERBIAN : UNKNOWN_LANGUAGE,  
  (37 >= kMinCorrPercent) ? HINDI : UNKNOWN_LANGUAGE,  
  ( 3 >= kMinCorrPercent) ? ITALIAN : UNKNOWN_LANGUAGE,  
  ( 1 >= kMinCorrPercent) ? YORUBA : UNKNOWN_LANGUAGE,  
  (15 >= kMinCorrPercent) ? DUTCH : UNKNOWN_LANGUAGE,  
  (42 >= kMinCorrPercent) ? CZECH : UNKNOWN_LANGUAGE,  
  
  (24 >= kMinCorrPercent) ? CHINESE : UNKNOWN_LANGUAGE,  
  (35 >= kMinCorrPercent) ? ICELANDIC : UNKNOWN_LANGUAGE,  
  (15 >= kMinCorrPercent) ? JAVANESE : UNKNOWN_LANGUAGE,  
  (17 >= kMinCorrPercent) ? TAJIK : UNKNOWN_LANGUAGE,  
  ( 7 >= kMinCorrPercent) ? TIGRINYA : UNKNOWN_LANGUAGE,  
  (29 >= kMinCorrPercent) ? TURKISH : UNKNOWN_LANGUAGE,  
  ( 0 >= kMinCorrPercent) ? Unknown : UNKNOWN_LANGUAGE,  
  ( 7 >= kMinCorrPercent) ? AMHARIC : UNKNOWN_LANGUAGE,  
  (27 >= kMinCorrPercent) ? URDU : UNKNOWN_LANGUAGE,  
  ( 0 >= kMinCorrPercent) ? Unknown : UNKNOWN_LANGUAGE,  
  ( 0 >= kMinCorrPercent) ? Unknown : UNKNOWN_LANGUAGE,  
  (41 >= kMinCorrPercent) ? NORWEGIAN : UNKNOWN_LANGUAGE,  
  ( 0 >= kMinCorrPercent) ? Unknown : UNKNOWN_LANGUAGE,  
  ( 0 >= kMinCorrPercent) ? Unknown : UNKNOWN_LANGUAGE,  
  (37 >= kMinCorrPercent) ? ZULU : UNKNOWN_LANGUAGE,  
  (37 >= kMinCorrPercent) ? XHOSA : UNKNOWN_LANGUAGE,  
  ( 2 >= kMinCorrPercent) ? SPANISH : UNKNOWN_LANGUAGE,  
  (29 >= kMinCorrPercent) ? TSWANA : UNKNOWN_LANGUAGE,  
  ( 7 >= kMinCorrPercent) ? TURKISH : UNKNOWN_LANGUAGE,  
  ( 8 >= kMinCorrPercent) ? KAZAKH : UNKNOWN_LANGUAGE,  
  ( 5 >= kMinCorrPercent) ? FRENCH : UNKNOWN_LANGUAGE,  
  ( 3 >= kMinCorrPercent) ? GANDA : UNKNOWN_LANGUAGE,  
  (27 >= kMinCorrPercent) ? HEBREW : UNKNOWN_LANGUAGE,  
  (28 >= kMinCorrPercent) ? SLOVENIAN : UNKNOWN_LANGUAGE,  
  (12 >= kMinCorrPercent) ? OROMO : UNKNOWN_LANGUAGE,  
  ( 9 >= kMinCorrPercent) ? UZBEK : UNKNOWN_LANGUAGE,  
  (15 >= kMinCorrPercent) ? PERSIAN : UNKNOWN_LANGUAGE,  
  ( 6 >= kMinCorrPercent) ? KYRGYZ : UNKNOWN_LANGUAGE,  
  ( 0 >= kMinCorrPercent) ? Unknown : UNKNOWN_LANGUAGE,  
  ( 0 >= kMinCorrPercent) ? Unknown : UNKNOWN_LANGUAGE,  
  ( 8 >= kMinCorrPercent) ? URDU : UNKNOWN_LANGUAGE,  
  (10 >= kMinCorrPercent) ? ITALIAN : UNKNOWN_LANGUAGE,  
  (31 >= kMinCorrPercent) ? DUTCH : UNKNOWN_LANGUAGE,  
  (17 >= kMinCorrPercent) ? GERMAN : UNKNOWN_LANGUAGE,  
  ( 2 >= kMinCorrPercent) ? SCOTS : UNKNOWN_LANGUAGE,  
  ( 0 >= kMinCorrPercent) ? Unknown : UNKNOWN_LANGUAGE,  
  (45 >= kMinCorrPercent) ? DZONGKHA : UNKNOWN_LANGUAGE,  
  ( 0 >= kMinCorrPercent) ? Unknown : UNKNOWN_LANGUAGE,  
  ( 0 >= kMinCorrPercent) ? Unknown : UNKNOWN_LANGUAGE,  
  ( 0 >= kMinCorrPercent) ? Unknown : UNKNOWN_LANGUAGE,  
  ( 8 >= kMinCorrPercent) ? DUTCH : UNKNOWN_LANGUAGE,  
  ( 0 >= kMinCorrPercent) ? Unknown : UNKNOWN_LANGUAGE,  
  (14 >= kMinCorrPercent) ? BENGALI : UNKNOWN_LANGUAGE,  
  (16 >= kMinCorrPercent) ? ITALIAN : UNKNOWN_LANGUAGE,  
  ( 5 >= kMinCorrPercent) ? INTERLINGUA : UNKNOWN_LANGUAGE,  
  ( 8 >= kMinCorrPercent) ? KYRGYZ : UNKNOWN_LANGUAGE,  
  ( 4 >= kMinCorrPercent) ? SWAHILI : UNKNOWN_LANGUAGE,  
  (11 >= kMinCorrPercent) ? RUSSIAN : UNKNOWN_LANGUAGE,  
  (19 >= kMinCorrPercent) ? PERSIAN : UNKNOWN_LANGUAGE,  
  ( 5 >= kMinCorrPercent) ? AYMARA : UNKNOWN_LANGUAGE,  
  ( 5 >= kMinCorrPercent) ? KINYARWANDA : UNKNOWN_LANGUAGE,  
  (17 >= kMinCorrPercent) ? UZBEK : UNKNOWN_LANGUAGE,  
  (13 >= kMinCorrPercent) ? BASHKIR : UNKNOWN_LANGUAGE,  
  (11 >= kMinCorrPercent) ? SAMOAN : UNKNOWN_LANGUAGE,  
  ( 2 >= kMinCorrPercent) ? TWI : UNKNOWN_LANGUAGE,  
  ( 0 >= kMinCorrPercent) ? Unknown : UNKNOWN_LANGUAGE,  
  ( 0 >= kMinCorrPercent) ? Unknown : UNKNOWN_LANGUAGE,  
  ( 0 >= kMinCorrPercent) ? Unknown : UNKNOWN_LANGUAGE,  
  ( 0 >= kMinCorrPercent) ? Unknown : UNKNOWN_LANGUAGE,  
  ( 6 >= kMinCorrPercent) ? TONGA : UNKNOWN_LANGUAGE,  
  ( 3 >= kMinCorrPercent) ? OROMO : UNKNOWN_LANGUAGE,  
  ( 1 >= kMinCorrPercent) ? MONGOLIAN : UNKNOWN_LANGUAGE,  
  ( 8 >= kMinCorrPercent) ? SOMALI : UNKNOWN_LANGUAGE,  
  ( 5 >= kMinCorrPercent) ? QUECHUA : UNKNOWN_LANGUAGE,  
  (13 >= kMinCorrPercent) ? TATAR : UNKNOWN_LANGUAGE,  
  ( 3 >= kMinCorrPercent) ? ENGLISH : UNKNOWN_LANGUAGE,  
  (45 >= kMinCorrPercent) ? TIBETAN : UNKNOWN_LANGUAGE,  
  ( 4 >= kMinCorrPercent) ? TONGA : UNKNOWN_LANGUAGE,  
  ( 7 >= kMinCorrPercent) ? INUPIAK : UNKNOWN_LANGUAGE,  
  ( 3 >= kMinCorrPercent) ? AFAR : UNKNOWN_LANGUAGE,  
  ( 3 >= kMinCorrPercent) ? OCCITAN : UNKNOWN_LANGUAGE,  
  ( 7 >= kMinCorrPercent) ? GREENLANDIC : UNKNOWN_LANGUAGE,  
  ( 0 >= kMinCorrPercent) ? Unknown : UNKNOWN_LANGUAGE,  
  ( 4 >= kMinCorrPercent) ? HINDI : UNKNOWN_LANGUAGE,  
  (30 >= kMinCorrPercent) ? RUNDI : UNKNOWN_LANGUAGE,  
  ( 2 >= kMinCorrPercent) ? TAGALOG : UNKNOWN_LANGUAGE,  
  (17 >= kMinCorrPercent) ? GERMAN : UNKNOWN_LANGUAGE,  
  (12 >= kMinCorrPercent) ? SOMALI : UNKNOWN_LANGUAGE,  
  (30 >= kMinCorrPercent) ? KINYARWANDA : UNKNOWN_LANGUAGE,  
  (11 >= kMinCorrPercent) ? TONGA : UNKNOWN_LANGUAGE,  
  ( 1 >= kMinCorrPercent) ? LINGALA : UNKNOWN_LANGUAGE,  
  (32 >= kMinCorrPercent) ? MARATHI : UNKNOWN_LANGUAGE,  
  (16 >= kMinCorrPercent) ? ZULU : UNKNOWN_LANGUAGE,  
  ( 5 >= kMinCorrPercent) ? SISWANT : UNKNOWN_LANGUAGE,  
  (29 >= kMinCorrPercent) ? SESOTHO : UNKNOWN_LANGUAGE,  
  ( 2 >= kMinCorrPercent) ? ESTONIAN : UNKNOWN_LANGUAGE,  
  ( 0 >= kMinCorrPercent) ? Unknown : UNKNOWN_LANGUAGE,  
  ( 1 >= kMinCorrPercent) ? MALAY : UNKNOWN_LANGUAGE,  
  (28 >= kMinCorrPercent) ? ENGLISH : UNKNOWN_LANGUAGE,  
  (15 >= kMinCorrPercent) ? KINYARWANDA : UNKNOWN_LANGUAGE,  
  ( 7 >= kMinCorrPercent) ? ENGLISH : UNKNOWN_LANGUAGE,  
  ( 0 >= kMinCorrPercent) ? Unknown : UNKNOWN_LANGUAGE,  

  ( 0 >= kMinCorrPercent) ? Unknown : UNKNOWN_LANGUAGE,  
  ( 0 >= kMinCorrPercent) ? Unknown : UNKNOWN_LANGUAGE,  
  ( 0 >= kMinCorrPercent) ? Unknown : UNKNOWN_LANGUAGE,  
  ( 0 >= kMinCorrPercent) ? Unknown : UNKNOWN_LANGUAGE,  
};





inline bool FlagFinish(int flags) {return (flags & kCLDFlagFinish) != 0;}
inline bool FlagSqueeze(int flags) {return (flags & kCLDFlagSqueeze) != 0;}
inline bool FlagRepeats(int flags) {return (flags & kCLDFlagRepeats) != 0;}
inline bool FlagTop40(int flags) {return (flags & kCLDFlagTop40) != 0;}
inline bool FlagShort(int flags) {return (flags & kCLDFlagShort) != 0;}
inline bool FlagHint(int flags) {return (flags & kCLDFlagHint) != 0;}
inline bool FlagUseWords(int flags) {return (flags & kCLDFlagUseWords) != 0;}


  

  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  


void DemoteNotTop40(Tote* chunk_tote, uint16 psplus_one) {
  
}

void PrintText(FILE* f, Language cur_lang, const string& temp) {
  if (temp.size() == 0) {return;}
  fprintf(f, "PrintText[%s]%s<br>\n", LanguageName(cur_lang), temp.c_str());
}





static Language prior_lang = UNKNOWN_LANGUAGE;
static bool prior_unreliable = false;









int BackscanToSpace(const char* src, int limit) {
  int n = 0;
  limit = minint(limit, kMaxSpaceScan);
  while (n < limit) {
    if (src[-n - 1] == ' ') {return n;}    
    ++n;
  }
  n = 0;
  while (n < limit) {
    if ((src[-n] & 0xc0) != 0x80) {return n;}    
    ++n;
  }
  return 0;
}




int ForwardscanToSpace(const char* src, int limit) {
  int n = 0;
  limit = minint(limit, kMaxSpaceScan);
  while (n < limit) {
    if (src[n] == ' ') {return n + 1;}    
    ++n;
  }
  n = 0;
  while (n < limit) {
    if ((src[n] & 0xc0) != 0x80) {return n;}    
    ++n;
  }
  return 0;
}


















int CountPredictedBytes(const char* isrc, int src_len, int* hash, int* tbl) {
  int p_count = 0;
  const uint8* src = reinterpret_cast<const uint8*>(isrc);
  const uint8* srclimit = src + src_len;
  int local_hash = *hash;

  while (src < srclimit) {
    int c = src[0];
    int incr = 1;

    
    if (c < 0xc0) {
      
      
    } else if ((c & 0xe0) == 0xc0) {
      
      c = (c << 8) | src[1];
      incr = 2;
    } else if ((c & 0xf0) == 0xe0) {
      
      c = (c << 16) | (src[1] << 8) | src[2];
      incr = 3;
    } else {
      
      c = (c << 24) | (src[1] << 16) | (src[2] << 8) | src[3];
      incr = 4;
    }
    src += incr;

    int p = tbl[local_hash];            
    tbl[local_hash] = c;                
    if (c == p) {
      p_count += incr;                  
    }

    local_hash = ((local_hash << 4) ^ c) & 0xfff;
  }
  *hash = local_hash;
  return p_count;
}





int CountSpaces4(const char* src, int src_len) {
  int s_count = 0;
  for (int i = 0; i < (src_len & ~3); i += 4) {
    s_count += (src[i] == ' ');
    s_count += (src[i+1] == ' ');
    s_count += (src[i+2] == ' ');
    s_count += (src[i+3] == ' ');
  }
  return s_count;
}














int CheapRepWordsInplace(char* isrc, int src_len, int* hash, int* tbl) {
  const uint8* src = reinterpret_cast<const uint8*>(isrc);
  const uint8* srclimit = src + src_len;
  char* dst = isrc;
  int local_hash = *hash;
  char* word_dst = dst;           
  int good_predict_bytes = 0;
  int word_length_bytes = 0;

  while (src < srclimit) {
    int c = src[0];
    int incr = 1;
    *dst++ = c;

    if (c == ' ') {
      if ((good_predict_bytes * 2) > word_length_bytes) {
        
        dst = word_dst;
        if (FLAGS_cld_showme) {
          
          
          
          if ((isrc < (dst - 2)) && (dst[-2] != '.')) {
            *dst++ = '.';
            *dst++ = ' ';
          }
        }
      }
      word_dst = dst;              
      good_predict_bytes = 0;
      word_length_bytes = 0;
    }

    
    if (c < 0xc0) {
      
      
    } else if ((c & 0xe0) == 0xc0) {
      
      *dst++ = src[1];
      c = (c << 8) | src[1];
      incr = 2;
    } else if ((c & 0xf0) == 0xe0) {
      
      *dst++ = src[1];
      *dst++ = src[2];
      c = (c << 16) | (src[1] << 8) | src[2];
      incr = 3;
    } else {
      
      *dst++ = src[1];
      *dst++ = src[2];
      *dst++ = src[3];
      c = (c << 24) | (src[1] << 16) | (src[2] << 8) | src[3];
      incr = 4;
    }
    src += incr;
    word_length_bytes += incr;

    int p = tbl[local_hash];            
    tbl[local_hash] = c;                
    if (c == p) {
      good_predict_bytes += incr;       
    }

    local_hash = ((local_hash << 4) ^ c) & 0xfff;
  }

  *hash = local_hash;

  if ((dst - isrc) < (src_len - 3)) {
    
    dst[0] = ' ';
    dst[1] = ' ';
    dst[2] = ' ';
    dst[3] = '\0';
  } else  if ((dst - isrc) < src_len) {
    
    dst[0] = ' ';
  }

  return static_cast<int>(dst - isrc);
}




int CheapRepWordsInplaceOverwrite(char* isrc, int src_len, int* hash, int* tbl) {
  const uint8* src = reinterpret_cast<const uint8*>(isrc);
  const uint8* srclimit = src + src_len;
  char* dst = isrc;
  int local_hash = *hash;
  char* word_dst = dst;           
  int good_predict_bytes = 0;
  int word_length_bytes = 0;

  while (src < srclimit) {
    int c = src[0];
    int incr = 1;
    *dst++ = c;

    if (c == ' ') {
      if ((good_predict_bytes * 2) > word_length_bytes) {
        
        for (char* p = word_dst; p < dst - 1; ++p) {*p = '.';}
      }
      word_dst = dst;              
      good_predict_bytes = 0;
      word_length_bytes = 0;
    }

    
    if (c < 0xc0) {
      
      
    } else if ((c & 0xe0) == 0xc0) {
      
      *dst++ = src[1];
      c = (c << 8) | src[1];
      incr = 2;
    } else if ((c & 0xf0) == 0xe0) {
      
      *dst++ = src[1];
      *dst++ = src[2];
      c = (c << 16) | (src[1] << 8) | src[2];
      incr = 3;
    } else {
      
      *dst++ = src[1];
      *dst++ = src[2];
      *dst++ = src[3];
      c = (c << 24) | (src[1] << 16) | (src[2] << 8) | src[3];
      incr = 4;
    }
    src += incr;
    word_length_bytes += incr;

    int p = tbl[local_hash];            
    tbl[local_hash] = c;                
    if (c == p) {
      good_predict_bytes += incr;       
    }

    local_hash = ((local_hash << 4) ^ c) & 0xfff;
  }

  *hash = local_hash;

  if ((dst - isrc) < (src_len - 3)) {
    
    dst[0] = ' ';
    dst[1] = ' ';
    dst[2] = ' ';
    dst[3] = '\0';
  } else  if ((dst - isrc) < src_len) {
    
    dst[0] = ' ';
  }

  return static_cast<int>(dst - isrc);
}














int CheapSqueezeInplace(char* isrc,
                                            int src_len,
                                            int ichunksize) {
  char* src = isrc;
  char* dst = src;
  char* srclimit = src + src_len;
  bool skipping = false;

  int hash = 0;
  
  int* predict_tbl = new int[kPredictionTableSize];
  memset(predict_tbl, 0, kPredictionTableSize * sizeof(predict_tbl[0]));

  int chunksize = ichunksize;
  if (chunksize == 0) {chunksize = kChunksizeDefault;}
  int space_thresh = (chunksize * kSpacesThreshPercent) / 100;
  int predict_thresh = (chunksize * kPredictThreshPercent) / 100;

  while (src < srclimit) {
    int remaining_bytes = srclimit - src;
    int len = minint(chunksize, remaining_bytes);
    
    
    
    while ((src[len] & 0xc0) == 0x80) {++len;}  

    int space_n = CountSpaces4(src, len);
    int predb_n = CountPredictedBytes(src, len, &hash, predict_tbl);
    if ((space_n >= space_thresh) || (predb_n >= predict_thresh)) {
      
      if (!skipping) {
        
        int n = BackscanToSpace(dst, static_cast<int>(dst - isrc));
        dst -= n;
        if (dst == isrc) {
          
          *dst++ = ' ';
        }
        if (FLAGS_cld_showme) {
          
          *dst++ = static_cast<unsigned char>(0xe2);
          *dst++ = static_cast<unsigned char>(0x96);
          *dst++ = static_cast<unsigned char>(0xa0);
          *dst++ = ' ';
        }
        skipping = true;
      }
    } else {
      
      if (skipping) {
        
        int n = ForwardscanToSpace(src, len);
        src += n;
        remaining_bytes -= n;   
        len -= n;
        skipping = false;
      }
      
      if (len > 0) {
        memmove(dst, src, len);
        dst += len;
      }
    }
    src += len;
  }

  if ((dst - isrc) < (src_len - 3)) {
    
    dst[0] = ' ';
    dst[1] = ' ';
    dst[2] = ' ';
    dst[3] = '\0';
  } else   if ((dst - isrc) < src_len) {
    
    dst[0] = ' ';
  }

  
  delete[] predict_tbl;
  return static_cast<int>(dst - isrc);
}



int CheapSqueezeInplaceOverwrite(char* isrc,
                                            int src_len,
                                            int ichunksize) {
  char* src = isrc;
  char* dst = src;
  char* srclimit = src + src_len;
  bool skipping = false;

  int hash = 0;
  
  int* predict_tbl = new int[kPredictionTableSize];
  memset(predict_tbl, 0, kPredictionTableSize * sizeof(predict_tbl[0]));

  int chunksize = ichunksize;
  if (chunksize == 0) {chunksize = kChunksizeDefault;}
  int space_thresh = (chunksize * kSpacesThreshPercent) / 100;
  int predict_thresh = (chunksize * kPredictThreshPercent) / 100;

  
  ++src;
  ++dst;
  while (src < srclimit) {
    int remaining_bytes = srclimit - src;
    int len = minint(chunksize, remaining_bytes);
    
    
    
    while ((src[len] & 0xc0) == 0x80) {++len;}  

    int space_n = CountSpaces4(src, len);
    int predb_n = CountPredictedBytes(src, len, &hash, predict_tbl);
    if ((space_n >= space_thresh) || (predb_n >= predict_thresh)) {
      
      if (!skipping) {
        
        int n = BackscanToSpace(dst, static_cast<int>(dst - isrc));
        
        for (char* p = dst - n; p < dst; ++p) {*p = '.';}
        skipping = true;
      }
      
      for (char* p = dst; p < dst + len; ++p) {*p = '.';}
      dst[len - 1] = ' ';    
    } else {
      
      if (skipping) {
        
        int n = ForwardscanToSpace(src, len);
        
        for (char* p = dst; p < dst + n - 1; ++p) {*p = '.';}
        skipping = false;
      }
    }
    dst += len;
    src += len;
  }

  if ((dst - isrc) < (src_len - 3)) {
    
    dst[0] = ' ';
    dst[1] = ' ';
    dst[2] = ' ';
    dst[3] = '\0';
  } else   if ((dst - isrc) < src_len) {
    
    dst[0] = ' ';
  }

  
  delete[] predict_tbl;
  return static_cast<int>(dst - isrc);
}











bool CheapSqueezeTriggerTest(const char* src, int src_len, int testsize) {
  
  if (src_len < testsize) {return false;}
  int space_thresh = (testsize * kSpacesTriggerPercent) / 100;
  int predict_thresh = (testsize * kPredictTriggerPercent) / 100;
  int hash = 0;
  
  int* predict_tbl = new int[kPredictionTableSize];
  memset(predict_tbl, 0, kPredictionTableSize * sizeof(predict_tbl[0]));

  bool retval = false;
  if ((CountSpaces4(src, testsize) >= space_thresh) ||
      (CountPredictedBytes(src, testsize, &hash, predict_tbl) >=
       predict_thresh)) {
    retval = true;
  }
  
  delete[] predict_tbl;
  return retval;
}





void RemoveExtendedLanguages(DocTote* doc_tote) {
  
}

static const int kMinReliableKeepPercent = 41;  


static const int kGoodFirstT3MinBytes = 24;         












void RemoveUnreliableLanguages(DocTote* doc_tote,
                               bool FLAGS_cld2_html, bool FLAGS_cld2_quiet) {
  
  
  int total_bytes = 0;
  for (int sub = 0; sub < doc_tote->MaxSize(); ++sub) {
    int plang = doc_tote->Key(sub);
    if (plang == DocTote::kUnusedKey) {continue;}               

    Language lang = static_cast<Language>(plang);
    int bytes = doc_tote->Value(sub);
    int reli = doc_tote->Reliability(sub);
    if (bytes == 0) {continue;}                     
    total_bytes += bytes;

    
    int reliable_percent = reli / bytes;
    if (reliable_percent >= kMinReliableKeepPercent) {continue;}   

    
    Language altlang = UNKNOWN_LANGUAGE;
    if (lang <= HAWAIIAN) {altlang = kClosestAltLanguage[lang];}
    if (altlang == UNKNOWN_LANGUAGE) {continue;}    

    
    int altsub = doc_tote->Find(altlang);
    if (altsub < 0) {continue;}                     

    int bytes2 = doc_tote->Value(altsub);
    int reli2 = doc_tote->Reliability(altsub);
    if (bytes2 == 0) {continue;}                    

    
    int reliable_percent2 = reli2 / bytes2;

    
    int tosub = altsub;
    int fromsub = sub;
    bool into_lang = false;
    if ((reliable_percent2 < reliable_percent) ||
        ((reliable_percent2 == reliable_percent) && (lang < altlang))) {
      tosub = sub;
      fromsub = altsub;
      into_lang = true;
    }

    
    int newpercent = maxint(reliable_percent, reliable_percent2);
    newpercent = maxint(newpercent, kMinReliableKeepPercent);
    int newbytes = bytes + bytes2;
    int newreli = newpercent * newbytes;

    doc_tote->SetKey(fromsub, DocTote::kUnusedKey);
    doc_tote->SetScore(fromsub, 0);
    doc_tote->SetReliability(fromsub, 0);
    doc_tote->SetScore(tosub, newbytes);
    doc_tote->SetReliability(tosub, newreli);

    
    if (FLAGS_cld2_html && (newbytes >= 10) &&
        !FLAGS_cld2_quiet) {
      if (into_lang) {
        fprintf(stderr, "{Unreli %s.%dR,%dB => %s} ",
                LanguageCode(altlang), reliable_percent2, bytes2,
                LanguageCode(lang));
      } else {
        fprintf(stderr, "{Unreli %s.%dR,%dB => %s} ",
                LanguageCode(lang), reliable_percent, bytes,
                LanguageCode(altlang));
      }
    }
  }


  
  for (int sub = 0; sub < doc_tote->MaxSize(); ++sub) {
    int plang = doc_tote->Key(sub);
    if (plang == DocTote::kUnusedKey) {continue;}               

    Language lang = static_cast<Language>(plang);
    int bytes = doc_tote->Value(sub);
    int reli = doc_tote->Reliability(sub);
    if (bytes == 0) {continue;}                     

    
    int reliable_percent = reli / bytes;
    if (reliable_percent >= kMinReliableKeepPercent) {  
       continue;                                        
    }

    
    doc_tote->SetKey(sub, DocTote::kUnusedKey);
    doc_tote->SetScore(sub, 0);
    doc_tote->SetReliability(sub, 0);

    
    if (FLAGS_cld2_html && (bytes >= 10) &&
        !FLAGS_cld2_quiet) {
      fprintf(stderr, "{Unreli %s.%dR,%dB} ",
              LanguageCode(lang), reliable_percent, bytes);
    }
  }

  
}



void MoveLang1ToLang2(Language lang1, Language lang2,
                      int lang1_sub, int lang2_sub,
                      DocTote* doc_tote,
                      ResultChunkVector* resultchunkvector) {
  
  int sum = doc_tote->Value(lang2_sub) + doc_tote->Value(lang1_sub);
  doc_tote->SetValue(lang2_sub, sum);
  sum = doc_tote->Score(lang2_sub) + doc_tote->Score(lang1_sub);
  doc_tote->SetScore(lang2_sub, sum);
  sum = doc_tote->Reliability(lang2_sub) + doc_tote->Reliability(lang1_sub);
  doc_tote->SetReliability(lang2_sub, sum);

  
  doc_tote->SetKey(lang1_sub, DocTote::kUnusedKey);
  doc_tote->SetScore(lang1_sub, 0);
  doc_tote->SetReliability(lang1_sub, 0);

  
  if (resultchunkvector == NULL) {return;}

  int k = 0;
  uint16 prior_lang = UNKNOWN_LANGUAGE;
  for (int i = 0; i < static_cast<int>(resultchunkvector->size()); ++i) {
    ResultChunk* rc = &(*resultchunkvector)[i];
    if (rc->lang1 == lang1) {
      
      rc->lang1 = lang2;
    }
    
    if ((rc->lang1 == prior_lang) && (k > 0)) {
      
      ResultChunk* prior_rc = &(*resultchunkvector)[k - 1];
      prior_rc->bytes += rc->bytes;
      
    } else {
      
      (*resultchunkvector)[k] = (*resultchunkvector)[i];
      
      ++k;
    }
    prior_lang = rc->lang1;
  }
  resultchunkvector->resize(k);
}





void RefineScoredClosePairs(DocTote* doc_tote,
                            ResultChunkVector* resultchunkvector,
                            bool FLAGS_cld2_html, bool FLAGS_cld2_quiet) {
  for (int sub = 0; sub < doc_tote->MaxSize(); ++sub) {
    int close_packedlang = doc_tote->Key(sub);
    int subscr = LanguageCloseSet(static_cast<Language>(close_packedlang));
    if (subscr == 0) {continue;}

    
    

    
    for (int sub2 = sub + 1; sub2 < doc_tote->MaxSize(); ++sub2) {
      if (LanguageCloseSet(static_cast<Language>(doc_tote->Key(sub2))) == subscr) {
        
        int close_packedlang2 = doc_tote->Key(sub2);

        
        int from_sub, to_sub;
        Language from_lang, to_lang;
        if (doc_tote->Value(sub) < doc_tote->Value(sub2)) {
          from_sub = sub;
          to_sub = sub2;
          from_lang = static_cast<Language>(close_packedlang);
          to_lang = static_cast<Language>(close_packedlang2);
        } else {
          from_sub = sub2;
          to_sub = sub;
          from_lang = static_cast<Language>(close_packedlang2);
          to_lang = static_cast<Language>(close_packedlang);
        }

        if ((FLAGS_cld2_html || FLAGS_dbgscore) && !FLAGS_cld2_quiet) {
          
          int val = doc_tote->Value(from_sub);           
          int reli = doc_tote->Reliability(from_sub);
          int reliable_percent = reli / (val ? val : 1);  
          fprintf(stderr, "{CloseLangPair: %s.%dR,%dB => %s}<br>\n",
                  LanguageCode(from_lang),
                  reliable_percent,
                  doc_tote->Value(from_sub),
                  LanguageCode(to_lang));
        }
        MoveLang1ToLang2(from_lang, to_lang, from_sub, to_sub,
                         doc_tote, resultchunkvector);
        break;    
      }
    }     
  }   
}


void ApplyAllLanguageHints(Tote* chunk_tote, int tote_grams,
                        uint8* lang_hint_boost) {
}


void PrintHtmlEscapedText(FILE* f, const char* txt, int len) {
   string temp(txt, len);
   fprintf(f, "%s", GetHtmlEscapedText(temp).c_str());
}

void PrintLang(FILE* f, Tote* chunk_tote,
              Language cur_lang, bool cur_unreliable,
              Language prior_lang, bool prior_unreliable) {
  if (cur_lang == prior_lang) {
    fprintf(f, "[]");
  } else {
    fprintf(f, "[%s%s]", LanguageCode(cur_lang), cur_unreliable ? "*" : "");
  }
}


void PrintTopLang(Language top_lang) {
  if ((top_lang == prior_lang) && (top_lang != UNKNOWN_LANGUAGE)) {
    fprintf(stderr, "[] ");
  } else {
    fprintf(stderr, "[%s] ", LanguageName(top_lang));
    prior_lang = top_lang;
  }
}

void PrintTopLangSpeculative(Language top_lang) {
  fprintf(stderr, "<span style=\"color:#%06X;\">", 0xa0a0a0);
  if ((top_lang == prior_lang) && (top_lang != UNKNOWN_LANGUAGE)) {
    fprintf(stderr, "[] ");
  } else {
    fprintf(stderr, "[%s] ", LanguageName(top_lang));
    prior_lang = top_lang;
  }
  fprintf(stderr, "</span>\n");
}

void PrintLangs(FILE* f, const Language* language3, const int* percent3,
                const int* text_bytes, const bool* is_reliable) {
  fprintf(f, "<br>&nbsp;&nbsp;Initial_Languages ");
  if (language3[0] != UNKNOWN_LANGUAGE) {
    fprintf(f, "%s%s(%d%%)  ",
            LanguageName(language3[0]),
            *is_reliable ? "" : "*",
            percent3[0]);
  }
  if (language3[1] != UNKNOWN_LANGUAGE) {
    fprintf(f, "%s(%d%%)  ", LanguageName(language3[1]), percent3[1]);
  }
  if (language3[2] != UNKNOWN_LANGUAGE) {
    fprintf(f, "%s(%d%%)  ", LanguageName(language3[2]), percent3[2]);
  }
  fprintf(f, "%d bytes \n", *text_bytes);

  fprintf(f, "<br>\n");
}



double GetNormalizedScore(Language lang, ULScript ulscript,
                          int bytecount, int score) {
  if (bytecount <= 0) {return 0.0;}
  return (score << 10) / bytecount;
}


void ExtractLangEtc(DocTote* doc_tote, int total_text_bytes,
                    int* reliable_percent3, Language* language3, int* percent3,
                    double*  normalized_score3,
                    int* text_bytes, bool* is_reliable) {
  reliable_percent3[0] = 0;
  reliable_percent3[1] = 0;
  reliable_percent3[2] = 0;
  language3[0] = UNKNOWN_LANGUAGE;
  language3[1] = UNKNOWN_LANGUAGE;
  language3[2] = UNKNOWN_LANGUAGE;
  percent3[0] = 0;
  percent3[1] = 0;
  percent3[2] = 0;
  normalized_score3[0] = 0.0;
  normalized_score3[1] = 0.0;
  normalized_score3[2] = 0.0;

  *text_bytes = total_text_bytes;
  *is_reliable = false;

  int bytecount1 = 0;
  int bytecount2 = 0;
  int bytecount3 = 0;

  int lang1 = doc_tote->Key(0);
  if ((lang1 != DocTote::kUnusedKey) && (lang1 != UNKNOWN_LANGUAGE)) {
    
    language3[0] = static_cast<Language>(lang1);
    bytecount1 = doc_tote->Value(0);
    int reli1 = doc_tote->Reliability(0);
    reliable_percent3[0] = reli1 / (bytecount1 ? bytecount1 : 1);  
    normalized_score3[0] = GetNormalizedScore(language3[0],
                                                  ULScript_Common,
                                                  bytecount1,
                                                  doc_tote->Score(0));
  }

  int lang2 = doc_tote->Key(1);
  if ((lang2 != DocTote::kUnusedKey) && (lang2 != UNKNOWN_LANGUAGE)) {
    language3[1] = static_cast<Language>(lang2);
    bytecount2 = doc_tote->Value(1);
    int reli2 = doc_tote->Reliability(1);
    reliable_percent3[1] = reli2 / (bytecount2 ? bytecount2 : 1);  
    normalized_score3[1] = GetNormalizedScore(language3[1],
                                                  ULScript_Common,
                                                  bytecount2,
                                                  doc_tote->Score(1));
  }

  int lang3 = doc_tote->Key(2);
  if ((lang3 != DocTote::kUnusedKey) && (lang3 != UNKNOWN_LANGUAGE)) {
    language3[2] = static_cast<Language>(lang3);
    bytecount3 = doc_tote->Value(2);
    int reli3 = doc_tote->Reliability(2);
    reliable_percent3[2] = reli3 / (bytecount3 ? bytecount3 : 1);  
    normalized_score3[2] = GetNormalizedScore(language3[2],
                                                  ULScript_Common,
                                                  bytecount3,
                                                  doc_tote->Score(2));
  }

  
  int total_bytecount12 = bytecount1 + bytecount2;
  int total_bytecount123 = total_bytecount12 + bytecount3;
  if (total_text_bytes < total_bytecount123) {
    total_text_bytes = total_bytecount123;
    *text_bytes = total_text_bytes;
  }

  
  int total_text_bytes_div = maxint(1, total_text_bytes);    
  percent3[0] = (bytecount1 * 100) / total_text_bytes_div;
  percent3[1] = (total_bytecount12 * 100) / total_text_bytes_div;
  percent3[2] = (total_bytecount123 * 100) / total_text_bytes_div;
  percent3[2] -= percent3[1];
  percent3[1] -= percent3[0];

  
  
  if (percent3[1] < percent3[2]) {
    ++percent3[1];
    --percent3[2];
  }
  if (percent3[0] < percent3[1]) {
    ++percent3[0];
    --percent3[1];
  }

  *text_bytes = total_text_bytes;

  if ((lang1 != DocTote::kUnusedKey) && (lang1 != UNKNOWN_LANGUAGE)) {
    
    
    int bytecount = doc_tote->Value(0);
    int reli = doc_tote->Reliability(0);
    int reliable_percent = reli / (bytecount ? bytecount : 1);  
    *is_reliable = (reliable_percent >= kMinReliableKeepPercent);
  } else {
    
    
    *is_reliable = false;
  }

  
  int ignore_percent = 100 - (percent3[0] + percent3[1] + percent3[2]);
  if ((ignore_percent > kIgnoreMaxPercent)) {
    *is_reliable = false;
  }
}

bool IsFIGS(Language lang) {
  if (lang == FRENCH) {return true;}
  if (lang == ITALIAN) {return true;}
  if (lang == GERMAN) {return true;}
  if (lang == SPANISH) {return true;}
  return false;
}

bool IsEFIGS(Language lang) {
  if (lang == ENGLISH) {return true;}
  if (lang == FRENCH) {return true;}
  if (lang == ITALIAN) {return true;}
  if (lang == GERMAN) {return true;}
  if (lang == SPANISH) {return true;}
  return false;
}



static const int kGoodSecondT1T2MinBytes = 15;        
static const int kGoodSecondT3MinBytes = 128;         







void CalcSummaryLang(DocTote* doc_tote, int total_text_bytes,
                     const int* reliable_percent3,
                     const Language* language3,
                     const int* percent3,
                     Language* summary_lang, bool* is_reliable,
                     bool FLAGS_cld2_html, bool FLAGS_cld2_quiet) {
  
  int slot_count = 3;
  int active_slot[3] = {0, 1, 2};

  int ignore_percent = 0;
  int return_percent = percent3[0];   
  *summary_lang = language3[0];
  *is_reliable = true;
  if (percent3[0] < kKeepMinPercent) {*is_reliable = false;}

  
  for (int i = 0; i < 3; ++i) {
    if (language3[i] == TG_UNKNOWN_LANGUAGE) {
      ignore_percent += percent3[i];
      
      for (int j=i+1; j < 3; ++j) {
        active_slot[j - 1] = active_slot[j];
      }
      -- slot_count;
      
      
      return_percent = (percent3[0] * 100) / (101 - ignore_percent);
      *summary_lang = language3[active_slot[0]];
      if (percent3[active_slot[0]] < kKeepMinPercent) {*is_reliable = false;}
    }
  }


  
  
  
  int second_bytes = (total_text_bytes * percent3[active_slot[1]]) / 100;
  
  int minbytesneeded = kGoodSecondT1T2MinBytes;
  int plang_second = PerScriptNumber(ULScript_Latin, language3[active_slot[1]]);

  if ((language3[active_slot[0]] == ENGLISH) &&
      (language3[active_slot[1]] != ENGLISH) &&
      (language3[active_slot[1]] != UNKNOWN_LANGUAGE) &&
      (percent3[active_slot[1]] >= kNonEnBoilerplateMinPercent) &&
      (second_bytes >= minbytesneeded)) {
    ignore_percent += percent3[active_slot[0]];
    return_percent = (percent3[active_slot[1]] * 100) / (101 - ignore_percent);
    *summary_lang = language3[active_slot[1]];
    if (percent3[active_slot[1]] < kKeepMinPercent) {*is_reliable = false;}

  
  
  
  } else if (IsFIGS(language3[active_slot[0]]) &&
             !IsEFIGS(language3[active_slot[1]]) &&
             (language3[active_slot[1]] != UNKNOWN_LANGUAGE) &&
             (percent3[active_slot[1]] >= kNonFIGSBoilerplateMinPercent) &&
             (second_bytes >= minbytesneeded)) {
    ignore_percent += percent3[active_slot[0]];
    return_percent = (percent3[active_slot[1]] * 100) / (101 - ignore_percent);
    *summary_lang = language3[active_slot[1]];
    if (percent3[active_slot[1]] < kKeepMinPercent) {*is_reliable = false;}

  
  
  } else  if ((language3[active_slot[1]] == ENGLISH) &&
              (language3[active_slot[0]] != ENGLISH)) {
    ignore_percent += percent3[active_slot[1]];
    return_percent = (percent3[active_slot[0]] * 100) / (101 - ignore_percent);
  } else  if (IsFIGS(language3[active_slot[1]]) &&
              !IsEFIGS(language3[active_slot[0]])) {
    ignore_percent += percent3[active_slot[1]];
    return_percent = (percent3[active_slot[0]] * 100) / (101 - ignore_percent);
  }

  
  if ((return_percent < kGoodFirstMinPercent)) {
    if (FLAGS_cld2_html && !FLAGS_cld2_quiet) {
      fprintf(stderr, "{Unreli %s %d%% percent too small} ",
              LanguageCode(*summary_lang), return_percent);
    }
    *summary_lang = UNKNOWN_LANGUAGE;
    *is_reliable = false;
  }

  
  if ((return_percent < kGoodFirstReliableMinPercent)) {
    *is_reliable = false;
  }

  
  ignore_percent = 100 - (percent3[0] + percent3[1] + percent3[2]);
  if ((ignore_percent > kIgnoreMaxPercent)) {
    *is_reliable = false;
  }

  
  if (slot_count == 0) {
    if (FLAGS_cld2_html && !FLAGS_cld2_quiet) {
      fprintf(stderr, "{Unreli %s no languages left} ",
              LanguageCode(*summary_lang));
    }
    *summary_lang = UNKNOWN_LANGUAGE;
    *is_reliable = false;
  }
}

void AddLangPriorBoost(Language lang, uint32 langprob,
                       ScoringContext* scoringcontext) {
  
  

  if (IsLatnLanguage(lang)) {
    LangBoosts* langprior_boost = &scoringcontext->langprior_boost.latn;
    int n = langprior_boost->n;
    langprior_boost->langprob[n] = langprob;
    langprior_boost->n = langprior_boost->wrap(n + 1);
  }

  if (IsOthrLanguage(lang)) {
    LangBoosts* langprior_boost = &scoringcontext->langprior_boost.othr;
    int n = langprior_boost->n;
    langprior_boost->langprob[n] = langprob;
    langprior_boost->n = langprior_boost->wrap(n + 1);
  }

}

void AddOneWhack(Language whacker_lang, Language whackee_lang,
                 ScoringContext* scoringcontext) {
  uint32 langprob = MakeLangProb(whackee_lang, 1);
  
  if (IsLatnLanguage(whacker_lang) && IsLatnLanguage(whackee_lang)) {
    LangBoosts* langprior_whack = &scoringcontext->langprior_whack.latn;
    int n = langprior_whack->n;
    langprior_whack->langprob[n] = langprob;
    langprior_whack->n = langprior_whack->wrap(n + 1);
  }
  if (IsOthrLanguage(whacker_lang) && IsOthrLanguage(whackee_lang)) {
    LangBoosts* langprior_whack = &scoringcontext->langprior_whack.othr;
    int n = langprior_whack->n;
    langprior_whack->langprob[n] = langprob;
    langprior_whack->n = langprior_whack->wrap(n + 1);
 }
}

void AddCloseLangWhack(Language lang, ScoringContext* scoringcontext) {
  
  
  if (lang == CLD2::CHINESE) {
    AddOneWhack(lang, CLD2::CHINESE_T, scoringcontext);
    return;
  }
  if (lang == CLD2::CHINESE_T) {
    AddOneWhack(lang, CLD2::CHINESE, scoringcontext);
    return;
  }

  int base_lang_set = LanguageCloseSet(lang);
  if (base_lang_set == 0) {return;}
  
  for (int i = 0; i < kLanguageToPLangSize; ++i) {
    Language lang2 = static_cast<Language>(i);
    if ((base_lang_set == LanguageCloseSet(lang2)) && (lang != lang2)) {
      AddOneWhack(lang, lang2, scoringcontext);
    }
  }
}


void ApplyHints(const char* buffer,
                int buffer_length,
                bool is_plain_text,
                const CLDHints* cld_hints,
                ScoringContext* scoringcontext) {
  CLDLangPriors lang_priors;
  InitCLDLangPriors(&lang_priors);

  
  
  
  
  
  if (!is_plain_text) {
    
    int32 max_scan_bytes = FLAGS_cld_max_lang_tag_scan_kb << 10;
    string lang_tags = GetLangTagsFromHtml(buffer, buffer_length,
                                           max_scan_bytes);
    SetCLDLangTagsHint(lang_tags, &lang_priors);
    if (scoringcontext->flags_cld2_html) {
      if (!lang_tags.empty()) {
        fprintf(scoringcontext->debug_file, "<br>lang_tags '%s'<br>\n",
                lang_tags.c_str());
      }
    }
  }

  if (cld_hints != NULL) {
    if ((cld_hints->content_language_hint != NULL) &&
        (cld_hints->content_language_hint[0] != '\0')) {
      SetCLDContentLangHint(cld_hints->content_language_hint, &lang_priors);
    }

    
    if ((cld_hints->tld_hint != NULL) && (cld_hints->tld_hint[0] != '\0')) {
      SetCLDTLDHint(cld_hints->tld_hint, &lang_priors);
    }

    if (cld_hints->encoding_hint != UNKNOWN_ENCODING) {
      Encoding enc = static_cast<Encoding>(cld_hints->encoding_hint);
      SetCLDEncodingHint(enc, &lang_priors);
    }

    if (cld_hints->language_hint != UNKNOWN_LANGUAGE) {
      SetCLDLanguageHint(cld_hints->language_hint, &lang_priors);
    }
  }

  
  TrimCLDLangPriors(4, &lang_priors);

  if (scoringcontext->flags_cld2_html) {
    string print_temp = DumpCLDLangPriors(&lang_priors);
    if (!print_temp.empty()) {
      fprintf(scoringcontext->debug_file, "DumpCLDLangPriors %s<br>\n",
              print_temp.c_str());
    }
  }

  
  for (int i = 0; i < GetCLDLangPriorCount(&lang_priors); ++i) {
    Language lang = GetCLDPriorLang(lang_priors.prior[i]);
    int qprob = GetCLDPriorWeight(lang_priors.prior[i]);
    if (qprob > 0) {
      uint32 langprob = MakeLangProb(lang, qprob);
      AddLangPriorBoost(lang, langprob, scoringcontext);
    }
  }

  
  
  
  std::vector<int> close_set_count(kCloseSetSize + 1, 0);

  for (int i = 0; i < GetCLDLangPriorCount(&lang_priors); ++i) {
    Language lang = GetCLDPriorLang(lang_priors.prior[i]);
    ++close_set_count[LanguageCloseSet(lang)];
    if (lang == CLD2::CHINESE) {++close_set_count[kCloseSetSize];}
    if (lang == CLD2::CHINESE_T) {++close_set_count[kCloseSetSize];}
  }

  
  
  for (int i = 0; i < GetCLDLangPriorCount(&lang_priors); ++i) {
    Language lang = GetCLDPriorLang(lang_priors.prior[i]);
    int qprob = GetCLDPriorWeight(lang_priors.prior[i]);
    if (qprob > 0) {
      int close_set = LanguageCloseSet(lang);
      if ((close_set > 0) && (close_set_count[close_set] == 1)) {
        AddCloseLangWhack(lang, scoringcontext);
      }
      if (((lang == CLD2::CHINESE) || (lang == CLD2::CHINESE_T)) &&
          (close_set_count[kCloseSetSize] == 1)) {
        AddCloseLangWhack(lang, scoringcontext);
      }
    }
  }






}




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
                        bool* is_reliable) {
  language3[0] = UNKNOWN_LANGUAGE;
  language3[1] = UNKNOWN_LANGUAGE;
  language3[2] = UNKNOWN_LANGUAGE;
  percent3[0] = 0;
  percent3[1] = 0;
  percent3[2] = 0;
  normalized_score3[0] = 0.0;
  normalized_score3[1] = 0.0;
  normalized_score3[2] = 0.0;
  if (resultchunkvector != NULL) {
    resultchunkvector->clear();
  }
  *text_bytes = 0;
  *is_reliable = false;

  if ((flags & kCLDFlagEcho) != 0) {
     string temp(buffer, buffer_length);
     if ((flags & kCLDFlagHtml) != 0) {
        fprintf(stderr, "CLD2[%d] '%s'<br>\n",
                buffer_length, GetHtmlEscapedText(temp).c_str());
     } else {
        fprintf(stderr, "CLD2[%d] '%s'\n",
                buffer_length, GetPlainEscapedText(temp).c_str());
     }
  }

#ifdef CLD2_DYNAMIC_MODE
  
  
  
  bool dataLoaded = isDataLoaded();
  if ((flags & kCLDFlagVerbose) != 0) {
    fprintf(stderr, "Data loaded: %s\n", (dataLoaded ? "true" : "false"));
  }
  if (!dataLoaded) {
    return UNKNOWN_LANGUAGE;
  }
#endif

  
  if (buffer_length == 0) {return UNKNOWN_LANGUAGE;}
  if (kScoringtables.quadgram_obj == NULL) {return UNKNOWN_LANGUAGE;}

  
  DocTote doc_tote;   

  
  ScoringContext scoringcontext;
  scoringcontext.debug_file = stderr;
  scoringcontext.flags_cld2_score_as_quads =
    ((flags & kCLDFlagScoreAsQuads) != 0);
  scoringcontext.flags_cld2_html = ((flags & kCLDFlagHtml) != 0);
  scoringcontext.flags_cld2_cr = ((flags & kCLDFlagCr) != 0);
  scoringcontext.flags_cld2_verbose = ((flags & kCLDFlagVerbose) != 0);
  scoringcontext.prior_chunk_lang = UNKNOWN_LANGUAGE;
  scoringcontext.ulscript = ULScript_Common;
  scoringcontext.scoringtables = &kScoringtables;
  scoringcontext.scanner = NULL;
  scoringcontext.init();            

  
  bool FLAGS_cld2_html = ((flags & kCLDFlagHtml) != 0);
  bool FLAGS_cld2_quiet = ((flags & kCLDFlagQuiet) != 0);

  ApplyHints(buffer, buffer_length, is_plain_text, cld_hints, &scoringcontext);

  
  int next_other_tote = 2;
  int tote_num = 0;

  
  Tote totes[4];                  
  bool tote_seen[4] = {false, false, false, false};
  int tote_grams[4] = {0, 0, 0, 0};     
  ULScript tote_script[4] =
    {ULScript_Latin, ULScript_Hani, ULScript_Common, ULScript_Common};

  
  ScriptScanner ss(buffer, buffer_length, is_plain_text);
  LangSpan scriptspan;

  scoringcontext.scanner = &ss;

  scriptspan.text = NULL;
  scriptspan.text_bytes = 0;
  scriptspan.offset = 0;
  scriptspan.ulscript = ULScript_Common;
  scriptspan.lang = UNKNOWN_LANGUAGE;

  int total_text_bytes = 0;
  int textlimit = FLAGS_cld_textlimit << 10;    
  if (textlimit == 0) {textlimit = 0x7fffffff;}

  int advance_by = 2;                   
  int advance_limit = textlimit >> 3;   

  int initial_word_span = kDefaultWordSpan;
  if (FLAGS_cld_forcewords) {
    initial_word_span = kReallyBigWordSpan;
  }

  
  
  
  int chunksizequads = FLAGS_cld_smoothwidth;
  chunksizequads = minint(maxint(chunksizequads, kMinChunkSizeQuads),
                               kMaxChunkSizeQuads);
  int chunksizeunis = (chunksizequads * 5) >> 1;

  
  
  int spantooshortlimit = kShortSpanThresh;

  
  prior_lang = UNKNOWN_LANGUAGE;
  prior_unreliable = false;

  
  int hash = 0;
  int* predict_tbl = new int[kPredictionTableSize];
  if (FlagRepeats(flags)) {
    memset(predict_tbl, 0, kPredictionTableSize * sizeof(predict_tbl[0]));
  }



  
  while (ss.GetOneScriptSpanLower(&scriptspan)) {
    ULScript ulscript = scriptspan.ulscript;

    
    if (FlagSqueeze(flags)) {
      
      int newlen;
      int chunksize = 0;    
      if (resultchunkvector != NULL) {
         newlen = CheapSqueezeInplaceOverwrite(scriptspan.text,
                                               scriptspan.text_bytes,
                                               chunksize);
      } else {
         newlen = CheapSqueezeInplace(scriptspan.text, scriptspan.text_bytes,
                                      chunksize);
      }
      scriptspan.text_bytes = newlen;
    } else {
      
      if (((kCheapSqueezeTestThresh >> 1) < scriptspan.text_bytes) &&
          !FlagFinish(flags)) {
        
        
        
        
        

        if (CheapSqueezeTriggerTest(scriptspan.text,
                                      scriptspan.text_bytes,
                                      kCheapSqueezeTestLen)) {
          
          if (FLAGS_cld2_html || FLAGS_dbgscore) {
            fprintf(stderr,
                    "<br>---text_bytes[%d] Recursive(Squeeze)---<br><br>\n",
                    total_text_bytes);
          }
          
          delete[] predict_tbl;

          return DetectLanguageSummaryV2(
                            buffer,
                            buffer_length,
                            is_plain_text,
                            cld_hints,
                            allow_extended_lang,
                            flags | kCLDFlagSqueeze,
                            plus_one,
                            language3,
                            percent3,
                            normalized_score3,
                            resultchunkvector,
                            text_bytes,
                            is_reliable);
        }
      }
    }

    
    if (FlagRepeats(flags)) {
      
      int newlen;
      if (resultchunkvector != NULL) {
        newlen = CheapRepWordsInplaceOverwrite(scriptspan.text,
                                               scriptspan.text_bytes,
                                               &hash, predict_tbl);
      } else {
        newlen = CheapRepWordsInplace(scriptspan.text, scriptspan.text_bytes,
                                      &hash, predict_tbl);
      }
      scriptspan.text_bytes = newlen;
    }

    
    
    
    
    
    
    

    
    
    
    
    
    

    scoringcontext.ulscript = scriptspan.ulscript;
    

    ScoreOneScriptSpan(scriptspan,
                       &scoringcontext,
                       &doc_tote,
                       resultchunkvector);

    total_text_bytes += scriptspan.text_bytes;
  }     

  
  delete[] predict_tbl;

  if (FLAGS_cld2_html && !FLAGS_cld2_quiet) {
    
    if (!scoringcontext.flags_cld2_cr) {fprintf(stderr, "<br>\n");}
    doc_tote.Dump(stderr);
  }


  
  if (!allow_extended_lang) {
    RemoveExtendedLanguages(&doc_tote);
  }

  
  
  RefineScoredClosePairs(&doc_tote, resultchunkvector,
                         FLAGS_cld2_html, FLAGS_cld2_quiet);


  
  
  int reliable_percent3[3];

  
  doc_tote.Sort(3);

  ExtractLangEtc(&doc_tote, total_text_bytes,
                 reliable_percent3, language3, percent3, normalized_score3,
                 text_bytes, is_reliable);

  bool have_good_answer = false;
  if (FlagFinish(flags)) {
    
    have_good_answer = true;
  } else if (total_text_bytes <= kShortTextThresh) {
    
    have_good_answer = true;
  } else if (*is_reliable &&
             (percent3[0] >= kGoodLang1Percent)) {
    have_good_answer = true;
  } else if (*is_reliable &&
             ((percent3[0] + percent3[1]) >= kGoodLang1and2Percent)) {
    have_good_answer = true;
  }


  if (have_good_answer) {
    

    
    RemoveUnreliableLanguages(&doc_tote, FLAGS_cld2_html, FLAGS_cld2_quiet);

    
    doc_tote.Sort(3);
    ExtractLangEtc(&doc_tote, total_text_bytes,
                   reliable_percent3, language3, percent3, normalized_score3,
                   text_bytes, is_reliable);



    Language summary_lang;
    CalcSummaryLang(&doc_tote, total_text_bytes,
                    reliable_percent3, language3, percent3,
                    &summary_lang, is_reliable,
                    FLAGS_cld2_html, FLAGS_cld2_quiet);

    if (FLAGS_cld2_html && !FLAGS_cld2_quiet) {
      for (int i = 0; i < 3; ++i) {
        if (language3[i] != UNKNOWN_LANGUAGE) {
          fprintf(stderr, "%s.%dR(%d%%) ",
                  LanguageCode(language3[i]),
                  reliable_percent3[i],
                  percent3[i]);
        }
      }

      fprintf(stderr, "%d bytes ", total_text_bytes);
      fprintf(stderr, "= %s%c ",
              LanguageName(summary_lang), *is_reliable ? ' ' : '*');
      fprintf(stderr, "<br><br>\n");
    }

    
    if (FLAGS_cld2_html && FLAGS_cld2_quiet) {
      fprintf(stderr, "&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp; ");
      for (int i = 0; i < 3; ++i) {
        if (language3[i] != UNKNOWN_LANGUAGE) {
          fprintf(stderr, "&nbsp;&nbsp;%s %d%% ",
                  LanguageCode(language3[i]),
                  percent3[i]);
        }
      }
      fprintf(stderr, "= %s%c ",
              LanguageName(summary_lang), *is_reliable ? ' ' : '*');
      fprintf(stderr, "<br>\n");
    }

    return summary_lang;
  }

  
  if ((FLAGS_cld2_html || FLAGS_dbgscore) && !FLAGS_cld2_quiet) {
    
    PrintLangs(stderr, language3, percent3, text_bytes, is_reliable);
  }

  
  
  Language new_plus_one = UNKNOWN_LANGUAGE;

  if (total_text_bytes < kShortTextThresh) {
      
      if (FLAGS_cld2_html || FLAGS_dbgscore) {
        fprintf(stderr, "&nbsp;&nbsp;---text_bytes[%d] "
                "Recursive(Top40/Rep/Short/Words)---<br><br>\n",
                total_text_bytes);
      }
      return DetectLanguageSummaryV2(
                        buffer,
                        buffer_length,
                        is_plain_text,
                        cld_hints,
                        allow_extended_lang,
                        flags | kCLDFlagTop40 | kCLDFlagRepeats |
                          kCLDFlagShort | kCLDFlagUseWords | kCLDFlagFinish,
                        new_plus_one,
                        language3,
                        percent3,
                        normalized_score3,
                        resultchunkvector,
                        text_bytes,
                        is_reliable);
  }

  
  if (FLAGS_cld2_html || FLAGS_dbgscore) {
    fprintf(stderr,
            "&nbsp;&nbsp;---text_bytes[%d] Recursive(Top40/Rep)---<br><br>\n",
            total_text_bytes);
  }
  return DetectLanguageSummaryV2(
                        buffer,
                        buffer_length,
                        is_plain_text,
                        cld_hints,
                        allow_extended_lang,
                        flags | kCLDFlagTop40 | kCLDFlagRepeats |
                          kCLDFlagFinish,
                        new_plus_one,
                        language3,
                        percent3,
                        normalized_score3,
                        resultchunkvector,
                        text_bytes,
                        is_reliable);
}



static char temp_detectlanguageversion[32];



const char* DetectLanguageVersion() {
  if (kScoringtables.quadgram_obj == NULL) {return "";}
  sprintf(temp_detectlanguageversion,
          "V2.0 - %u", kScoringtables.quadgram_obj->kCLDTableBuildDate);
  return temp_detectlanguageversion;
}


}       
