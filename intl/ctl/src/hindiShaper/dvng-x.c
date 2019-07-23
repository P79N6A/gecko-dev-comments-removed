








































#include <glib.h>
#include <string.h>

#include "pango-engine.h"
#include "pango-coverage.h"

#define G_N_ELEMENTS(arr) (sizeof (arr) / sizeof ((arr)[0]))
#define MAX_CLUSTER_CHRS 256
#define MAX_GLYPHS       256
#define GLYPH_COMBINING  256





























 
































#define _NP    1L
#define _UP    (1L<<1)
#define _IV    (1L<<2)
#define _CN    (1L<<3)
#define _CK    (1L<<4)
#define _RC    (1L<<5)
#define _NM    (1L<<6)
#define _IM    (1L<<7)
#define _HL    (1L<<8)
#define _NK    (1L<<9)
#define _VD    (1L<<10)
#define _HD    (1L<<11)
#define _II_M  (1L<<12)
#define _EY_M  (1L<<13)
#define _AI_M  (1L<<14)
#define _OW1_M (1L<<15)
#define _OW2_M (1L<<16)
#define _MS    (1L<<17)
#define _AYE_M (1L<<18)
#define _EE_M  (1L<<19)
#define _AWE_M (1L<<20)
#define _O_M   (1L<<21)
#define _RM    (_II_M|_EY_M|_AI_M|_OW1_M|_OW2_M|_AYE_M|_EE_M|_AWE_M|_O_M)


#define __ND 0




#define __UP     1
#define __NP     2
#define __IV     3
#define __CN     4
#define __CK     5
#define __RC     6
#define __NM     7
#define __RM     8
#define __IM     9
#define __HL    10
#define __NK    11
#define __VD    12
#define __HD    13




#define St0      0
#define St1      1
#define St2      2
#define St3      3
#define St4      4
#define St5      5
#define St6      6
#define St7      7
#define St8      8
#define St9      9
#define St10    10
#define St11    11
#define St12    12
#define St13    13
#define St14    14
#define St15    15
#define St16    16
#define St17    17
#define St18    18
#define St19    19
#define St20    20

#define _ND     0
#define _NC     1
#define _UC     (1<<1)
#define _BC     (1<<2)
#define _SC     (1<<3)
#define _AV     (1<<4)
#define _BV     (1<<5)
#define _TN     (1<<6)
#define _AD     (1<<7)
#define _BD     (1<<8)
#define _AM     (1<<9)

#define MAP_SIZE     243
#define MAX_STATE     21
#define MAX_DEVA_TYPE 14
#define MAX_CORE_CONS  6

#define SCRIPT_ENGINE_NAME  "DvngScriptEngineX"
#define PANGO_RENDER_TYPE_X "PangoliteRenderX"

#define ucs2dvng(ch) (gunichar2)((gunichar2)(ch) - 0x0900)

typedef guint16 PangoliteXSubfont;
#define PANGO_MOZ_MAKE_GLYPH(index) ((guint32)0 | (index))



static PangoliteEngineRange dvng_ranges[] = {
  { 0x0901, 0x0903, "*" },
  { 0x0905, 0x0939, "*" },
  { 0x093c, 0x094d, "*" },
  { 0x0950, 0x0954, "*" },
  { 0x0958, 0x0970, "*" }, 
};

static PangoliteEngineInfo script_engines[] = {
  {
    SCRIPT_ENGINE_NAME,
    PANGO_ENGINE_TYPE_SHAPE,
    PANGO_RENDER_TYPE_X,
    dvng_ranges, 
    G_N_ELEMENTS(dvng_ranges)
  }
};



typedef struct _DvngFontInfo DvngFontInfo;



typedef enum {
  DVNG_FONT_NONE,
  DVNG_FONT_SUN
} DvngFontType;



typedef struct {
  const gunichar       ISCII[MAX_CORE_CONS];
  const unsigned short ISFOC[MAX_CORE_CONS];
} DvngGlyphEntry;



struct _DvngFontInfo
{
  DvngFontType  type;
  PangoliteXSubfont subfont;
};

typedef long DvngCls;
typedef int  StateType;




static const DvngCls DvngChrClsTbl[128] = {


     _ND,     _UP,     _UP,     _NP,    _ND,     _IV,   _IV, _IV,
            _IV,     _IV,     _IV,     _IV,    _IV,     _IV,   _IV, _IV,
     _IV,     _IV,     _IV,     _IV,    _IV, _CK|_MS,   _CK, _CK,
            _CN,     _CN,     _CN,     _CN,    _CK,     _CN,   _CN,_CN|_MS,
 _CN|_MS, _CK|_MS, _CK|_MS,     _CN,    _CN,     _CN,   _CN, _CN,
            _CN,     _CN,     _CN, _CK|_MS,    _CN,     _CN,   _CN, _CN,
     _RC,     _CN,     _CN,     _CN,    _CN,     _CN,   _CN, _CN,
            _CN, _CN|_MS,     _ND,     _ND,    _NK,     _VD,   _NM, _IM,
   _II_M,     _NM,     _NM,     _NM,    _NM,  _AYE_M, _EE_M, _EY_M,
          _AI_M,  _AWE_M,    _O_M,  _OW1_M, _OW2_M,     _HL,   _ND, _ND,
     _ND,     _VD,     _VD,     _VD,    _VD,     _ND,   _ND, _ND,
            _CN,     _CN,     _CN,     _CN,    _CN,     _CN,   _CN, _CN,
     _IV,     _IV,     _NM,     _NM,    _ND,     _ND,   _HD, _HD,
            _HD,     _HD,     _HD,     _HD,    _HD,     _HD,   _HD, _HD,
     _ND,     _ND,     _ND,     _ND,    _ND,     _ND,   _ND, _ND,
            _ND,     _ND,     _ND,     _ND,    _ND,     _ND,   _ND, _ND,
};




static const gint DvngChrTypeTbl[128] = {


 __ND, __UP, __UP, __NP, __ND, __IV, __IV, __IV,
        __IV, __IV, __IV, __IV, __IV, __IV, __IV, __IV,
 __IV, __IV, __IV, __IV, __IV, __CK, __CK, __CK,
        __CN, __CN, __CN, __CN, __CK, __CN, __CN, __CN,
 __CN, __CK, __CK, __CN, __CN, __CN, __CN, __CN,
        __CN, __CN, __CN, __CK, __CN, __CN, __CN, __CN,
 __RC, __CN, __CN, __CN, __CN, __CN, __CN, __CN,
        __CN, __CN, __ND, __ND, __NK, __VD, __NM, __IM,
 __RM, __NM, __NM, __NM, __NM, __RM, __RM, __RM,
        __RM, __RM, __RM, __RM, __RM, __HL, __ND, __ND,
 __ND, __VD, __VD, __VD, __VD, __ND, __ND, __ND,
        __CN, __CN, __CN, __CN, __CN, __CN, __CN, __CN,
 __IV, __IV, __NM, __NM, __ND, __ND, __HD, __HD,
        __HD, __HD, __HD, __HD, __HD, __HD, __HD, __HD,
 __ND, __ND, __ND, __ND, __ND, __ND, __ND, __ND,
        __ND, __ND, __ND, __ND, __ND, __ND, __ND, __ND,
};




