




















#ifndef __AFBLUE_H__
#define __AFBLUE_H__


FT_BEGIN_HEADER


  
  
#define GET_UTF8_CHAR( ch, p )                    \
          ch = (unsigned char)*p++;               \
          if ( ch >= 0x80 )                       \
          {                                       \
            FT_UInt  len;                         \
                                                  \
                                                  \
            if ( ch < 0xE0 )                      \
            {                                     \
              len = 1;                            \
              ch &= 0x1F;                         \
            }                                     \
            else if ( ch < 0xF0 )                 \
            {                                     \
              len = 2;                            \
              ch &= 0x0F;                         \
            }                                     \
            else                                  \
            {                                     \
              len = 3;                            \
              ch &= 0x07;                         \
            }                                     \
                                                  \
            for ( ; len > 0; len-- )              \
              ch = ( ch << 6 ) | ( *p++ & 0x3F ); \
          }


  
  
  
  
  
  
  

  


#define AF_BLUE_STRING_MAX_LEN  51

  
  

  typedef enum  AF_Blue_String_
  {
    AF_BLUE_STRING_CYRILLIC_CAPITAL_TOP = 0,
    AF_BLUE_STRING_CYRILLIC_CAPITAL_BOTTOM = 17,
    AF_BLUE_STRING_CYRILLIC_SMALL = 34,
    AF_BLUE_STRING_CYRILLIC_SMALL_DESCENDER = 51,
    AF_BLUE_STRING_DEVANAGARI_BASE = 58,
    AF_BLUE_STRING_DEVANAGARI_TOP = 83,
    AF_BLUE_STRING_DEVANAGARI_HEAD = 108,
    AF_BLUE_STRING_DEVANAGARI_BOTTOM = 133,
    AF_BLUE_STRING_GREEK_CAPITAL_TOP = 140,
    AF_BLUE_STRING_GREEK_CAPITAL_BOTTOM = 155,
    AF_BLUE_STRING_GREEK_SMALL_BETA_TOP = 168,
    AF_BLUE_STRING_GREEK_SMALL = 181,
    AF_BLUE_STRING_GREEK_SMALL_DESCENDER = 198,
    AF_BLUE_STRING_HEBREW_TOP = 215,
    AF_BLUE_STRING_HEBREW_BOTTOM = 232,
    AF_BLUE_STRING_HEBREW_DESCENDER = 245,
    AF_BLUE_STRING_LATIN_CAPITAL_TOP = 256,
    AF_BLUE_STRING_LATIN_CAPITAL_BOTTOM = 265,
    AF_BLUE_STRING_LATIN_SMALL_F_TOP = 274,
    AF_BLUE_STRING_LATIN_SMALL = 282,
    AF_BLUE_STRING_LATIN_SMALL_DESCENDER = 290,
    AF_BLUE_STRING_TELUGU_TOP = 296,
    AF_BLUE_STRING_TELUGU_BOTTOM = 318,
    af_blue_1_1 = 339,
#ifdef AF_CONFIG_OPTION_CJK
    AF_BLUE_STRING_CJK_TOP = af_blue_1_1 + 1,
    AF_BLUE_STRING_CJK_BOTTOM = af_blue_1_1 + 153,
    af_blue_1_1_1 = af_blue_1_1 + 304,
#ifdef AF_CONFIG_OPTION_CJK_BLUE_HANI_VERT
    AF_BLUE_STRING_CJK_LEFT = af_blue_1_1_1 + 1,
    AF_BLUE_STRING_CJK_RIGHT = af_blue_1_1_1 + 153,
    af_blue_1_1_2 = af_blue_1_1_1 + 304,
#else
    af_blue_1_1_2 = af_blue_1_1_1 + 0,
#endif 
    af_blue_1_2 = af_blue_1_1_2 + 0,
#else
    af_blue_1_2 = af_blue_1_1 + 0,
#endif 


    AF_BLUE_STRING_MAX   

  } AF_Blue_String;


  FT_LOCAL_ARRAY( char )
  af_blue_strings[];


  
  
  
  
  
  
  

  


  
  
  
#define AF_BLUE_PROPERTY_LATIN_TOP       ( 1 << 0 )   /* must have value 1 */
#define AF_BLUE_PROPERTY_LATIN_NEUTRAL   ( 1 << 1 )
#define AF_BLUE_PROPERTY_LATIN_X_HEIGHT  ( 1 << 2 )
#define AF_BLUE_PROPERTY_LATIN_LONG      ( 1 << 3 )

#define AF_BLUE_PROPERTY_CJK_TOP    ( 1 << 0 )        /* must have value 1 */
#define AF_BLUE_PROPERTY_CJK_HORIZ  ( 1 << 1 )        /* must have value 2 */
#define AF_BLUE_PROPERTY_CJK_RIGHT  AF_BLUE_PROPERTY_CJK_TOP


#define AF_BLUE_STRINGSET_MAX_LEN  7

  
  

  typedef enum  AF_Blue_Stringset_
  {
    AF_BLUE_STRINGSET_CYRL = 0,
    AF_BLUE_STRINGSET_DEVA = 6,
    AF_BLUE_STRINGSET_GREK = 12,
    AF_BLUE_STRINGSET_HEBR = 19,
    AF_BLUE_STRINGSET_LATN = 23,
    AF_BLUE_STRINGSET_TELU = 30,
    af_blue_2_1 = 33,
#ifdef AF_CONFIG_OPTION_CJK
    AF_BLUE_STRINGSET_HANI = af_blue_2_1 + 0,
    af_blue_2_1_1 = af_blue_2_1 + 2,
#ifdef AF_CONFIG_OPTION_CJK_BLUE_HANI_VERT
    af_blue_2_1_2 = af_blue_2_1_1 + 2,
#else
    af_blue_2_1_2 = af_blue_2_1_1 + 0,
#endif 
    af_blue_2_2 = af_blue_2_1_2 + 1,
#else
    af_blue_2_2 = af_blue_2_1 + 0,
#endif 


    AF_BLUE_STRINGSET_MAX   

  } AF_Blue_Stringset;


  typedef struct  AF_Blue_StringRec_
  {
    AF_Blue_String  string;
    FT_UShort       properties;

  } AF_Blue_StringRec;


  FT_LOCAL_ARRAY( AF_Blue_StringRec )
  af_blue_stringsets[];



FT_END_HEADER


#endif 



