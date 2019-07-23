








































#include <glib.h>
#include <string.h>

#include "pango-engine.h"
#include "pango-coverage.h"

#define G_N_ELEMENTS(arr)     (sizeof (arr) / sizeof ((arr)[0]))

#define ucs2tis(wc)	(unsigned int)((unsigned int)(wc) - 0x0E00 + 0xA0)
#define tis2uni(c)	((gunichar2)(c) - 0xA0 + 0x0E00)

#define MAX_CLUSTER_CHRS 256
#define MAX_GLYPHS       256
#define GLYPH_COMBINING  256


#define CTRL		0
#define NON			1
#define CONS		2
#define LV			3
#define FV1			4
#define FV2			5
#define FV3			6
#define BV1			7
#define BV2			8
#define BD			9
#define TONE		10
#define AD1			11
#define AD2			12
#define AD3			13
#define AV1			14
#define AV2			15
#define AV3			16

#define _ND			0
#define _NC			1
#define _UC			(1<<1)
#define _BC			(1<<2)
#define _SC			(1<<3)
#define _AV			(1<<4)
#define _BV			(1<<5)
#define _TN			(1<<6)
#define _AD			(1<<7)
#define _BD			(1<<8)
#define _AM			(1<<9)

#define NoTailCons	 _NC
#define UpTailCons	 _UC
#define BotTailCons	 _BC
#define SpltTailCons _SC
#define Cons			   (NoTailCons|UpTailCons|BotTailCons|SpltTailCons)
#define AboveVowel	 _AV
#define BelowVowel	 _BV
#define Tone			   _TN
#define AboveDiac	   _AD
#define BelowDiac	   _BD
#define SaraAm		   _AM

#define char_class(wc)		     TAC_char_class[(unsigned int)(wc)]
#define is_char_type(wc, mask) (char_type_table[ucs2tis((wc))] & (mask))

#define SCRIPT_ENGINE_NAME "ThaiScriptEngineX"
#define PANGO_RENDER_TYPE_X "PangoliteRenderX"

typedef guint16 PangoliteXSubfont;
#define PANGO_MOZ_MAKE_GLYPH(index) ((guint32)0 | (index))



static PangoliteEngineRange thai_ranges[] = {
  { 0x0e01, 0x0e5b, "*" },  
};

static PangoliteEngineInfo script_engines[] = {
  {
    SCRIPT_ENGINE_NAME,
    PANGO_ENGINE_TYPE_SHAPE,
    PANGO_RENDER_TYPE_X,
    thai_ranges, 
    G_N_ELEMENTS(thai_ranges)
  }
};





typedef struct _ThaiFontInfo ThaiFontInfo;



typedef enum {
  THAI_FONT_NONE,
  THAI_FONT_XTIS,
  THAI_FONT_TIS,
  THAI_FONT_TIS_MAC,
  THAI_FONT_TIS_WIN,
  THAI_FONT_ISO10646
} ThaiFontType;

struct _ThaiFontInfo
{
  ThaiFontType  type;
  PangoliteXSubfont subfont;
};









static const char groups[32] = {
  0, 1, 0, 0, 1, 1, 1, 1,
  1, 1, 1, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 2,
  2, 2, 2, 2, 2, 2, 1, 0
};



   
static const char group1_map[32] = {
  0, 1, 0, 0, 2, 3, 4, 5,
  6, 7, 8, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0
};



   
static const char group2_map[32] = {
  0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 1,
  2, 3, 4, 5, 6, 7, 1, 0
};

