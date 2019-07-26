





#ifndef vm_RegExpStaticsObject_h
#define vm_RegExpStaticsObject_h

#include "jsobj.h"

namespace js {

class RegExpStaticsObject : public JSObject
{
  public:
    static Class class_;

    size_t sizeOfData(mozilla::MallocSizeOf mallocSizeOf) {
        return mallocSizeOf(getPrivate());
    }
};

} 

#endif 
