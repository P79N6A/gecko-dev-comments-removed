







































#ifndef ParseMapPool_inl_h__
#define ParseMapPool_inl_h__

#include "jscntxt.h"
#include "jsparse.h" 

#include "ParseMaps.h"

namespace js {

template <>
inline AtomDefnMap *
ParseMapPool::acquire<AtomDefnMap>()
{
    return reinterpret_cast<AtomDefnMap *>(allocate());
}

template <>
inline AtomIndexMap *
ParseMapPool::acquire<AtomIndexMap>()
{
    return reinterpret_cast<AtomIndexMap *>(allocate());
}

template <>
inline AtomDOHMap *
ParseMapPool::acquire<AtomDOHMap>()
{
    return reinterpret_cast<AtomDOHMap *>(allocate());
}

inline void *
ParseMapPool::allocate()
{
    if (recyclable.empty())
        return allocateFresh();

    void *map = recyclable.popCopy();
    asAtomMap(map)->clear();
    return map;
}

inline JSDefinition *
AtomDecls::lookupFirst(JSAtom *atom)
{
    JS_ASSERT(map);
    AtomDOHPtr p = map->lookup(atom);
    if (!p)
        return NULL;
    if (p.value().isHeader()) {
        
        return p.value().header()->defn;
    }
    return p.value().defn();
}

inline MultiDeclRange
AtomDecls::lookupMulti(JSAtom *atom)
{
    JS_ASSERT(map);
    AtomDOHPtr p = map->lookup(atom);
    if (!p)
        return MultiDeclRange((JSDefinition *) NULL);

    DefnOrHeader &doh = p.value();
    if (doh.isHeader())
        return MultiDeclRange(doh.header());
    else
        return MultiDeclRange(doh.defn());
}

inline bool
AtomDecls::addUnique(JSAtom *atom, JSDefinition *defn)
{
    JS_ASSERT(map);
    AtomDOHAddPtr p = map->lookupForAdd(atom);
    if (p) {
        JS_ASSERT(!p.value().isHeader());
        p.value() = DefnOrHeader(defn);
        return true;
    }
    return map->add(p, atom, DefnOrHeader(defn));
}

template <class Map>
inline bool
AtomThingMapPtr<Map>::ensureMap(JSContext *cx)
{
    if (map_)
        return true;
    map_ = cx->parseMapPool().acquire<Map>();
    return !!map_;
}

template <class Map>
inline void
AtomThingMapPtr<Map>::releaseMap(JSContext *cx)
{
    if (!map_)
        return;
    cx->parseMapPool().release(map_);
    map_ = NULL;
}

inline bool
AtomDecls::init()
{
    map = cx->parseMapPool().acquire<AtomDOHMap>();
    return map;
}

inline
AtomDecls::~AtomDecls()
{
    if (map)
        cx->parseMapPool().release(map);
}

} 

#endif