static const gint char_type_table[256] = {
  


   _ND, _ND, _ND, _ND, _ND, _ND, _ND, _ND,
         _ND, _ND, _ND, _ND, _ND, _ND, _ND, _ND,
   _ND, _ND, _ND, _ND, _ND, _ND, _ND, _ND,
         _ND, _ND, _ND, _ND, _ND, _ND, _ND, _ND,
   _ND, _ND, _ND, _ND, _ND, _ND, _ND, _ND,
         _ND, _ND, _ND, _ND, _ND, _ND, _ND, _ND,
   _ND, _ND, _ND, _ND, _ND, _ND, _ND, _ND,
         _ND, _ND, _ND, _ND, _ND, _ND, _ND, _ND,
   _ND, _ND, _ND, _ND, _ND, _ND, _ND, _ND,
         _ND, _ND, _ND, _ND, _ND, _ND, _ND, _ND,
   _ND, _ND, _ND, _ND, _ND, _ND, _ND, _ND,
         _ND, _ND, _ND, _ND, _ND, _ND, _ND, _ND,
   _ND, _ND, _ND, _ND, _ND, _ND, _ND, _ND,
         _ND, _ND, _ND, _ND, _ND, _ND, _ND, _ND,
   _ND, _ND, _ND, _ND, _ND, _ND, _ND, _ND,
         _ND, _ND, _ND, _ND, _ND, _ND, _ND, _ND,
   _ND, _ND, _ND, _ND, _ND, _ND, _ND, _ND,
         _ND, _ND, _ND, _ND, _ND, _ND, _ND, _ND,
   _ND, _ND, _ND, _ND, _ND, _ND, _ND, _ND,
         _ND, _ND, _ND, _ND, _ND, _ND, _ND, _ND,
		
   _ND, _NC, _NC, _NC, _NC, _NC, _NC, _NC,
         _NC, _NC, _NC, _NC, _NC, _SC, _BC, _BC,
   _SC, _NC, _NC, _NC, _NC, _NC, _NC, _NC,
         _NC, _NC, _NC, _UC, _NC, _UC, _NC, _UC,
   _NC, _NC, _NC, _NC, _ND, _NC, _ND, _NC,
         _NC, _NC, _NC, _NC, _UC, _NC, _NC, _ND,
   _ND, _AV, _ND, _AM, _AV, _AV, _AV, _AV,
         _BV, _BV, _BD, _ND, _ND, _ND, _ND, _ND,
   _ND, _ND, _ND, _ND, _ND, _ND, _ND, _AD,
         _TN, _TN, _TN, _TN, _AD, _AD, _AD, _ND,
   _ND, _ND, _ND, _ND, _ND, _ND, _ND, _ND,
         _ND, _ND, _ND, _ND, _ND, _ND, _ND, _ND,
};

static const gint TAC_char_class[256] = {
  


   CTRL,CTRL,CTRL,CTRL,CTRL,CTRL,CTRL,CTRL,
         CTRL,CTRL,CTRL,CTRL,CTRL,CTRL,CTRL,CTRL,
   CTRL,CTRL,CTRL,CTRL,CTRL,CTRL,CTRL,CTRL,
         CTRL,CTRL,CTRL,CTRL,CTRL,CTRL,CTRL,CTRL,
    NON, NON, NON, NON, NON, NON, NON, NON,
          NON, NON, NON, NON, NON, NON, NON, NON,
    NON, NON, NON, NON, NON, NON, NON, NON,
          NON, NON, NON, NON, NON, NON, NON, NON,
    NON, NON, NON, NON, NON, NON, NON, NON,
          NON, NON, NON, NON, NON, NON, NON, NON,
    NON, NON, NON, NON, NON, NON, NON, NON,
          NON, NON, NON, NON, NON, NON, NON, NON,
    NON, NON, NON, NON, NON, NON, NON, NON,
          NON, NON, NON, NON, NON, NON, NON, NON,
    NON, NON, NON, NON, NON, NON, NON, NON,
          NON, NON, NON, NON, NON, NON, NON,CTRL,
   CTRL,CTRL,CTRL,CTRL,CTRL,CTRL,CTRL,CTRL,
         CTRL,CTRL,CTRL,CTRL,CTRL,CTRL,CTRL,CTRL,
   CTRL,CTRL,CTRL,CTRL,CTRL,CTRL,CTRL,CTRL,
         CTRL,CTRL,CTRL,CTRL,CTRL,CTRL,CTRL,CTRL,
    NON,CONS,CONS,CONS,CONS,CONS,CONS,CONS,
         CONS,CONS,CONS,CONS,CONS,CONS,CONS,CONS,
   CONS,CONS,CONS,CONS,CONS,CONS,CONS,CONS,
         CONS,CONS,CONS,CONS,CONS,CONS,CONS,CONS,
   CONS,CONS,CONS,CONS, FV3,CONS, FV3,CONS,
         CONS,CONS,CONS,CONS,CONS,CONS,CONS, NON,
    FV1, AV2, FV1, FV1, AV1, AV3, AV2, AV3,
          BV1, BV2,  BD, NON, NON, NON, NON, NON,
     LV,  LV,  LV,  LV,  LV, FV2, NON, AD2,
         TONE,TONE,TONE,TONE, AD1, AD1, AD3, NON,
    NON, NON, NON, NON, NON, NON, NON, NON,
          NON, NON, NON, NON, NON, NON, NON,CTRL,
};

