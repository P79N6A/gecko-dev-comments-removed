





#ifndef frontend_ParseMaps_inl_h
#define frontend_ParseMaps_inl_h

#include "jscntxt.h"

#include "frontend/ParseMaps.h"

namespace js {
namespace frontend {

template <class Map>
inline bool
AtomThingMapPtr<Map>::ensureMap(JSContext *cx)
{
    if (map_)
        return true;
    map_ = cx->runtime()->parseMapPool.acquire<Map>();
    return !!map_;
}

template <class Map>
inline void
AtomThingMapPtr<Map>::releaseMap(JSContext *cx)
{
    if (!map_)
        return;
    cx->runtime()->parseMapPool.release(map_);
    map_ = NULL;
}

template <typename ParseHandler>
inline bool
AtomDecls<ParseHandler>::init()
{
    map = cx->runtime()->parseMapPool.acquire<AtomDefnListMap>();
    return map;
}

template <typename ParseHandler>
inline
AtomDecls<ParseHandler>::~AtomDecls()
{
    if (map)
        cx->runtime()->parseMapPool.release(map);
}

} 
} 

#endif 
