





#ifndef vm_Runtime_inl_h
#define vm_Runtime_inl_h

#include "vm/Runtime.h"

#include "jscompartment.h"

#include "gc/Allocator.h"
#include "gc/GCTrace.h"
#include "vm/Probes.h"

#include "jsobjinlines.h"

namespace js {

inline bool
NewObjectCache::lookupProto(const Class* clasp, JSObject* proto, gc::AllocKind kind, EntryIndex* pentry)
{
    MOZ_ASSERT(!proto->is<GlobalObject>());
    return lookup(clasp, proto, kind, pentry);
}

inline bool
NewObjectCache::lookupGlobal(const Class* clasp, GlobalObject* global, gc::AllocKind kind, EntryIndex* pentry)
{
    return lookup(clasp, global, kind, pentry);
}

inline void
NewObjectCache::fillGlobal(EntryIndex entry, const Class* clasp, GlobalObject* global,
                           gc::AllocKind kind, NativeObject* obj)
{
    
    return fill(entry, clasp, global, kind, obj);
}

inline NativeObject*
NewObjectCache::newObjectFromHit(JSContext* cx, EntryIndex entryIndex, gc::InitialHeap heap)
{
    MOZ_ASSERT(unsigned(entryIndex) < mozilla::ArrayLength(entries));
    Entry* entry = &entries[entryIndex];

    NativeObject* templateObj = reinterpret_cast<NativeObject*>(&entry->templateObject);

    
    
    ObjectGroup* group = templateObj->group_;

    if (group->shouldPreTenure())
        heap = gc::TenuredHeap;

    if (cx->runtime()->gc.upcomingZealousGC())
        return nullptr;

    NativeObject* obj = static_cast<NativeObject*>(Allocate<JSObject, NoGC>(cx, entry->kind, 0,
                                                                             heap, group->clasp()));
    if (!obj)
        return nullptr;

    copyCachedToObject(obj, templateObj, entry->kind);

    SetNewObjectMetadata(cx, obj);

    probes::CreateObject(cx, obj);
    gc::TraceCreateObject(obj);
    return obj;
}

}  

#endif 