static const gchar TAC_compose_and_input_check_type_table[17][17] = {
   

  	{ 'X', 'A', 'A', 'A', 'A', 'A', 'A', 'R',
                  'R', 'R', 'R', 'R', 'R', 'R', 'R', 'R', 'R' },
        { 'X', 'A', 'A', 'A', 'S', 'S', 'A', 'R',
                  'R', 'R', 'R', 'R', 'R', 'R', 'R', 'R', 'R' },
        { 'X', 'A', 'A', 'A', 'A', 'S', 'A', 'C',
                  'C', 'C', 'C', 'C', 'C', 'C', 'C', 'C', 'C' },
        {'X', 'S', 'A', 'S', 'S', 'S', 'S', 'R',
                 'R', 'R', 'R', 'R', 'R', 'R', 'R', 'R', 'R' },
        { 'X', 'S', 'A', 'A', 'S', 'S', 'A', 'R',
                  'R', 'R', 'R', 'R', 'R', 'R', 'R', 'R', 'R' },
        { 'X', 'A', 'A', 'A', 'A', 'S', 'A', 'R',
                  'R', 'R', 'R', 'R', 'R', 'R', 'R', 'R', 'R' },
        { 'X', 'A', 'A', 'A', 'S', 'A', 'S', 'R',
                  'R', 'R', 'R', 'R', 'R', 'R', 'R', 'R', 'R' },
        { 'X', 'A', 'A', 'A', 'S', 'S', 'A', 'R',
                  'R', 'R', 'C', 'C', 'R', 'R', 'R', 'R', 'R' },
        { 'X', 'A', 'A', 'A', 'S', 'S', 'A', 'R',
                  'R', 'R', 'C', 'R', 'R', 'R', 'R', 'R', 'R' },
        { 'X', 'A', 'A', 'A', 'S', 'S', 'A', 'R',
                  'R', 'R', 'R', 'R', 'R', 'R', 'R', 'R', 'R' },
        { 'X', 'A', 'A', 'A', 'A', 'A', 'A', 'R',
                  'R', 'R', 'R', 'R', 'R', 'R', 'R', 'R', 'R' },
        { 'X', 'A', 'A', 'A', 'S', 'S', 'A', 'R',
                  'R', 'R', 'R', 'R', 'R', 'R', 'R', 'R', 'R' },
        { 'X', 'A', 'A', 'A', 'S', 'S', 'A', 'R',
                  'R', 'R', 'R', 'R', 'R', 'R', 'R', 'R', 'R' },
        { 'X', 'A', 'A', 'A', 'S', 'S', 'A', 'R',
                  'R', 'R', 'R', 'R', 'R', 'R', 'R', 'R', 'R' },
        { 'X', 'A', 'A', 'A', 'S', 'S', 'A', 'R',
                  'R', 'R', 'C', 'C', 'R', 'R', 'R', 'R', 'R' },
        { 'X', 'A', 'A', 'A', 'S', 'S', 'A', 'R',
                  'R', 'R', 'C', 'R', 'R', 'R', 'R', 'R', 'R' },
        { 'X', 'A', 'A', 'A', 'S', 'S', 'A', 'R',
                  'R', 'R', 'C', 'R', 'C', 'R', 'R', 'R', 'R' }
};

typedef struct {
  gint ShiftDown_TONE_AD[8];
  gint ShiftDownLeft_TONE_AD[8];
  gint ShiftLeft_TONE_AD[8];
  gint ShiftLeft_AV[7];
  gint ShiftDown_BV_BD[3];
  gint TailCutCons[4];
} ThaiShapeTable;