static const gint DvngComposeTbl[14][14] = {
  
   { 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,}, 
   { 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,}, 
   { 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,}, 
   { 0,  1,  1,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,}, 
   { 0,  1,  1,  0,  0,  0,  0,  1,  1,  1,  1,  0,  0,  0,}, 
   { 0,  1,  1,  0,  0,  0,  0,  1,  1,  1,  1,  1,  0,  0,}, 
   { 0,  1,  1,  0,  0,  0,  0,  1,  1,  1,  1,  0,  0,  0,}, 
   { 0,  1,  1,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,}, 
   { 0,  1,  1,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,}, 
   { 0,  1,  1,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,}, 
   { 0,  0,  0,  0,  1,  1,  1,  0,  0,  0,  0,  0,  0,  0,}, 
   { 0,  1,  1,  0,  0,  0,  0,  1,  1,  1,  1,  0,  0,  0,}, 
   { 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,}, 
   { 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,}, 
};

StateType DvngStateTbl[MAX_STATE][MAX_DEVA_TYPE] = {

  

                                           
 {  St11, St1, St1,  St2,  St4,  St4, St12,  St1,  St1,
    St1,  St1, St1,  St1, St11, },
                                           
 {  St1,  St1,  St1,  St1,  St1,  St1,  St1,  St1,  St1,
    St1,  St1,  St1,  St1,  St1, },
                                           
 {  St2,  St3,  St3,  St2,  St2,  St2,  St2,  St2,  St2,
    St2,  St2,  St2,  St2,  St2, },
                                           
 {  St3,  St3,  St3,  St3,  St3,  St3,  St3,  St3,  St3,
    St3,  St3,  St3,  St3,  St3, },
                                           
 {  St4,  St8,  St8,  St4,  St4,  St4,  St4,  St6,  St6,
    St9,  St5,  St4,  St4,  St4, },
                                           
 {  St5,  St5,  St5,  St5,  St4,  St4,  St4,  St5,  St5,
    St5,  St5,  St5,  St5,  St5, },
                                           
 {  St6,  St7,  St7,  St6,  St6,  St6,  St6,  St6,  St6,
    St6,  St6,  St6,  St6,  St6, },
                                           
 {  St7,  St7,  St7,  St7,  St7,  St7,  St7,  St7,  St7,
    St7,  St7,  St7,  St7,  St7, },
                                           
 {  St8,  St8,  St8,  St8,  St8,  St8,  St8,  St8,  St8,
    St8,  St8,  St8,  St8,  St8, },
                                           
 {  St9, St10, St10,  St9,  St9,  St9,  St9,  St9,  St9,
    St9,  St9,  St9,  St9,  St9, },
                                          
 { St10, St10, St10, St10, St10, St10, St10, St10, St10,
   St10, St10, St10, St10, St10, },
                                          
 { St11, St11, St11, St11, St11, St11, St11, St11, St11,
   St11, St11, St11, St11, St11, },
                                          
 { St12,  St8,  St8, St12, St12, St12, St12,  St6,  St6,
    St9, St13, St12, St12, St12, },
                                          
 { St13, St13, St13, St13, St14, St14, St14, St13, St13,
   St13, St13, St13, St13, St13, },
                                           
 { St14, St18, St18, St14, St14, St14, St14, St16, St16,
   St19, St15, St14, St14, St14, },
                                           
 { St15, St15, St15, St15, St14, St14, St14, St15, St15,
   St15, St15, St15, St15, St15, },
                                           
 { St16, St17, St17, St16, St16, St16, St16, St16, St16,
   St16, St16, St16, St16, St16, },
                                           
 { St17, St17, St17, St17, St17, St17, St17, St17, St17,
   St17, St17, St17, St17, St17, },
                                           
 { St18, St18, St18, St18, St18, St18, St18, St18, St18,
   St18, St18, St18, St18, St18, },
                                           
 { St19, St20, St20, St19, St19, St19, St19, St19, St19,
   St19, St19, St19, St19, St19, },
                                           
 { St20, St20, St20, St20, St20, St20, St20, St20, St20,
   St20, St20, St20, St20, St20, },
};

int DvngRuleTbl[128] = {



  0,  1,  1,  1,  0,  1,  1,  1,
         1,  1,  1,  1,  1,  1,  1,  1,
  1,  1,  1,  1,  1,  4,  4,  4,
         4,  2,  4,  1,  4,  4,  3,  3,
  3,  3,  3,  2,  4,  4,  4,  4,
         4,  1,  4,  4,  4,  4,  4,  4,
  2,  2,  2,  2,  1,  4,  4,  3,
         5,  4,  0,  0,  2,  1,  1,  1,
  1,  1,  1,  1,  1,  1,  1,  1,
         1,  1,  1,  1,  1,  3,  0,  0,
  1,  1,  1,  1,  1,  0,  0,  0,
         1,  1,  1,  1,  1,  1,  1,  1,
  1,  1,  1,  1,  1,  1,  1,  1,
         1,  1,  1,  1,  1,  1,  1,  1,
  1,  0,  0,  0,  0,  0,  0,  0,
         0,  0,  0,  0,  0,  0,  0,  0,
};

#define IsDvngCharCls(ch, mask) (DvngChrClsTbl[ucs2dvng((ch))] & (mask))

#define IsComposible(ch1, ch2) (DvngComposeTbl[DvngChrTypeTbl[ucs2dvng((ch1))]][DvngChrTypeTbl[ucs2dvng((ch2))]])

#define GetDvngRuleCt(ch) (DvngRuleTbl[ucs2dvng((ch))])

#define IsKern(gid) ((gid >= 0xF830 && gid <= 0xF83E) ? TRUE : FALSE)

#define IsMatraAtStem(gid) (((gid >= 0xF811 && gid <= 0xF813) || \
                             (gid >= 0xF81E && gid <= 0xF82F) || \
                             (gid >= 0x0962 && gid <= 0x0963) || \
                             (gid >= 0x0941 && gid <= 0x0948) || \
                             (gid == 0x093C) || (gid == 0x094D)) ? TRUE : FALSE)

#define SetClusterState(st, ch) DvngStateTbl[(st)][DvngChrTypeTbl[ucs2dvng((ch))]]

#define MAP_SIZE 243




