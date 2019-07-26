



















#ifndef I18N_ENCODINGS_CLD2_INTERNAL_GENERATED_ULSCRIPT_H__
#define I18N_ENCODINGS_CLD2_INTERNAL_GENERATED_ULSCRIPT_H__

namespace CLD2 {

typedef enum {RTypeNone = 0, RTypeOne, RTypeMany, RTypeCJK} ULScriptRType;

typedef struct {const char* s; int i;} CharIntPair;

typedef enum {
  ULScript_Common              = 0,  
  ULScript_Latin               = 1,  
  ULScript_Greek               = 2,  
  ULScript_Cyrillic            = 3,  
  ULScript_Armenian            = 4,  
  ULScript_Hebrew              = 5,  
  ULScript_Arabic              = 6,  
  ULScript_Syriac              = 7,  
  ULScript_Thaana              = 8,  
  ULScript_Devanagari          = 9,  
  ULScript_Bengali             = 10,  
  ULScript_Gurmukhi            = 11,  
  ULScript_Gujarati            = 12,  
  ULScript_Oriya               = 13,  
  ULScript_Tamil               = 14,  
  ULScript_Telugu              = 15,  
  ULScript_Kannada             = 16,  
  ULScript_Malayalam           = 17,  
  ULScript_Sinhala             = 18,  
  ULScript_Thai                = 19,  
  ULScript_Lao                 = 20,  
  ULScript_Tibetan             = 21,  
  ULScript_Myanmar             = 22,  
  ULScript_Georgian            = 23,  
  ULScript_Hani                = 24,  
  ULScript_Ethiopic            = 25,  
  ULScript_Cherokee            = 26,  
  ULScript_Canadian_Aboriginal = 27,  
  ULScript_Ogham               = 28,  
  ULScript_Runic               = 29,  
  ULScript_Khmer               = 30,  
  ULScript_Mongolian           = 31,  
  ULScript_32                  = 32,  
  ULScript_33                  = 33,  
  ULScript_Bopomofo            = 34,  
  ULScript_35                  = 35,  
  ULScript_Yi                  = 36,  
  ULScript_Old_Italic          = 37,  
  ULScript_Gothic              = 38,  
  ULScript_Deseret             = 39,  
  ULScript_Inherited           = 40,  
  ULScript_Tagalog             = 41,  
  ULScript_Hanunoo             = 42,  
  ULScript_Buhid               = 43,  
  ULScript_Tagbanwa            = 44,  
  ULScript_Limbu               = 45,  
  ULScript_Tai_Le              = 46,  
  ULScript_Linear_B            = 47,  
  ULScript_Ugaritic            = 48,  
  ULScript_Shavian             = 49,  
  ULScript_Osmanya             = 50,  
  ULScript_Cypriot             = 51,  
  ULScript_Braille             = 52,  
  ULScript_Buginese            = 53,  
  ULScript_Coptic              = 54,  
  ULScript_New_Tai_Lue         = 55,  
  ULScript_Glagolitic          = 56,  
  ULScript_Tifinagh            = 57,  
  ULScript_Syloti_Nagri        = 58,  
  ULScript_Old_Persian         = 59,  
  ULScript_Kharoshthi          = 60,  
  ULScript_Balinese            = 61,  
  ULScript_Cuneiform           = 62,  
  ULScript_Phoenician          = 63,  
  ULScript_Phags_Pa            = 64,  
  ULScript_Nko                 = 65,  
  ULScript_Sundanese           = 66,  
  ULScript_Lepcha              = 67,  
  ULScript_Ol_Chiki            = 68,  
  ULScript_Vai                 = 69,  
  ULScript_Saurashtra          = 70,  
  ULScript_Kayah_Li            = 71,  
  ULScript_Rejang              = 72,  
  ULScript_Lycian              = 73,  
  ULScript_Carian              = 74,  
  ULScript_Lydian              = 75,  
  ULScript_Cham                = 76,  
  ULScript_Tai_Tham            = 77,  
  ULScript_Tai_Viet            = 78,  
  ULScript_Avestan             = 79,  
  ULScript_Egyptian_Hieroglyphs = 80,  
  ULScript_Samaritan           = 81,  
  ULScript_Lisu                = 82,  
  ULScript_Bamum               = 83,  
  ULScript_Javanese            = 84,  
  ULScript_Meetei_Mayek        = 85,  
  ULScript_Imperial_Aramaic    = 86,  
  ULScript_Old_South_Arabian   = 87,  
  ULScript_Inscriptional_Parthian = 88,  
  ULScript_Inscriptional_Pahlavi = 89,  
  ULScript_Old_Turkic          = 90,  
  ULScript_Kaithi              = 91,  
  ULScript_Batak               = 92,  
  ULScript_Brahmi              = 93,  
  ULScript_Mandaic             = 94,  
  ULScript_Chakma              = 95,  
  ULScript_Meroitic_Cursive    = 96,  
  ULScript_Meroitic_Hieroglyphs = 97,  
  ULScript_Miao                = 98,  
  ULScript_Sharada             = 99,  
  ULScript_Sora_Sompeng        = 100,  
  ULScript_Takri               = 101,  
  NUM_ULSCRIPTS
} ULScript;

#define UNKNOWN_ULSCRIPT ULScript_Common

}  

#endif   
