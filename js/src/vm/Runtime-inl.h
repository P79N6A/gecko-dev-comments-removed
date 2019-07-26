





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
    JS_ASSERT(!proto->is<GlobalObject>());
    return lookup(clasp, proto, kind, pentry);
}

inline bool
NewObjectCache::lookupGlobal(const Class *clasp, js::GlobalObject *global, gc::AllocKind kind, EntryIndex *pentry)
{
    return lookup(clasp, global, kind, pentry);
}

inline void
NewObjectCache::fillGlobal(EntryIndex entry, const Class *clasp, js::GlobalObject *global, gc::AllocKind kind, JSObject *obj)
{
    
    return fill(entry, clasp, global, kind, obj);
}

inline JSObject *
NewObjectCache::newObjectFromHit(JSContext *cx, EntryIndex entry_, js::gc::InitialHeap heap)
{
    
    JS_ASSERT(!cx->compartment()->hasObjectMetadataCallback());

    JS_ASSERT(unsigned(entry_) < mozilla::ArrayLength(entries));
    Entry *entry = &entries[entry_];

    JSObject *templateObj = reinterpret_cast<JSObject *>(&entry->templateObject);

    
    
    types::TypeObject *type = templateObj->type_;

    if (type->shouldPreTenure())
        heap = gc::TenuredHeap;

    if (cx->runtime()->upcomingZealousGC())
        return nullptr;

    JSObject *obj = js::NewGCObject<NoGC>(cx, entry->kind, 0, heap);
    if (obj) {
        copyCachedToObject(obj, templateObj, entry->kind);
        probes::CreateObject(cx, obj);
        return obj;
    }

    return nullptr;
}

}  

#endif 