static const DvngGlyphEntry sunGlyphTbl[MAP_SIZE] = {
  
  { {0x0901,0x0},                       {0x0901,0x0} },
  { {0x0902,0x0},                       {0x0902,0x0} },
  { {0x0903,0x0},                       {0x0903,0x0} },

  
  { {0x0905,0x0},                       {0x0905,0x0} },
  { {0x0906,0x0},                       {0x0906,0x0} },
  { {0x0907,0x0},                       {0x0907,0x0} },
  { {0x0908,0x0},                       {0x0908,0x0} },
  { {0x0909,0x0},                       {0x0909,0x0} },
  { {0x090a,0x0},                       {0x090a,0xF830,0x0} },
  { {0x090b,0x0},                       {0x090b,0xF831,0x0} },
  { {0x0960,0x0},                       {0x0960,0xF831,0x0} },
  { {0x090c,0x0},                       {0x090c,0xF83D,0x0} },
  { {0x0961,0x0},                       {0x0961,0xF83D,0x0} },
  { {0x090d,0x0},                       {0x090d,0x0} },
  { {0x090e,0x0},                       {0x090e,0x0} },
  { {0x090f,0x0},                       {0x090f,0x0} },
  { {0x0910,0x0},                       {0x0910,0x0} },
  { {0x0911,0x0},                       {0x0911,0x0} },
  { {0x0912,0x0},                       {0x0912,0x0} },
  { {0x0913,0x0},                       {0x0913,0x0} },
  { {0x0914,0x0},                       {0x0914,0x0} },

  
  { {0x093e,0x0},                       {0x093e,0x0} },
  { {0x093f,0x0},                       {0x093f,0x0} },
  { {0x0940,0x0},                       {0x0940,0x0} },
  { {0x0941,0x0},                       {0x0941,0x0} },
  { {0x0942,0x0},                       {0x0942,0x0} },
  { {0x0943,0x0},                       {0x0943,0x0} },
  { {0x0944,0x0},                       {0x0944,0x0} },
  { {0x0945,0x0},                       {0x0945,0x0} },
  { {0x0946,0x0},                       {0x0946,0x0} },
  { {0x0947,0x0},                       {0x0947,0x0} },
  { {0x0948,0x0},                       {0x0948,0x0} },
  { {0x0949,0x0},                       {0x0949,0x0} },
  { {0x094a,0x0},                       {0x094a,0x0} },
  { {0x094b,0x0},                       {0x094b,0x0} },
  { {0x094c,0x0},                       {0x094c,0x0} },
  { {0x0962,0x0},                       {0x0962,0x0} },
  { {0x0963,0x0},                       {0x0963,0x0} },

  
  
  { {0x0915,0x0},                       {0x0915,0xF832,0x0} },
  
  { {0x0915,0x093c,0x0},                {0x0958,0xF832,0x0} },
  
  { {0x0915,0x094d,0x0},                {0xF7C1,0x0} },
  
  { {0x0915,0x093c,0x094d,0x0},         {0xF7C2,0x0} },

  { {0x0915,0x094d,0x0915,0x0},         {0xF845,0xF832,0x0} },

  
  { {0x0915,0x094d,0x0924,0x0},         {0xF7C4,0xF832,0x0} },
  
  { {0x0915,0x094d,0x0930,0x0},         {0xF7C3,0xF832,0x0} },
  { {0x0915,0x094d,0x0930,0x094d,0x0},  {0xF7C3,0x094d,0xF832,0x0} },
  
  { {0x0915,0x094d,0x0937,0x0},         {0xF7C5,0x093E,0x0} }, 
  
  { {0x0915,0x094d,0x0937,0x094d,0x0},  {0xF7C5,0x0} },

  
  { {0x0916,0x0},                       {0x0916,0x0} },
  { {0x0916,0x094d,0x0},                {0xF7C6,0x0} },
  { {0x0916,0x093c,0x0},                {0x0959,0x0} },
  { {0x0916,0x093c,0x094d,0x0},         {0xF7C7,0x0} },
  { {0x0916,0x094d,0x0930,0x0},         {0xF7C8,0x093E,0x0} },
  { {0x0916,0x094d,0x0930,0x094d,0x0},  {0xF7C8,0x0} },

  
  { {0x0917,0x0},                       {0x0917,0x0} },
  { {0x0917,0x094d,0x0},                {0xF7C9,0x0} },
  { {0x0917,0x093c,0x0},                {0x095a,0x0} },
  { {0x0917,0x093c,0x094d,0x0},         {0xF7CA,0x0} },
  { {0x0917,0x094d,0x0930,0x0},         {0xF7CB,0x093E,0x0} },
  { {0x0917,0x094d,0x0930,0x094d,0x0},  {0xF7CB,0x0} },

  
  { {0x0918,0x0},                       {0x0918,0x0} },
  { {0x0918,0x094d,0x0},                {0xF7CC,0x0} },
  { {0x0918,0x094d,0x0930,0x0},         {0xF7CD,0x093E,0x0} },
  { {0x0918,0x094d,0x0930,0x094d,0x0},  {0xF7CD,0x0} },

  
  { {0x0919,0x0},                       {0x0919,0xF833,0x0} },

  

  
  { {0x091a,0x0},                       {0x091a,0x0} },
  
  { {0x091a,0x094d,0x0},                {0xF7CE,0x0} },
  
  { {0x091a,0x094d,0x0930,0x0},         {0xF7CF,0x093E,0x0} },
  
  { {0x091a,0x094d,0x0930,0x094d,0x0},  {0xF7CF,0x0} },

  
  { {0x091b,0x0},                       {0x091b,0xF834,0x0} },

  
  

  
  
  { {0x091c,0x0},                       {0x091c,0x0} },
  
  { {0x091c,0x094d,0x0},                {0xF7D0,0x0} },
  
  { {0x091c,0x093c,0x0},                {0xF7D1,0x093E,0x0} },
  
  { {0x091c,0x093c,0x094d,0x0 },        {0xF7D1,0x0} },
  
  { {0x091c,0x094d,0x0930,0x0},         {0xF7D2,0x093E,0x0} },
  
  { {0x091c,0x094d,0x0930,0x094d,0x0},  {0xF7D2,0x0} },

  
  
  { {0x091c,0x094d,0x091e,0x0},         {0xF7D3,0x093E,0x0} },
  
  { {0x091c,0x094d,0x091e,0x094d,0x0},  {0xF7D3,0x0} },

  
  
  { {0x091d,0x0},                       {0x091d,0x0} },
  
  { {0x091d,0x094d,0x0},                {0xF7D4,0x0} },
  
  { {0x091d,0x094d,0x0930,0x0},         {0xF7D5,0x093E,0x0} },
  
  { {0x091d,0x094d,0x0930,0x094d,0x0},  {0xF7D5,0x0} },

  
  
  { {0x091e,0x0},                       {0x091e,0x0} },
  
  { {0x091e,0x094d,0x0},                {0xF7D6,0x0} },

  { {0x091e,0x094d,0x091c,0x0},         {0xF846,0x0} },

  
  
  { {0x091f,0x0},                       {0x091F,0xF835,0x0} },

  { {0x091f,0x094d,0x0},                {0x091f,0x094d,0xF835,0x0} },

  
  { {0x091f,0x094d,0x091f,0x0},         {0xF7D7,0xF835,0x0} },
  
  { {0x091f,0x094d,0x0920,0x0},         {0xF7D8,0xF835,0x0} },

  
  
  { {0x0920,0x0},                       {0x0920,0xF836,0x0} },

  { {0x0920,0x094d,0x0},                {0x0920,0x094d,0xF836,0x0} },

  
  { {0x0920,0x094d,0x0920,0x0},         {0xF7D9,0xF836,0x0} },

  
  
  { {0x0921,0x0},                       {0x0921,0xF837,0x0} },

  { {0x0921,0x094d,0x0},                {0x0921,0x094d,0xF837,0x0} },
 
  
  
  { {0x0921,0x093c,0x0},                {0x095c,0xF837,0x0} },

  
  { {0x0921,0x093c,0x094d,0x0},         {0x095c,0x094d,0xF837,0x0} },

  { {0x0921,0x094d,0x0917,0x0},         {0xF847,0xF837,0x0} },

  
  
  { {0x0921,0x094d,0x0921,0x0},         {0xF7DA,0xF837,0x0} },

  
  
  { {0x0921,0x094d,0x0922,0x0},         {0xF7DB,0xF837,0x0} },

  
  
  { {0x0922,0x0},                       {0x0922,0xF838,0x0} },

  
  
  { {0x0922,0x093c,0x0},                {0x095d,0xF838,0x0} },
  { {0x0922,0x093c,0x094d,0x0},         {0x095d,0x094d,0xF838,0x0} },

  { {0x0922,0x094d,0x0},                {0x0922,0x094d,0xF838,0x0} },

  
  
  { {0x0923,0x0},                       {0x0923,0x0} },
  
  { {0x0923,0x094d,0x0},                {0xF7DC,0x0} },

  
  
  { {0x0924,0x0},                       {0x0924,0x0} },
  
  { {0x0924,0x094d,0x0},                {0xF7DD,0x0} },
  
  { {0x0924,0x094d,0x0930,0x0},         {0xF7DE,0x093E,0x0} },
  
  { {0x0924,0x094d,0x0930,0x094d,0x0},  {0xF7DE,0x0} },
  
  { {0x0924,0x094d,0x0924,0x0},         {0xF7DF,0x093E,0x0} },
  
  { {0x0924,0x094d,0x0924,0x094d,0x0},  {0xF7DF,0x0} },

  
  
  { {0x0925,0x0},                       {0x0925,0x0} },
  
  { {0x0925,0x094d,0x0},                {0xF7E0,0x0} },
  
  { {0x0925,0x094d,0x0930,0x0},         {0xF7E1,0x093E,0x0} },
  
  { {0x0925,0x094d,0x0930,0x094d,0x0},  {0xF7E1,0x0} },

  
  
  { {0x0926,0x0},                       {0x0926,0xF839,0x0} },
  
  { {0x0926,0x094d,0x0},                {0x0926,0x094d,0xF839,0x0} },

  
  
  { {0x0926,0x0943,0x0},                {0xF7E2,0xF839,0x0} },

  
  
  { {0x0926,0x094d,0x0930,0x0},         {0xF7E3,0xF839,0x0} },
  { {0x0926,0x094d,0x0930,0x094d,0x0},  {0xF7E3,0x094d,0xF839,0x0} },

  { {0x0926,0x094d,0x0918,0x0},         {0xF848,0xF839,0x0} },

  
  
  { {0x0926,0x094d,0x0926,0x0},         {0xF7E4,0xF839,0x0} },

  
  
  { {0x0926,0x094d,0x0927,0x0},         {0xF7E5,0xF839,0x0} },

  { {0x0926,0x094d,0x092c,0x0},         {0xF849,0xF839,0x0} },

  { {0x0926,0x094d,0x092d,0x0},         {0xF844,0xF839,0x0} },

  
  
  { {0x0926,0x094d,0x092e,0x0},         {0xF7E6,0xF839,0x0} },

  
  
  { {0x0926,0x094d,0x092f,0x0},         {0xF7E7,0xF839,0x0} },

  
  
  { {0x0926,0x094d,0x0935,0x0},         {0xF7E8,0xF839,0x0} },

  
  
  { {0x0927,0x0},                       {0x0927,0x0} },
  
  { {0x0927,0x094d,0x0},                {0xF7E9,0x0} },
  
  { {0x0927,0x094d,0x0930,0x0},         {0xF7EA,0x093E,0x0} },
  
  { {0x0927,0x094d,0x0930,0x094d,0x0},  {0xF7EA,0x0} },

  
  
  { {0x0928,0x0},                       {0x0928,0x0} },
  
  { {0x0928,0x094d,0x0},                {0xF7EB,0x0} },
  
  { {0x0928,0x094d,0x0930,0x0},         {0xF7EC,0x093E,0x0} },
  
  { {0x0928,0x094d,0x0930,0x094d,0x0},  {0xF7EC,0x0} },
  
  { {0x0928,0x094d,0x0928,0x0},         {0xF7ED,0x093E,0x0} },
  
  { {0x0928,0x094d,0x0928,0x094d,0x0},  {0xF7ED,0x0} },

  { {0x0929,0x0},                       {0x0929,0x0} },

  
  
  { {0x092a,0x0},                       {0x092a,0x0} },
  
  { {0x092a,0x094d,0x0},                {0xF7EE,0x0} },
  
  { {0x092a,0x094d,0x0930,0x0},         {0xF7EF,0x093E,0x0} },
  
  { {0x092a,0x094d,0x0930,0x094d,0x0},  {0xF7EF,0x0} },

  
  
  { {0x092b,0x0 },                      {0x092b,0xF832,0x0} },
  
  { {0x092b,0x094d,0x0},                {0xF7F0,0x0} },
  
  { {0x092b,0x093c,0x0},                {0x095e,0xF832,0x0} },
  
  { {0x092b,0x093c,0x094d,0x0},         {0xF7F1,0x0} },
  
  { {0x092b,0x094d,0x0930,0x0},         {0xF7F5,0xF832,0x0} },
  
  { {0x092b,0x094d,0x0930,0x094d,0x0},  {0xF7F5,0xF832,0x094d,0x0} },

  
  
  { {0x092c,0x0},                       {0x092c,0x0} },
  
  { {0x092c,0x094d,0x0},                {0xF7F6,0x0} },
  
  { {0x092c,0x094d,0x0930,0x0},         {0xF7F7,0x093E,0x0} },
  
  { {0x092c,0x094d,0x0930,0x094d,0x0},  {0xF7F7,0x0} },

  
  
  { {0x092d,0x0},                       {0x092d,0x0} },
  
  { {0x092d,0x094d,0x0},                {0xF7F8,0x0} },
  
  { {0x092d,0x094d,0x0930,0x0},         {0xF7F9,0x093E,0x0} },
  
  { {0x092d,0x094d,0x0930,0x094d,0x0},  {0xF7F9,0x0} },
  
  
  { {0x092e,0x0},                       {0x092e,0x0} },
  
  { {0x092e,0x094d,0x0},                {0xF7FA,0x0} },
  
  { {0x092e,0x094d,0x0930,0x0},         {0xF7FB,0x093E,0x0} },
  
  { {0x092e,0x094d,0x0930,0x094d,0x0},  {0xF7FB,0x0} },

  
  
  { {0x092f,0x0},                       {0x092f,0x0} },
  
  { {0x092f,0x094d,0x0},                {0xF7FC,0x0} },
  
  { {0x092f,0x094d,0x0930,0x0},         {0xF7FD,0x093E,0x0} },
  
  { {0x092f,0x094d,0x0930,0x094d,0x0},  {0xF7FD,0x0} },

  
  
  { {0x0930,0x0 },                      {0x0930,0xF83A,0x0} },
  
  
  { {0x0930,0x0941,0x0},                {0xF800,0xF83B,0x0} },
  
  { {0x0930,0x0942,0x0},                {0xF801,0xF83C,0x0} },

  { {0x0931,0x0},                       {0x0931,0x0} },
  { {0x0931,0x094d,0x0},                {0xF7FF,0x0} },

  
  
  { {0x0932,0x0},                       {0x0932,0x0} },
  
  { {0x0932,0x094d,0x0},                {0xF802,0x0} },
  
  
  { {0x0933,0x0},                       {0x0933,0x0} },
  
  { {0x0933,0x094d,0x0},                {0xF803,0x0} },

  { {0x0934,0x0} ,                      {0x0934,0x0} },
  
  
  { {0x0935,0x0},                       {0x0935,0x0} },
  
  { {0x0935,0x094d,0x0},                {0xF804,0x0} },
  
  { {0x0935,0x094d,0x0930,0x0},         {0xF805,0x093E,0x0} },
  
  { {0x0935,0x094d,0x0930,0x094d,0x0},  {0xF805,0x0} },

  
  
  { {0x0936,0x0},                       {0x0936,0x0} },
  
  { {0x0936,0x094d,0x0},                {0xF806,0x0} },

  { {0x0936,0x094d,0x091a,0x0},         {0xF83F,0x0} },
  { {0x0936,0x094d,0x0928,0x0},         {0xF840,0x0} },

  
  { {0x0936,0x094d,0x0935,0x0},         {0xF807,0x093E,0x0} },
  
  { {0x0936,0x094d,0x0935,0x094d,0x0},  {0xF807,0x0} },
  
  { {0x0936,0x094d,0x0930,0x0},         {0xF808,0x093E,0x0} },
  
  { {0x0936,0x094d,0x0930,0x094d,0x0},  {0xF808,0x0} },
  
  
  { {0x0937,0x0},                       {0x0937,0x0} },
  
  { {0x0937,0x094d,0x0},                {0xF809,0x0} },

  { {0x0937,0x094d,0x091f,0x0},         {0xF841,0xF835,0x0} },
  { {0x0937,0x094d,0x0920,0x0},         {0xF842,0xF836,0x0} },

  
  
  { {0x0938,0x0},                       {0x0938,0x0} },
  
  { {0x0938,0x094d,0x0},                {0xF80A,0x0} },
  
  { {0x0938,0x094d,0x0930,0x0},         {0xF80B,0x093E,0x0} },
  
  { {0x0938,0x094d,0x0930,0x094d,0x0},  {0xF80B,0x0} },

  { {0x0938,0x094d,0x0924,0x094d,0x0930,0x0}, {0xF843,0x0} },

  
  
  { {0x0939,0x0},                       {0x0939,0xF83E,0x0} },
  
  { {0x0939,0x094d,0x0},                {0xF80C,0xF83E,0x0} },
  
  { {0x0939,0x0943,0x0},                {0xF80D,0xF83E,0x0} },
  
  { {0x0939,0x094d,0x0930,0x0},         {0xF80E,0xF83E,0x0} },
  
  { {0x0939,0x094d,0x0930,0x094d,0x0},  {0xF80E,0x094d,0xF83E,0x0} },

  { {0x0939,0x094d,0x0923,0x0},         {0xF84D,0xF83E,0x0} },
  { {0x0939,0x094d,0x0928,0x0},         {0xF84C,0xF83E,0x0} },

  
  { {0x0939,0x094d,0x092e,0x0},         {0xF80F,0xF83E,0x0} },
  
  { {0x0939,0x094d,0x092f,0x0},         {0xF810,0xF83E,0x0} },

  { {0x0939,0x094d,0x0932,0x0},         {0xF84A,0xF83E,0x0} },
  { {0x0939,0x094d,0x0935,0x0},         {0xF84B,0xF83E,0x0} },

  { {0x0958,0x0},                       {0x0958,0xF832,0x0} },
  { {0x0959,0x0},                       {0x0959,0x0} },
  { {0x095a,0x0},                       {0x095a,0x0} },
  { {0x095b,0x0},                       {0x095b,0x0} },
  { {0x095c,0x0},                       {0x095c,0xF837,0x0} },
  { {0x095d,0x0},                       {0x095d,0xF838,0x0} },
  { {0x095e,0x0},                       {0x095e,0xF832,0x0} },
  { {0x095f,0x0},                       {0x095f,0x0} },

  

  

  
  { {0x093c,0x0},                       {0x093c,0x0} },
  
  { {0x093c,0x0941,0x0},                {0xF81E,0x0} },
  
  { {0x093c,0x0942,0x0},                {0xF821,0x0} },

  
  { {0x094d,0x0},                       {0x094d,0x0} },
  
  { {0x094d,0x092f,0x0},                {0xF7FE,0x0} },
  
  { {0x094d,0x0930,0x0},                {0xF811,0x0} },
  
  { {0x094d,0x0930,0x094d,0x0},         {0x094d,0x0930,0x0930,0x0} },
  
  { {0x094d,0x0930,0x0941,0x0},         {0xF81F,0x0} },
  
  { {0x094d,0x0930,0x0942,0x0},         {0xF822,0x0} },

  
  { {0x093d,0x0},                       {0x093d,0x0} },
  { {0x0950,0x0},                       {0x0950,0x0} },
  { {0x0951,0x0},                       {0x0951,0x0} },
  { {0x0952,0x0},                       {0x0952,0x0} },
  { {0x0953,0x0},                       {0x0953,0x0} },
  { {0x0954,0x0},                       {0x0954,0x0} },
  { {0x0964,0x0},                       {0x0964,0x0} },
  { {0x0965,0x0},                       {0x0965,0x0} },
  { {0x0970,0x0},                       {0x0970,0x0} },

  
  { {0x0966,0x0},                       {0x0966,0x0} },
  { {0x0967,0x0},                       {0x0967,0x0} },
  { {0x0968,0x0},                       {0x0968,0x0} },
  { {0x0969,0x0},                       {0x0969,0x0} },
  { {0x096a,0x0},                       {0x096a,0x0} },
  { {0x096b,0x0},                       {0x096b,0x0} },
  { {0x096c,0x0},                       {0x096c,0x0} },
  { {0x096d,0x0},                       {0x096d,0x0} },
  { {0x096e,0x0},                       {0x096e,0x0} },
  { {0x096f,0x0},                       {0x096f,0x0} }
};




