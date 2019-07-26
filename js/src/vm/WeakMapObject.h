





#ifndef vm_WeakMapObject_h
#define vm_WeakMapObject_h

#include "jsobj.h"
#include "jsweakmap.h"

namespace js {

class ObjectValueMap : public WeakMap<PreBarrieredObject, RelocatableValue>
{
  public:
    ObjectValueMap(JSContext *cx, JSObject *obj)
      : WeakMap<PreBarrieredObject, RelocatableValue>(cx, obj) {}

    virtual bool findZoneEdges();
};

class WeakMapObject : public JSObject
{
  public:
    static const Class class_;

    ObjectValueMap *getMap() { return static_cast<ObjectValueMap*>(getPrivate()); }
};

} 

#endif 
