





#ifndef vm_GeneratorObject_h
#define vm_GeneratorObject_h

#include "jsobj.h"

namespace js {

class LegacyGeneratorObject : public JSObject
{
  public:
    static const Class class_;

    JSGenerator *getGenerator() { return static_cast<JSGenerator*>(getPrivate()); }
};

class StarGeneratorObject : public JSObject
{
  public:
    static const Class class_;

    JSGenerator *getGenerator() { return static_cast<JSGenerator*>(getPrivate()); }
};

} 

#endif 