static DvngFontInfo *
get_font_info(const char *fontCharset)
{
  static const char *charsets[] = {
    "sun.unicode.india-0",
  };

  static const int charset_types[] = {
    DVNG_FONT_SUN
  };
  
  DvngFontInfo *font_info = g_new(DvngFontInfo, 1);
  guint        i; 

  font_info->type = DVNG_FONT_NONE;
  for (i = 0; i < G_N_ELEMENTS(charsets); i++) {
    if (strcmp(fontCharset, charsets[i]) == 0) {    
      font_info->type = (DvngFontType)charset_types[i];
      font_info->subfont = (PangoliteXSubfont)i;
      break;
    }
  }
  
  return font_info;
}

static void
add_glyph(PangoliteGlyphString *glyphs,
          gint                 clusterStart,
          PangoliteGlyph       glyph,
          gint                 combining)
{
  gint index = glyphs->num_glyphs;

  if ((clusterStart == 0) && (index != 0))
    clusterStart++;

  pangolite_glyph_string_set_size (glyphs, index + 1);  
  glyphs->glyphs[index].glyph = glyph;
  glyphs->glyphs[index].is_cluster_start = combining;
  glyphs->log_clusters[index] = clusterStart;
}

static void
GetBaseConsGlyphs(gunichar2  *cluster,
                  gint       numCoreCons,
                  PangoliteGlyph *glyphList,
                  gint       *nGlyphs)
{
  int i, j, delta, nMin, nMaxRuleCt, ruleIdx;
  gboolean  StillMatching;
  gunichar2 temp_out;
  gint      tmpCt = *nGlyphs;

  i = 0;
  while (i < numCoreCons) {
    
    nMaxRuleCt = GetDvngRuleCt(cluster[i]);    
    while (nMaxRuleCt) {
      nMin = MIN(nMaxRuleCt, numCoreCons);      
      ruleIdx = 0;

NotFound:
      j = delta = 0;
      StillMatching = FALSE;
      while (((delta < nMin) || sunGlyphTbl[ruleIdx].ISCII[j]) &&
             (ruleIdx < MAP_SIZE) ) {
        StillMatching = TRUE;
        if ((delta < nMin) && (j < MAX_CORE_CONS) &&
            (cluster[i + delta] != sunGlyphTbl[ruleIdx].ISCII[j])) {
          ruleIdx++;
          goto NotFound;
        }
        delta++;
        j++;
      }
      
      if (StillMatching) 
        break;
      else
        nMaxRuleCt--;
    }
    
    i += nMin;
    
    
    if ((StillMatching == FALSE) || (ruleIdx >= MAP_SIZE)) {
      for (j = 0; j < numCoreCons; j++)
        glyphList[tmpCt++] = PANGO_MOZ_MAKE_GLYPH(cluster[j]);
    }
    else if (((tmpCt > 0) && IsKern(glyphList[tmpCt - 1])) &&
             IsMatraAtStem(sunGlyphTbl[ruleIdx].ISFOC[0])) {
      temp_out = glyphList[tmpCt - 1];
      
      for (j=0; sunGlyphTbl[ruleIdx].ISFOC[j]; j++)
        glyphList[tmpCt - 1] = PANGO_MOZ_MAKE_GLYPH(sunGlyphTbl[ruleIdx].ISFOC[j]);
        
      glyphList[tmpCt++] = PANGO_MOZ_MAKE_GLYPH(temp_out);
    }
    else {
      for (j=0; sunGlyphTbl[ruleIdx].ISFOC[j]; j++)
        glyphList[tmpCt++] = PANGO_MOZ_MAKE_GLYPH(sunGlyphTbl[ruleIdx].ISFOC[j]);
    }
  }
  *nGlyphs = tmpCt;
}

