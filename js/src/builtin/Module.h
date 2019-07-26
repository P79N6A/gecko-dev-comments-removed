





#ifndef builtin_Module_h
#define builtin_Module_h

#include "jsobj.h"

namespace js {

class Module : public JSObject {
  public:
    static Module *create(ExclusiveContext *cx, js::HandleAtom atom);

    JSAtom *atom() {
        return &getReservedSlot(ATOM_SLOT).toString()->asAtom();
    };

    JSScript *script() {
        return (JSScript *) getReservedSlot(SCRIPT_SLOT).toPrivate();
    }

    static Class class_;

  private:
    inline void setAtom(JSAtom *atom);
    inline void setScript(JSScript *script);

    static const uint32_t ATOM_SLOT = 0;
    static const uint32_t SCRIPT_SLOT = 1;
};

} 

#endif 
