







































#ifndef __PANGO_TYPES_H__
#define __PANGO_TYPES_H__

#include <glib.h>

typedef struct _PangoliteLangRange PangoliteLangRange;
typedef struct _PangoliteLogAttr PangoliteLogAttr;

typedef struct _PangoliteEngineShape PangoliteEngineShape;



typedef guint32 PangoliteGlyph;




typedef enum {
  PANGO_DIRECTION_LTR,
  PANGO_DIRECTION_RTL,
  PANGO_DIRECTION_TTB_LTR,
  PANGO_DIRECTION_TTB_RTL
} PangoliteDirection;



struct _PangoliteLangRange
{
  gint  start;
  gint  length;
  gchar *lang;
};


typedef struct _PangoliteAnalysis PangoliteAnalysis;

struct _PangoliteAnalysis
{
  char             *fontCharset;
  PangoliteEngineShape *shape_engine;
  
  PangoliteDirection   aDir;
};

#ifndef MOZ_WIDGET_GTK2 

typedef guint32 gunichar;
typedef guint16 gunichar2;
#endif

#define G_CONST_RETURN const

#ifdef __cplusplus
extern "C" {
#endif

#ifdef __cplusplus
}
#endif 

#endif
