







































#ifndef __PANGO_ENGINE_H__
#define __PANGO_ENGINE_H__

#include "pango-types.h"
#include "pango-glyph.h"
#include "pango-coverage.h"

#ifdef __cplusplus
extern "C" {
#endif



#define PANGO_ENGINE_TYPE_SHAPE "PangoliteEngineShape"
#define PANGO_RENDER_TYPE_X    "PangoliteRenderX"
#define PANGO_RENDER_TYPE_NONE "PangoliteRenderNone"

typedef struct _PangoliteEngineInfo PangoliteEngineInfo;
typedef struct _PangoliteEngineRange PangoliteEngineRange;
typedef struct _PangoliteEngine PangoliteEngine;

struct _PangoliteEngineRange 
{
  guint32 start;
  guint32 end;
  gchar   *langs;
};

struct _PangoliteEngineInfo
{
  gchar            *id;
  gchar            *engine_type;
  gchar            *render_type;
  PangoliteEngineRange *ranges;
  gint             n_ranges;
};

struct _PangoliteEngine
{
  gchar *id;
  gchar *type;
  gint  length;
};

struct _PangoliteEngineShape
{
  PangoliteEngine engine;
  void (*script_shape) (const char       *fontCharset, 
                        const gunichar2  *text, 
                        int              length, 
                        PangoliteAnalysis    *analysis, 
                        PangoliteGlyphString *glyphs);
  PangoliteCoverage *(*get_coverage) (const char *fontCharset, const char *lang);

};


void         script_engine_list(PangoliteEngineInfo **engines, int *n_engines);
PangoliteEngine *script_engine_load(const char *id);
void         script_engine_unload(PangoliteEngine *engine);

#ifdef __cplusplus
}
#endif 

#endif
