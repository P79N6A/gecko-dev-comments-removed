





#ifndef vm_ErrorObject_h_
#define vm_ErrorObject_h_

#include "jsobj.h"

struct JSExnPrivate;

namespace js {

class ErrorObject : public JSObject
{
  public:
    static const Class class_;

    JSExnPrivate *getExnPrivate() { return static_cast<JSExnPrivate*>(getPrivate()); }
};

} 

#endif 