#define shiftdown_tone_ad(c,tbl)     ((tbl)->ShiftDown_TONE_AD[(c)-0xE7])
#define shiftdownleft_tone_ad(c,tbl) ((tbl)->ShiftDownLeft_TONE_AD[(c)-0xE7])
#define shiftleft_tone_ad(c,tbl)     ((tbl)->ShiftLeft_TONE_AD[(c)-0xE7])
#define shiftleft_av(c,tbl)          ((tbl)->ShiftLeft_AV[(c)-0xD1])
#define shiftdown_bv_bd(c,tbl)       ((tbl)->ShiftDown_BV_BD[(c)-0xD8])
#define tailcutcons(c,tbl)           ((tbl)->TailCutCons[(c)-0xAD])



static const ThaiShapeTable Mac_shape_table = {
  { 0xE7, 0x88, 0x89, 0x8A, 0x8B, 0x8C, 0xED, 0xEE },
  { 0xE7, 0x83, 0x84, 0x85, 0x86, 0x87, 0x8F, 0xEE },
  { 0x93, 0x98, 0x99, 0x9A, 0x9B, 0x9C, 0x8F, 0xEE },
  { 0x92, 0x00, 0x00, 0x94, 0x95, 0x96, 0x97 },
  { 0xD8, 0xD9, 0xDA },
  { 0xAD, 0x00, 0x00, 0xB0 }
};



static const ThaiShapeTable Win_shape_table = {
    { 0xE7, 0x8B, 0x8C, 0x8D, 0x8E, 0x8F, 0xED, 0xEE },
    { 0xE7, 0x86, 0x87, 0x88, 0x89, 0x8A, 0x99, 0xEE },
    { 0x9A, 0x9B, 0x9C, 0x9D, 0x9E, 0x9F, 0x99, 0xEE },
    { 0x98, 0x00, 0x00, 0x81, 0x82, 0x83, 0x84 },
    { 0xFC, 0xFD, 0xFE },
    { 0x90, 0x00, 0x00, 0x80 }
};



static const ThaiShapeTable tis620_0_shape_table = {
  { 0xE7, 0xE8, 0xE9, 0xEA, 0xEB, 0xEC, 0xED, 0xEE },
  { 0xE7, 0xE8, 0xE9, 0xEA, 0xEB, 0xEC, 0xED, 0xEE },
  { 0xE7, 0xE8, 0xE9, 0xEA, 0xEB, 0xEC, 0xED, 0xEE },
  { 0xD1, 0x00, 0x00, 0xD4, 0xD5, 0xD6, 0xD7 },
  { 0xD8, 0xD9, 0xDA },
  { 0xAD, 0x00, 0x00, 0xB0 }
};




static ThaiFontInfo *
get_font_info (const char *fontCharset)
{
  static const char *charsets[] = {
    "tis620-2",
    "tis620-1",
    "tis620-0",
    "xtis620.2529-1",
    "xtis-0",
    "tis620.2533-1",
    "tis620.2529-1",
    "iso8859-11",
    "iso10646-1",
  };

  static const int charset_types[] = {
    THAI_FONT_TIS_WIN,
    THAI_FONT_TIS_MAC,
    THAI_FONT_TIS,
    THAI_FONT_XTIS,
    THAI_FONT_XTIS,
    THAI_FONT_TIS,
    THAI_FONT_TIS,
    THAI_FONT_TIS,
    THAI_FONT_ISO10646
  };
  
  ThaiFontInfo *font_info = g_new(ThaiFontInfo, 1);
  guint        i;

  font_info->type = THAI_FONT_NONE;
  for (i = 0; i < G_N_ELEMENTS(charsets); i++) {
    if (strcmp(fontCharset, charsets[i]) == 0) {	  
      font_info->type = (ThaiFontType)charset_types[i];
      font_info->subfont = (PangoliteXSubfont)i;
      break;
    }
  }
  return font_info;
}

static void
add_glyph(ThaiFontInfo         *font_info,
          PangoliteGlyphString *glyphs,
          gint                 cluster_start,
          PangoliteGlyph       glyph,
          gint                 combining)
{
  gint           index = glyphs->num_glyphs;

  if ((cluster_start == 0) && (index != 0))
     cluster_start++;

  pangolite_glyph_string_set_size(glyphs, index + 1);  
  glyphs->glyphs[index].glyph = glyph;
  glyphs->glyphs[index].is_cluster_start = combining;
  glyphs->log_clusters[index] = cluster_start;
}

