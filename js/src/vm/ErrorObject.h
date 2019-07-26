





#ifndef vm_ErrorObject_h_
#define vm_ErrorObject_h_

#include "jsobj.h"

class JSExnPrivate;

namespace js {

class ErrorObject : public JSObject
{
  public:
    static Class class_;

    JSExnPrivate *getExnPrivate() { return static_cast<JSExnPrivate*>(getPrivate()); }
};

} 

#endif 
