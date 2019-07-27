

















#ifndef __FTOPTION_H__
#define __FTOPTION_H__


#include <ft2build.h>


FT_BEGIN_HEADER

  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  


  
  
  
  
  
  
  


  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
#define FT_CONFIG_OPTION_SUBPIXEL_RENDERING



















#undef FT_CONFIG_OPTION_FORCE_INT64


  
  
  
  
  
  



  
  
  
  
  
  
  
  
  
#define FT_CONFIG_OPTION_INLINE_MULFIX














#define FT_CONFIG_OPTION_USE_LZW














#define FT_CONFIG_OPTION_USE_ZLIB























  
  
  
  
  
  
  
  
  
  
  
  
  
#define FT_CONFIG_OPTION_USE_BZIP2













#define FT_CONFIG_OPTION_USE_PNG












#define FT_CONFIG_OPTION_USE_HARFBUZZ













  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  




  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
#define FT_CONFIG_OPTION_POSTSCRIPT_NAMES

















#define FT_CONFIG_OPTION_ADOBE_GLYPH_LIST












#define FT_CONFIG_OPTION_MAC_FONTS




















#ifdef FT_CONFIG_OPTION_MAC_FONTS
#define FT_CONFIG_OPTION_GUESSING_EMBEDDED_RFORK
#endif


  
  
  
  
  
  
  
  
#define FT_CONFIG_OPTION_INCREMENTAL











#define FT_RENDER_POOL_SIZE  16384L


  
  
  
  
  
  
  
#define FT_MAX_MODULES  32


  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
#define FT_DEBUG_LEVEL_ERROR
#define FT_DEBUG_LEVEL_TRACE
































#define FT_DEBUG_AUTOFIT

















#define FT_DEBUG_MEMORY
















#undef FT_CONFIG_OPTION_USE_MODULE_ERRORS


  
  
  
  
  
  
  
  
  
  
  
  



  
  
  
  
  
  
  


  
  
  
  
  
  
#define TT_CONFIG_OPTION_EMBEDDED_BITMAPS














#define TT_CONFIG_OPTION_POSTSCRIPT_NAMES













#define TT_CONFIG_OPTION_SFNT_NAMES








#define TT_CONFIG_CMAP_FORMAT_0
#define TT_CONFIG_CMAP_FORMAT_2
#define TT_CONFIG_CMAP_FORMAT_4
#define TT_CONFIG_CMAP_FORMAT_6
#define TT_CONFIG_CMAP_FORMAT_8
#define TT_CONFIG_CMAP_FORMAT_10
#define TT_CONFIG_CMAP_FORMAT_12
#define TT_CONFIG_CMAP_FORMAT_13
#define TT_CONFIG_CMAP_FORMAT_14





















#define TT_CONFIG_OPTION_BYTECODE_INTERPRETER





















#define TT_CONFIG_OPTION_SUBPIXEL_HINTING




















































  
  
  
  
  
  
  
  
  
  
#define TT_CONFIG_OPTION_INTERPRETER_SWITCH


















#undef TT_CONFIG_OPTION_COMPONENT_OFFSET_SCALED


  
  
  
  
  
  
  
#define TT_CONFIG_OPTION_GX_VAR_SUPPORT







#define TT_CONFIG_OPTION_BDF

















#define T1_MAX_DICT_DEPTH  5


  
  
  
  
  
#define T1_MAX_SUBRS_CALLS  16


  
  
  
  
  
  
  
#define T1_MAX_CHARSTRINGS_OPERANDS  256


  
  
  
  
  
  
  
#undef T1_CONFIG_OPTION_NO_AFM


  
  
  
  
  
  
#undef T1_CONFIG_OPTION_NO_MM_SUPPORT


  
  
  
  
  
  
  


  
  
  
  
  
  
  
  
  
  
  
#define CFF_CONFIG_OPTION_DARKENING_PARAMETER_X1   500
#define CFF_CONFIG_OPTION_DARKENING_PARAMETER_Y1   400

#define CFF_CONFIG_OPTION_DARKENING_PARAMETER_X2  1000
#define CFF_CONFIG_OPTION_DARKENING_PARAMETER_Y2   275

#define CFF_CONFIG_OPTION_DARKENING_PARAMETER_X3  1667
#define CFF_CONFIG_OPTION_DARKENING_PARAMETER_Y3   275

#define CFF_CONFIG_OPTION_DARKENING_PARAMETER_X4  2333
#define CFF_CONFIG_OPTION_DARKENING_PARAMETER_Y4     0


  
  
  
  
  
  
  
#define CFF_CONFIG_OPTION_OLD_ENGINE
















#define AF_CONFIG_OPTION_CJK





#define AF_CONFIG_OPTION_INDIC












#define AF_CONFIG_OPTION_USE_WARPER











  



#ifdef TT_CONFIG_OPTION_BYTECODE_INTERPRETER
#define  TT_USE_BYTECODE_INTERPRETER
#undef   TT_CONFIG_OPTION_UNPATENTED_HINTING
#elif defined TT_CONFIG_OPTION_UNPATENTED_HINTING
#define  TT_USE_BYTECODE_INTERPRETER
#endif


  



#if CFF_CONFIG_OPTION_DARKENING_PARAMETER_X1 < 0   || \
    CFF_CONFIG_OPTION_DARKENING_PARAMETER_X2 < 0   || \
    CFF_CONFIG_OPTION_DARKENING_PARAMETER_X3 < 0   || \
    CFF_CONFIG_OPTION_DARKENING_PARAMETER_X4 < 0   || \
                                                      \
    CFF_CONFIG_OPTION_DARKENING_PARAMETER_Y1 < 0   || \
    CFF_CONFIG_OPTION_DARKENING_PARAMETER_Y2 < 0   || \
    CFF_CONFIG_OPTION_DARKENING_PARAMETER_Y3 < 0   || \
    CFF_CONFIG_OPTION_DARKENING_PARAMETER_Y4 < 0   || \
                                                      \
    CFF_CONFIG_OPTION_DARKENING_PARAMETER_X1 >        \
      CFF_CONFIG_OPTION_DARKENING_PARAMETER_X2     || \
    CFF_CONFIG_OPTION_DARKENING_PARAMETER_X2 >        \
      CFF_CONFIG_OPTION_DARKENING_PARAMETER_X3     || \
    CFF_CONFIG_OPTION_DARKENING_PARAMETER_X3 >        \
      CFF_CONFIG_OPTION_DARKENING_PARAMETER_X4     || \
                                                      \
    CFF_CONFIG_OPTION_DARKENING_PARAMETER_Y1 > 500 || \
    CFF_CONFIG_OPTION_DARKENING_PARAMETER_Y2 > 500 || \
    CFF_CONFIG_OPTION_DARKENING_PARAMETER_Y3 > 500 || \
    CFF_CONFIG_OPTION_DARKENING_PARAMETER_Y4 > 500
#error "Invalid CFF darkening parameters!"
#endif

FT_END_HEADER


#endif 