static gint
get_adjusted_glyphs_list(ThaiFontInfo *font_info,
                         gunichar2     *cluster,
                         gint         num_chrs,
                         PangoliteGlyph   *glyph_lists,
                         const ThaiShapeTable *shaping_table)
{
  switch (num_chrs) {
  case 1:
    if (is_char_type (cluster[0], BelowVowel|BelowDiac|AboveVowel|AboveDiac|Tone)) {
	    if (font_info->type == THAI_FONT_TIS)
	      glyph_lists[0] = PANGO_MOZ_MAKE_GLYPH (0x20);
	    else
	      glyph_lists[0] = PANGO_MOZ_MAKE_GLYPH (0x7F);
      glyph_lists[1] = PANGO_MOZ_MAKE_GLYPH (ucs2tis (cluster[0]));
	    return 2;
    }
    else {
      glyph_lists[0] = PANGO_MOZ_MAKE_GLYPH (ucs2tis (cluster[0]));
      return 1;
    }
    break;
    
  case 2:
    if (is_char_type (cluster[0], NoTailCons|BotTailCons|SpltTailCons) &&
        is_char_type (cluster[1], SaraAm)) {
      glyph_lists[0] = PANGO_MOZ_MAKE_GLYPH (ucs2tis (cluster[0]));
      glyph_lists[1] = PANGO_MOZ_MAKE_GLYPH (0xED);
      glyph_lists[2] = PANGO_MOZ_MAKE_GLYPH (0xD2);
      return 3;
    }
    else if (is_char_type (cluster[0], UpTailCons) &&
          	 is_char_type (cluster[1], SaraAm)) {
      glyph_lists[0] = PANGO_MOZ_MAKE_GLYPH (ucs2tis (cluster[0]));
      glyph_lists[1] = PANGO_MOZ_MAKE_GLYPH (shiftleft_tone_ad (0xED, shaping_table));
      glyph_lists[2] = PANGO_MOZ_MAKE_GLYPH (0xD2);
      return 3;
    }
    else if (is_char_type (cluster[0], NoTailCons|BotTailCons|SpltTailCons) &&
          	 is_char_type (cluster[1], AboveVowel)) {
      glyph_lists[0] = PANGO_MOZ_MAKE_GLYPH (ucs2tis (cluster[0]));
      glyph_lists[1] = PANGO_MOZ_MAKE_GLYPH (ucs2tis (cluster[1]));
      return 2;
    }
    else if (is_char_type (cluster[0], NoTailCons|BotTailCons|SpltTailCons) &&
          	 is_char_type (cluster[1], AboveDiac|Tone)) {
      glyph_lists[0] = PANGO_MOZ_MAKE_GLYPH (ucs2tis (cluster[0]));
      glyph_lists[1] = PANGO_MOZ_MAKE_GLYPH (shiftdown_tone_ad (ucs2tis (cluster[1]), shaping_table));
      return 2;
    }
    else if (is_char_type (cluster[0], UpTailCons) &&
          	 is_char_type (cluster[1], AboveVowel)) {
      glyph_lists[0] = PANGO_MOZ_MAKE_GLYPH (ucs2tis (cluster[0]));
      glyph_lists[1] = PANGO_MOZ_MAKE_GLYPH (shiftleft_av (ucs2tis (cluster[1]), shaping_table));
      return 2;
    }
    else if (is_char_type (cluster[0], UpTailCons) &&
          	 is_char_type (cluster[1], AboveDiac|Tone)) {
      glyph_lists[0] = PANGO_MOZ_MAKE_GLYPH (ucs2tis (cluster[0]));
      glyph_lists[1] = PANGO_MOZ_MAKE_GLYPH (shiftdownleft_tone_ad (ucs2tis (cluster[1]), shaping_table));
      return 2;
    }
    else if (is_char_type (cluster[0], NoTailCons|UpTailCons) &&
          	 is_char_type (cluster[1], BelowVowel|BelowDiac)) {
      glyph_lists[0] = PANGO_MOZ_MAKE_GLYPH (ucs2tis (cluster[0]));
      glyph_lists[1] = PANGO_MOZ_MAKE_GLYPH (ucs2tis (cluster[1]));
      return 2;
    }
    else if (is_char_type (cluster[0], BotTailCons) &&
             is_char_type (cluster[1], BelowVowel|BelowDiac)) {
      glyph_lists[0] = PANGO_MOZ_MAKE_GLYPH (ucs2tis (cluster[0]));
      glyph_lists[1] = PANGO_MOZ_MAKE_GLYPH (shiftdown_bv_bd (ucs2tis (cluster[1]), shaping_table));
      return 2;
    }
    else if (is_char_type (cluster[0], SpltTailCons) &&
          	 is_char_type (cluster[1], BelowVowel|BelowDiac)) {
        glyph_lists[0] = PANGO_MOZ_MAKE_GLYPH (tailcutcons (ucs2tis (cluster[0]), shaping_table));
        glyph_lists[1] = PANGO_MOZ_MAKE_GLYPH (ucs2tis (cluster[1]));
        return 2;
      }
    else {
      if (font_info->type == THAI_FONT_TIS)
        glyph_lists[0] = PANGO_MOZ_MAKE_GLYPH (0x20);
      else
        glyph_lists[0] = PANGO_MOZ_MAKE_GLYPH (0x7F);
      glyph_lists[1] =
        PANGO_MOZ_MAKE_GLYPH (ucs2tis (cluster[0]));
      glyph_lists[2] =
        PANGO_MOZ_MAKE_GLYPH (ucs2tis (cluster[1]));
      return 3;
    }
    break;
    
  case 3:
    if (is_char_type (cluster[0], NoTailCons|BotTailCons|SpltTailCons) &&
        is_char_type (cluster[1], Tone) &&
        is_char_type (cluster[2], SaraAm)) {
      glyph_lists[0] = PANGO_MOZ_MAKE_GLYPH (ucs2tis (cluster[0]));
      glyph_lists[1] = PANGO_MOZ_MAKE_GLYPH (0xED);
      glyph_lists[2] = PANGO_MOZ_MAKE_GLYPH (ucs2tis (cluster[1]));
      glyph_lists[3] = PANGO_MOZ_MAKE_GLYPH (0xD2);
      return 4;
    }
    else if (is_char_type (cluster[0], UpTailCons) &&
             is_char_type (cluster[1], Tone) &&
             is_char_type (cluster[2], SaraAm)) {
      glyph_lists[0] = PANGO_MOZ_MAKE_GLYPH (ucs2tis (cluster[0]));
      glyph_lists[1] = PANGO_MOZ_MAKE_GLYPH (shiftleft_tone_ad (0xED, shaping_table));
      glyph_lists[2] = PANGO_MOZ_MAKE_GLYPH (shiftleft_tone_ad (ucs2tis (cluster[1]), shaping_table));
      glyph_lists[3] = PANGO_MOZ_MAKE_GLYPH (0xD2);
      return 4;
    }
    else if (is_char_type (cluster[0], UpTailCons) &&
             is_char_type (cluster[1], AboveVowel) &&
             is_char_type (cluster[2], AboveDiac|Tone)) {
      glyph_lists[0] = PANGO_MOZ_MAKE_GLYPH (ucs2tis (cluster[0]));
      glyph_lists[1] = PANGO_MOZ_MAKE_GLYPH (shiftleft_av (ucs2tis (cluster[1]), shaping_table));
      glyph_lists[2] = PANGO_MOZ_MAKE_GLYPH (shiftleft_tone_ad (ucs2tis (cluster[2]), shaping_table));
      return 3;
    }
    else if (is_char_type (cluster[0], UpTailCons) &&
             is_char_type (cluster[1], BelowVowel) &&
             is_char_type (cluster[2], AboveDiac|Tone)) {
      glyph_lists[0] = PANGO_MOZ_MAKE_GLYPH (ucs2tis (cluster[0]));
      glyph_lists[1] = PANGO_MOZ_MAKE_GLYPH (ucs2tis (cluster[1]));
      glyph_lists[2] = PANGO_MOZ_MAKE_GLYPH (shiftdownleft_tone_ad (ucs2tis (cluster[2]), shaping_table));
      return 3;
    }
    else if (is_char_type (cluster[0], NoTailCons) &&
             is_char_type (cluster[1], BelowVowel) &&
             is_char_type (cluster[2], AboveDiac|Tone)) {
      glyph_lists[0] = PANGO_MOZ_MAKE_GLYPH (ucs2tis (cluster[0]));
      glyph_lists[1] = PANGO_MOZ_MAKE_GLYPH (ucs2tis (cluster[1]));
      glyph_lists[2] = PANGO_MOZ_MAKE_GLYPH (shiftdown_tone_ad (ucs2tis (cluster[2]), shaping_table));
      return 3;
    }
    else if (is_char_type (cluster[0], SpltTailCons) &&
             is_char_type (cluster[1], BelowVowel) &&
             is_char_type (cluster[2], AboveDiac|Tone)) {
      glyph_lists[0] = PANGO_MOZ_MAKE_GLYPH (tailcutcons (ucs2tis (cluster[0]), shaping_table));
      glyph_lists[1] = PANGO_MOZ_MAKE_GLYPH (ucs2tis (cluster[1]));
      glyph_lists[2] = PANGO_MOZ_MAKE_GLYPH (shiftdown_tone_ad (ucs2tis (cluster[2]), shaping_table));
      return 3;
    }
    else if (is_char_type (cluster[0], BotTailCons) &&
             is_char_type (cluster[1], BelowVowel) &&
             is_char_type (cluster[2], AboveDiac|Tone)) {
      glyph_lists[0] = PANGO_MOZ_MAKE_GLYPH (ucs2tis (cluster[0]));
      glyph_lists[1] = PANGO_MOZ_MAKE_GLYPH (shiftdown_bv_bd (ucs2tis (cluster[1]), shaping_table));
      glyph_lists[2] = PANGO_MOZ_MAKE_GLYPH (shiftdown_tone_ad (ucs2tis (cluster[2]), shaping_table));
      return 3;
    }
    else {
      glyph_lists[0] = PANGO_MOZ_MAKE_GLYPH (ucs2tis (cluster[0]));
      glyph_lists[1] = PANGO_MOZ_MAKE_GLYPH (ucs2tis (cluster[1]));
      glyph_lists[2] = PANGO_MOZ_MAKE_GLYPH (ucs2tis (cluster[2]));
      return 3;
    }
    break;
  }
  
  return 0;
}

