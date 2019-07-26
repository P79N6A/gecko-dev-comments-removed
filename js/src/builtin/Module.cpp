





#include "builtin/Module.h"

#include "jsobjinlines.h"

using namespace js;

Class Module::class_ = {
    "Module",
    JSCLASS_HAS_RESERVED_SLOTS(2) | JSCLASS_IS_ANONYMOUS,
    JS_PropertyStub,        
    JS_DeletePropertyStub,  
    JS_PropertyStub,        
    JS_StrictPropertyStub,  
    JS_EnumerateStub,
    JS_ResolveStub,
    JS_ConvertStub
};

inline void
Module::setAtom(JSAtom *atom)
{
    setReservedSlot(ATOM_SLOT, StringValue(atom));
}

inline void
Module::setScript(JSScript *script)
{
    setReservedSlot(SCRIPT_SLOT, PrivateValue(script));
}

Module *
Module::create(ExclusiveContext *cx, HandleAtom atom)
{
    RootedObject object(cx, NewBuiltinClassInstance(cx, &class_));
    if (!object)
        return NULL;
    RootedModule module(cx, &object->as<Module>());
    module->setAtom(atom);
    module->setScript(NULL);
    return module;
}
