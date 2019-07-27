





#ifndef frontend_ParseMaps_inl_h
#define frontend_ParseMaps_inl_h

#include "frontend/ParseMaps.h"

#include "jscntxtinlines.h"

namespace js {
namespace frontend {

template <class Map>
inline bool
AtomThingMapPtr<Map>::ensureMap(ExclusiveContext *cx)
{
    if (map_)
        return true;

    AutoLockForExclusiveAccess lock(cx);
    map_ = cx->parseMapPool().acquire<Map>();
    return !!map_;
}

template <class Map>
inline void
AtomThingMapPtr<Map>::releaseMap(ExclusiveContext *cx)
{
    if (!map_)
        return;

    AutoLockForExclusiveAccess lock(cx);
    cx->parseMapPool().release(map_);
    map_ = nullptr;
}

template <typename ParseHandler>
inline bool
AtomDecls<ParseHandler>::init()
{
    AutoLockForExclusiveAccess lock(cx);
    map = cx->parseMapPool().acquire<AtomDefnListMap>();
    return map;
}

template <typename ParseHandler>
inline
AtomDecls<ParseHandler>::~AtomDecls()
{
    if (map) {
        AutoLockForExclusiveAccess lock(cx);
        cx->parseMapPool().release(map);
    }
}

} 
} 

#endif 