static gint
get_glyphs_list(ThaiFontInfo *font_info,
                gunichar2	   *cluster,
                gint		     num_chrs,
                PangoliteGlyph	 *glyph_lists)
{
  PangoliteGlyph glyph;
  gint       xtis_index, i;

  switch (font_info->type) {
  case THAI_FONT_NONE:
    for (i = 0; i < num_chrs; i++)
      
      glyph_lists[i] = 0; 
    return num_chrs;
    
  case THAI_FONT_XTIS:
    


    xtis_index = 0x100 * (cluster[0] - 0xe00 + 0x20) + 0x30;
    if (cluster[1])
	    xtis_index +=8 * group1_map[cluster[1] - 0xe30];
    if (cluster[2])
	    xtis_index += group2_map[cluster[2] - 0xe30];
    glyph = PANGO_MOZ_MAKE_GLYPH(xtis_index);
    





    for (i=0; i < num_chrs; i++)
      glyph_lists[i] = PANGO_MOZ_MAKE_GLYPH(0x100 * (cluster[i] - 0xe00 + 0x20) + 0x30);
    return num_chrs;
    
  case THAI_FONT_TIS:
    

    return get_adjusted_glyphs_list (font_info, cluster,
                                     num_chrs, glyph_lists, &tis620_0_shape_table);
    
  case THAI_FONT_TIS_MAC:
    

    return get_adjusted_glyphs_list(font_info, cluster,
                                    num_chrs, glyph_lists, &Mac_shape_table);
    
  case THAI_FONT_TIS_WIN:
    

    return get_adjusted_glyphs_list(font_info, cluster,
                                    num_chrs, glyph_lists, &Win_shape_table);
    
  case THAI_FONT_ISO10646:
    for (i=0; i < num_chrs; i++)
      glyph_lists[i] = PANGO_MOZ_MAKE_GLYPH(cluster[i]);
    return num_chrs;
  }
  
  return 0;			
}

