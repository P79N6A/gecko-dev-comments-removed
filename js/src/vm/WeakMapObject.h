





#ifndef vm_WeakMapObject_h
#define vm_WeakMapObject_h

#include "jsobj.h"
#include "jsweakmap.h"

namespace js {

class ObjectValueMap : public WeakMap<PreBarrieredObject, RelocatableValue>
{
  public:
    ObjectValueMap(JSContext* cx, JSObject* obj)
      : WeakMap<PreBarrieredObject, RelocatableValue>(cx, obj) {}

    virtual bool findZoneEdges();
};

class WeakMapObject : public NativeObject
{
  public:
    static const Class class_;

    ObjectValueMap* getMap() { return static_cast<ObjectValueMap*>(getPrivate()); }
};


class ObjectWeakMap
{
  private:
    ObjectValueMap map;
    typedef gc::HashKeyRef<ObjectValueMap, JSObject*> StoreBufferRef;

  public:
    explicit ObjectWeakMap(JSContext* cx);
    ~ObjectWeakMap();

    JSObject* lookup(const JSObject* obj);
    bool add(JSContext* cx, JSObject* obj, JSObject* target);
    void clear();

    void trace(JSTracer* trc);
    size_t sizeOfExcludingThis(mozilla::MallocSizeOf mallocSizeOf);
    size_t sizeOfIncludingThis(mozilla::MallocSizeOf mallocSizeOf) {
        return mallocSizeOf(this) + sizeOfExcludingThis(mallocSizeOf);
    }

#ifdef JSGC_HASH_TABLE_CHECKS
    void checkAfterMovingGC();
#endif
};

} 

#endif 