static gint
get_adjusted_glyphs_list(DvngFontInfo *fontInfo,
                         gunichar2    *cluster,
                         gint         nChars,
                         PangoliteGlyph   *gLst,
                         StateType    *DvngClusterState)
{
  int i, k, len;
  gint      nGlyphs = 0;
  gunichar2 dummy;
  
  switch (*DvngClusterState) {
  case St1:
    if (IsDvngCharCls(cluster[0], _IM)) {
      GetBaseConsGlyphs(cluster, nChars, gLst, &nGlyphs);
      gLst[nGlyphs++] = PANGO_MOZ_MAKE_GLYPH(0xF7C0);
    }
    else {
      gLst[nGlyphs++] = PANGO_MOZ_MAKE_GLYPH(0xF7C0);
      GetBaseConsGlyphs(cluster, nChars, gLst, &nGlyphs);
    }
    break;

    case St2:
    case St3:
    case St4:
    case St5:
    case St6:
      GetBaseConsGlyphs(cluster, nChars, gLst, &nGlyphs);
      break;
      
    case St7:
      if (IsDvngCharCls(cluster[nChars - 1], _UP)) {

        if (IsDvngCharCls(cluster[nChars - 2], _RM))
          GetBaseConsGlyphs(cluster, nChars - 2, gLst, &nGlyphs);
        else
          GetBaseConsGlyphs(cluster, nChars, gLst, &nGlyphs);

        if (IsDvngCharCls(cluster[nChars - 2], _RM)) {

          if (IsDvngCharCls(cluster[nChars - 2], _II_M))
            gLst[nGlyphs++] = PANGO_MOZ_MAKE_GLYPH(0xF81B);

          else if (IsDvngCharCls(cluster[nChars - 2], _AYE_M)) {
            dummy = gLst[nGlyphs - 1];
            gLst[nGlyphs - 1] = PANGO_MOZ_MAKE_GLYPH(0xF82D);
            gLst[nGlyphs++] = PANGO_MOZ_MAKE_GLYPH(dummy);
          }

          else if (IsDvngCharCls(cluster[nChars - 2], _EE_M))
            gLst[nGlyphs++] = PANGO_MOZ_MAKE_GLYPH(0xF824);
          
          else if (IsDvngCharCls(cluster[nChars - 2], _EY_M))
            gLst[nGlyphs++] = PANGO_MOZ_MAKE_GLYPH(0xF827);

          else if (IsDvngCharCls(cluster[nChars - 2], _AI_M))
            gLst[nGlyphs++] = PANGO_MOZ_MAKE_GLYPH(0xF82A);

          else if (IsDvngCharCls(cluster[nChars - 2], _AWE_M)) {
            gLst[nGlyphs++] = PANGO_MOZ_MAKE_GLYPH(0x093E);
            gLst[nGlyphs++] = PANGO_MOZ_MAKE_GLYPH(0xF82D);
          }

          else if (IsDvngCharCls(cluster[nChars - 2], _O_M)) {
            gLst[nGlyphs++] = PANGO_MOZ_MAKE_GLYPH(0x093E);
            gLst[nGlyphs++] = PANGO_MOZ_MAKE_GLYPH(0xF824);
          }

          else if (IsDvngCharCls(cluster[nChars - 2], _OW1_M)) {
            gLst[nGlyphs++] = PANGO_MOZ_MAKE_GLYPH(0x093E);
            gLst[nGlyphs++] = PANGO_MOZ_MAKE_GLYPH(0xF827);
          }

          else if (IsDvngCharCls(cluster[nChars - 2], _OW2_M)) {
            gLst[nGlyphs++] = PANGO_MOZ_MAKE_GLYPH(0x093E);
            gLst[nGlyphs++] = PANGO_MOZ_MAKE_GLYPH(0xF82A);
          }
        }
      }
      else {
        GetBaseConsGlyphs(cluster, nChars, gLst, &nGlyphs);
      }
      break;

    case St8:
      GetBaseConsGlyphs(cluster, nChars - 1, gLst, &nGlyphs);
      if (IsKern(gLst[nGlyphs - 1])) {
        dummy = gLst[nGlyphs - 1];
        gLst[nGlyphs - 1] = PANGO_MOZ_MAKE_GLYPH(cluster[nChars - 1]);
        gLst[nGlyphs++] = PANGO_MOZ_MAKE_GLYPH(dummy);
      }
      else
        gLst[nGlyphs++] = PANGO_MOZ_MAKE_GLYPH(cluster[nChars - 1]);
      break;
      
    case St9:
      if (IsDvngCharCls(cluster[0], _MS))
        gLst[nGlyphs++] = PANGO_MOZ_MAKE_GLYPH(0x093F);
      else
        gLst[nGlyphs++] = PANGO_MOZ_MAKE_GLYPH(0xF817);        

      GetBaseConsGlyphs(cluster, nChars - 1, gLst, &nGlyphs);
      break;

  case St10:
    if (IsDvngCharCls(cluster[0], _MS))
      gLst[nGlyphs++] = PANGO_MOZ_MAKE_GLYPH(0xF814);
    else
      gLst[nGlyphs++] = PANGO_MOZ_MAKE_GLYPH(0xF818);

    GetBaseConsGlyphs(cluster, nChars - 2, gLst, &nGlyphs);
    break;
    
  case St11:
    GetBaseConsGlyphs(cluster, nChars, gLst, &nGlyphs);
    break;
    
  case St12:
    gLst[nGlyphs++] = PANGO_MOZ_MAKE_GLYPH(0x0930);
    gLst[nGlyphs++] = PANGO_MOZ_MAKE_GLYPH(0xF83A);
    break;
    
  case St13:
    gLst[nGlyphs++] = PANGO_MOZ_MAKE_GLYPH(0x0930);
    gLst[nGlyphs++] = PANGO_MOZ_MAKE_GLYPH(0x094D);
    gLst[nGlyphs++] = PANGO_MOZ_MAKE_GLYPH(0xF83A);
    break;
    
  case St14:    
    GetBaseConsGlyphs(cluster+2, nChars - 2, gLst, &nGlyphs);
    if (IsKern(gLst[nGlyphs - 1])) {
      dummy = gLst[nGlyphs - 1];
      gLst[nGlyphs - 1] = PANGO_MOZ_MAKE_GLYPH(0xF812);
      gLst[nGlyphs++] = PANGO_MOZ_MAKE_GLYPH(dummy);
    }
    else
      gLst[nGlyphs++] = PANGO_MOZ_MAKE_GLYPH(0xF812);
    break;
    
  case St15:    
    GetBaseConsGlyphs(cluster+2, nChars - 3, gLst, &nGlyphs);
    if (IsKern(gLst[nGlyphs - 1])) {      
      dummy = gLst[nGlyphs - 2];
      gLst[nGlyphs - 2] = PANGO_MOZ_MAKE_GLYPH(0xF812);
      gLst[nGlyphs++] = PANGO_MOZ_MAKE_GLYPH(0x094D);
      gLst[nGlyphs++] = PANGO_MOZ_MAKE_GLYPH(dummy);
    }
    else {
      gLst[nGlyphs++] = PANGO_MOZ_MAKE_GLYPH(0xF812);
      gLst[nGlyphs++] = PANGO_MOZ_MAKE_GLYPH(0x094D);
    }
    break;
    
  case St16:
    if (IsDvngCharCls(cluster[nChars - 1], _RM))
      GetBaseConsGlyphs(cluster+2, nChars - 3, gLst, &nGlyphs);
    else
      GetBaseConsGlyphs(cluster+2, nChars - 2, gLst, &nGlyphs);

    if (IsDvngCharCls(cluster[nChars - 1], ~(_RM))){

      if (IsKern(gLst[nGlyphs - 1])) {
        dummy = gLst[nGlyphs - 1];
        gLst[nGlyphs - 1] = PANGO_MOZ_MAKE_GLYPH(0xF812);
        gLst[nGlyphs++] = PANGO_MOZ_MAKE_GLYPH(dummy);
      }
      else
        gLst[nGlyphs++] = PANGO_MOZ_MAKE_GLYPH(0xF812);
    }
    else {

      if (IsDvngCharCls(cluster[nChars - 1], _II_M))
        gLst[nGlyphs++] = PANGO_MOZ_MAKE_GLYPH(0xF81C);

      else if (IsDvngCharCls(cluster[nChars - 1], _EY_M))
        gLst[nGlyphs++] = PANGO_MOZ_MAKE_GLYPH(0xF828);

      else if (IsDvngCharCls(cluster[nChars -1], _AI_M))
        gLst[nGlyphs++] = PANGO_MOZ_MAKE_GLYPH(0xF82B);

      else if (IsDvngCharCls(cluster[nChars - 1], _OW1_M)) {
        gLst[nGlyphs++] = PANGO_MOZ_MAKE_GLYPH(0x093E);
        gLst[nGlyphs++] = PANGO_MOZ_MAKE_GLYPH(0xF828);
      }

      else if (IsDvngCharCls(cluster[nChars - 1], _OW2_M)) {
        gLst[nGlyphs++] = PANGO_MOZ_MAKE_GLYPH(0x093E);
        gLst[nGlyphs++] = PANGO_MOZ_MAKE_GLYPH(0xF82B);
      }
    }
    break;
    
  case St17:
    if (IsDvngCharCls(cluster[nChars - 1], _UP)) {

      if (IsDvngCharCls(cluster[nChars - 2], _RM))
        GetBaseConsGlyphs(cluster+2, nChars - 4, gLst, &nGlyphs);
      else
        GetBaseConsGlyphs(cluster+2, nChars - 3, gLst, &nGlyphs);

      if (IsDvngCharCls(cluster[nChars - 2], ~(_RM))) {

        if (IsKern(gLst[nGlyphs - 1])) {
          dummy = gLst[nGlyphs - 1];
          gLst[nGlyphs - 1] = PANGO_MOZ_MAKE_GLYPH(0xF813);
          gLst[nGlyphs++] = PANGO_MOZ_MAKE_GLYPH(dummy);
        }
        else
          gLst[nGlyphs++] = PANGO_MOZ_MAKE_GLYPH(0xF813);
      }
      else {
        if (IsDvngCharCls(cluster[nChars - 2], _II_M))
          gLst[nGlyphs++] = PANGO_MOZ_MAKE_GLYPH(0xF81D);

        else if (IsDvngCharCls(cluster[nChars - 2], _EY_M))
          gLst[nGlyphs++] = PANGO_MOZ_MAKE_GLYPH(0xF829);

        else if (IsDvngCharCls(cluster[nChars - 2], _AI_M))
          gLst[nGlyphs++] = PANGO_MOZ_MAKE_GLYPH(0xF82C);

        else if (IsDvngCharCls(cluster[nChars - 2], _OW1_M)) {
          gLst[nGlyphs++] = PANGO_MOZ_MAKE_GLYPH(0x093E);
          gLst[nGlyphs++] = PANGO_MOZ_MAKE_GLYPH(0xF829);
        }
        else if (IsDvngCharCls(cluster[nChars - 2], _OW2_M)) {
          gLst[nGlyphs++] = PANGO_MOZ_MAKE_GLYPH(0x093E);
          gLst[nGlyphs++] = PANGO_MOZ_MAKE_GLYPH(0xF82C);
        }
      } 
    break;
    
  case St18:
    GetBaseConsGlyphs(cluster-2, nChars-3, gLst, &nGlyphs);
    if (IsKern(gLst[nGlyphs - 1])) {
      dummy = gLst[nGlyphs - 1];
      gLst[nGlyphs - 1] = PANGO_MOZ_MAKE_GLYPH(0xF813);
      gLst[nGlyphs++] = PANGO_MOZ_MAKE_GLYPH(dummy);
    }
    else
      gLst[nGlyphs++] = PANGO_MOZ_MAKE_GLYPH(0xF813);
    break;
    
  case St19:
    if (IsDvngCharCls(cluster[0], _MS))
      gLst[nGlyphs++] = PANGO_MOZ_MAKE_GLYPH(0xF815);
    else
      gLst[nGlyphs++] = PANGO_MOZ_MAKE_GLYPH(0xF819);

    GetBaseConsGlyphs(cluster+2, nChars-3, gLst, &nGlyphs);
    break;
    
  case St20:
    if (IsDvngCharCls(cluster[0], _MS))
      gLst[nGlyphs++] = PANGO_MOZ_MAKE_GLYPH(0xF816);
    else
      gLst[nGlyphs++] = PANGO_MOZ_MAKE_GLYPH(0xF81A);

    GetBaseConsGlyphs(cluster+2, nChars - 4, gLst, &nGlyphs);
    break;
    }
  }

  return nGlyphs;
}

