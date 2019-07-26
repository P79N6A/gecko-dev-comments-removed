





#ifndef vm_GeneratorObject_h
#define vm_GeneratorObject_h

#include "jsobj.h"

namespace js {

class GeneratorObject : public JSObject
{
  public:
    static Class class_;

    JSGenerator *getGenerator() { return static_cast<JSGenerator*>(getPrivate()); }
};

} 

#endif 
