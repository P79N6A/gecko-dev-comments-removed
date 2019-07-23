







































#ifndef __PANGO_MODULES_H__
#define __PANGO_MODULES_H__

#include "pango-engine.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _PangoliteMap PangoliteMap;
typedef struct _PangoliteMapEntry PangoliteMapEntry;

struct _PangoliteMapEntry 
{
  PangoliteEngineInfo *info;
  gboolean        is_exact;
};

typedef struct _PangoliteIncludedModule PangoliteIncludedModule;

struct _PangoliteIncludedModule
{
  void        (*list) (PangoliteEngineInfo **engines, int *n_engines);
  PangoliteEngine *(*load) (const char *id);
  void        (*unload) (PangoliteEngine *engine);
};

PangoliteMap      *pangolite_find_map(const char *lang, guint engine_type_id, 
                              guint render_type_id);
PangoliteMapEntry *pangolite_map_get_entry(PangoliteMap *map, guint32 wc);
PangoliteEngine   *pangolite_map_get_engine(PangoliteMap *map, guint32 wc);
void          pangolite_module_register(PangoliteIncludedModule *module);

#ifdef __cplusplus
}
#endif 

#endif