static gint
get_glyphs_list(DvngFontInfo *fontInfo,
                gunichar2    *cluster,
                gint          numChars,
                PangoliteGlyph   *glyphLst,
                StateType    *clustState)
{
  PangoliteGlyph glyph;
  gint       i;

  switch (fontInfo->type) {
  case DVNG_FONT_NONE:
    for (i = 0; i < numChars; i++)
      glyphLst[i] = 0; 
    return numChars;
    
  case DVNG_FONT_SUN:
    return get_adjusted_glyphs_list(fontInfo, cluster, numChars, glyphLst, clustState);
  }
  
  return 0; 
}

static void
add_cluster(DvngFontInfo     *fontInfo,
            PangoliteGlyphString *glyphs,
            gint              clusterBeg,
            gunichar2        *cluster,
            gint              numChars,
            StateType        *clustState)
{
  PangoliteGlyph glyphsList[MAX_GLYPHS];
  gint           i, numGlyphs, ClusterStart=0;
  
  numGlyphs = get_glyphs_list(fontInfo, cluster, numChars, glyphsList, clustState);
  for (i = 0; i < numGlyphs; i++) {

    ClusterStart = (gint)GLYPH_COMBINING;
    if (i == 0)
      ClusterStart = numChars;
    add_glyph(glyphs, clusterBeg, glyphsList[i], ClusterStart);
  }
}