static void
add_cluster(ThaiFontInfo         *font_info,
            PangoliteGlyphString *glyphs,
            gint                 cluster_start,
            gunichar2            *cluster,
            gint                 num_chrs)
	     
{
  PangoliteGlyph glyphs_list[MAX_GLYPHS];
  gint           i, num_glyphs, ClusterStart=0;
  
  num_glyphs = get_glyphs_list(font_info, cluster, num_chrs, glyphs_list);
  for (i=0; i<num_glyphs; i++) {
    ClusterStart = (gint)GLYPH_COMBINING;
    if (i == 0)
      ClusterStart = num_chrs;

    add_glyph(font_info, glyphs, cluster_start, glyphs_list[i], ClusterStart);
  }
}

static gboolean
is_wtt_composible (gunichar2 cur_wc, gunichar2 nxt_wc)
{
  switch (TAC_compose_and_input_check_type_table[char_class(ucs2tis(cur_wc))]
          [char_class(ucs2tis(nxt_wc))]) {
  case 'A':
  case 'S':
  case 'R':
  case 'X':
    return FALSE;
    
  case 'C':
    return TRUE;
  }
  
  g_assert_not_reached();
  return FALSE;
}

static const gunichar2 *
get_next_cluster(const gunichar2 *text,
                 gint           length,
                 gunichar2       *cluster,
                 gint           *num_chrs)
{
  const gunichar2 *p;
  gint  n_chars = 0;
  
  p = text;
  while (p < text + length && n_chars < 3) {
    gunichar2 current = *p;
    
    if (n_chars == 0 ||
        is_wtt_composible ((gunichar2)(cluster[n_chars - 1]), current) ||
        (n_chars == 1 &&
         is_char_type (cluster[0], Cons) &&
         is_char_type (current, SaraAm)) ||
        (n_chars == 2 &&
         is_char_type (cluster[0], Cons) &&
         is_char_type (cluster[1], Tone) &&
         is_char_type (current, SaraAm))) {
      cluster[n_chars++] = current;
  p++;
    }
    else
      break;
  }
  
  *num_chrs = n_chars;
  return p;
} 

