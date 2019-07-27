





#ifndef vm_Runtime_inl_h
#define vm_Runtime_inl_h

#include "vm/Runtime.h"

#include "jscompartment.h"

#include "vm/Probes.h"

#include "jsgcinlines.h"

namespace js {

inline bool
NewObjectCache::lookupProto(const Class *clasp, JSObject *proto, gc::AllocKind kind, EntryIndex *pentry)
{
    MOZ_ASSERT(!proto->is<GlobalObject>());
    return lookup(clasp, proto, kind, pentry);
}

inline bool
NewObjectCache::lookupGlobal(const Class *clasp, js::GlobalObject *global, gc::AllocKind kind, EntryIndex *pentry)
{
    return lookup(clasp, global, kind, pentry);
}

inline void
NewObjectCache::fillGlobal(EntryIndex entry, const Class *clasp, js::GlobalObject *global,
                           gc::AllocKind kind, NativeObject *obj)
{
    
    return fill(entry, clasp, global, kind, obj);
}

inline JSObject *
NewObjectCache::newObjectFromHit(JSContext *cx, EntryIndex entryIndex, js::gc::InitialHeap heap)
{
    
    MOZ_ASSERT(!cx->compartment()->hasObjectMetadataCallback());

    MOZ_ASSERT(unsigned(entryIndex) < mozilla::ArrayLength(entries));
    Entry *entry = &entries[entryIndex];

    JSObject *templateObj = reinterpret_cast<JSObject *>(&entry->templateObject);

    
    
    ObjectGroup *group = templateObj->group_;

    if (group->shouldPreTenure())
        heap = gc::TenuredHeap;

    if (cx->runtime()->gc.upcomingZealousGC())
        return nullptr;

    JSObject *obj = js::gc::AllocateObjectForCacheHit<NoGC>(cx, entry->kind, heap, group->clasp());
    if (obj) {
        copyCachedToObject(obj, templateObj, entry->kind);
        probes::CreateObject(cx, obj);
        js::gc::TraceCreateObject(obj);
        return obj;
    }

    
    
    mozilla::DebugOnly<JSObject *> obj2 =
        js::gc::AllocateObjectForCacheHit<CanGC>(cx, entry->kind, heap, group->clasp());
    MOZ_ASSERT(!obj2);
    return nullptr;
}

}  

#endif 