static const gunichar2 *
get_next_cluster(const gunichar2 *text,
                 gint             length,
                 gunichar2       *cluster,
                 gint            *numChars,
                 StateType       *clustState)
{
  const gunichar2 *p;
  gint            n_chars = 0;
  StateType       aSt = *clustState;

  p = text;
  while (p < text + length) {
    gunichar2 cur = *p;
    
    if ((n_chars == 0) ||
        ((n_chars > 0) && IsComposible(cluster[n_chars - 1], cur))) {
      cluster[n_chars++] = cur;
      aSt = SetClusterState(aSt, cur);
      p++;
    }
    else
      break;
  }
  
  *numChars = n_chars;
  *clustState = aSt;
  return p;
}

static void 
dvng_engine_shape(const char       *fontCharset,
                  const gunichar2  *text,
                  gint             length,
                  PangoliteAnalysis    *analysis,
                  PangoliteGlyphString *glyphs)
{
  DvngFontInfo    *fontInfo;
  const gunichar2 *p, *log_cluster;
  gunichar2       cluster[MAX_CLUSTER_CHRS];
  gint            num_chrs;
  StateType       aSt = St0;

  fontInfo = get_font_info(fontCharset);

  p = text;
  while (p < text + length) {
    log_cluster = p;
    aSt = St0;
    p = get_next_cluster(p, text + length - p, cluster, &num_chrs, &aSt);
    add_cluster(fontInfo, glyphs, log_cluster-text, cluster, num_chrs, &aSt);
  }
}