static void 
thai_engine_shape(const char       *fontCharset,
                  const gunichar2   *text,
                  gint             length,
                  PangoliteAnalysis    *analysis,
                  PangoliteGlyphString *glyphs)
{
  ThaiFontInfo   *font_info;
  const gunichar2 *p;
  const gunichar2 *log_cluster;
  gunichar2       cluster[MAX_CLUSTER_CHRS];
  gint           num_chrs;

  font_info = get_font_info(fontCharset);

  p = text;
  while (p < text + length) {
    log_cluster = p;
    p = get_next_cluster(p, text + length - p, cluster, &num_chrs);
    add_cluster(font_info, glyphs, log_cluster - text, cluster, num_chrs);
  }
}

static PangoliteCoverage *
thai_engine_get_coverage(const char *fontCharset,
                         const char *lang)
{
  PangoliteCoverage *result = pangolite_coverage_new();  
  ThaiFontInfo *font_info = get_font_info(fontCharset);
  
  if (font_info->type != THAI_FONT_NONE) {
    gunichar2 wc;
    
    for (wc = 0xe01; wc <= 0xe3a; wc++)
      pangolite_coverage_set(result, wc, PANGO_COVERAGE_EXACT);
    for (wc = 0xe3f; wc <= 0xe5b; wc++)
      pangolite_coverage_set(result, wc, PANGO_COVERAGE_EXACT);
  }
  
  return result;
}

static PangoliteEngine *
thai_engine_x_new()
{
  PangoliteEngineShape *result;
  
  result = g_new(PangoliteEngineShape, 1);
  
  result->engine.id = SCRIPT_ENGINE_NAME;
  result->engine.type = PANGO_ENGINE_TYPE_SHAPE;
  result->engine.length = sizeof(result);
  result->script_shape = thai_engine_shape;
  result->get_coverage = thai_engine_get_coverage;

  return(PangoliteEngine *)result;
}








#ifdef X_MODULE_PREFIX
#define MODULE_ENTRY(func) _pangolite_thai_x_##func
#else
#define MODULE_ENTRY(func) func
#endif



void 
MODULE_ENTRY(script_engine_list)(PangoliteEngineInfo **engines, gint *n_engines)
{
  *engines = script_engines;
  *n_engines = G_N_ELEMENTS(script_engines);
}



PangoliteEngine *
MODULE_ENTRY(script_engine_load)(const char *id)
{
  if (!strcmp(id, SCRIPT_ENGINE_NAME))
    return thai_engine_x_new();
  else
    return NULL;
}

void 
MODULE_ENTRY(script_engine_unload)(PangoliteEngine *engine)
{
}
