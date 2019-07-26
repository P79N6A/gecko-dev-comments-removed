





#ifndef Module_h___
#define Module_h___

#include "jsobj.h"

namespace js {

class Module : public JSObject {
  public:
    static Module *create(JSContext *cx, js::HandleAtom atom);

    JSAtom *atom() {
        return &getReservedSlot(ATOM_SLOT).toString()->asAtom();
    };

    JSScript *script() {
        return (JSScript *) getReservedSlot(SCRIPT_SLOT).toPrivate();
    }

  private:
    inline void setAtom(JSAtom *atom);
    inline void setScript(JSScript *script);

    static const uint32_t ATOM_SLOT = 0;
    static const uint32_t SCRIPT_SLOT = 1;
};

} 

inline js::Module &
JSObject::asModule()
{
    JS_ASSERT(isModule());
    return *static_cast<js::Module *>(this);
}

#endif 