static PangoliteCoverage *
dvng_engine_get_coverage(const char *fontCharset,
                         const char *lang)
{
  PangoliteCoverage *result = pangolite_coverage_new ();  
  DvngFontInfo  *fontInfo = get_font_info (fontCharset);
  
  if (fontInfo->type != DVNG_FONT_NONE) {
    gunichar2 wc;
 
    for (wc = 0x901; wc <= 0x903; wc++)
      pangolite_coverage_set (result, wc, PANGO_COVERAGE_EXACT);
    for (wc = 0x905; wc <= 0x939; wc++)
      pangolite_coverage_set (result, wc, PANGO_COVERAGE_EXACT);
    for (wc = 0x93c; wc <= 0x94d; wc++)
      pangolite_coverage_set (result, wc, PANGO_COVERAGE_EXACT);
    for (wc = 0x950; wc <= 0x954; wc++)
      pangolite_coverage_set (result, wc, PANGO_COVERAGE_EXACT);
    for (wc = 0x958; wc <= 0x970; wc++)
      pangolite_coverage_set (result, wc, PANGO_COVERAGE_EXACT);
    
  }
  
  return result;
}

static PangoliteEngine *
dvng_engine_x_new ()
{
  PangoliteEngineShape *result;
  
  result = g_new (PangoliteEngineShape, 1);
  result->engine.id = SCRIPT_ENGINE_NAME;
  result->engine.type = PANGO_ENGINE_TYPE_SHAPE;
  result->engine.length = sizeof (result);
  result->script_shape = dvng_engine_shape;
  result->get_coverage = dvng_engine_get_coverage;
  return (PangoliteEngine *)result;
}








#ifdef X_MODULE_PREFIX
#define MODULE_ENTRY(func) _pangolite_dvng_x_##func
#else
#define MODULE_ENTRY(func) func
#endif



void 
MODULE_ENTRY(script_engine_list) (PangoliteEngineInfo **engines, gint *n_engines)
{
  *engines = script_engines;
  *n_engines = G_N_ELEMENTS (script_engines);
}



PangoliteEngine *
MODULE_ENTRY(script_engine_load) (const char *id)
{
  if (!strcmp (id, SCRIPT_ENGINE_NAME))
    return dvng_engine_x_new ();
  else
    return NULL;
}

void 
MODULE_ENTRY(script_engine_unload) (PangoliteEngine *engine)
{
}

