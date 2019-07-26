





#ifndef vm_WeakMapObject_h
#define vm_WeakMapObject_h

#include "jsobj.h"
#include "jsweakmap.h"

namespace js {

typedef WeakMap<PreBarrieredObject, RelocatableValue> ObjectValueMap;

class WeakMapObject : public JSObject
{
  public:
    static const Class class_;

    ObjectValueMap *getMap() { return static_cast<ObjectValueMap*>(getPrivate()); }
};

} 

#endif 
