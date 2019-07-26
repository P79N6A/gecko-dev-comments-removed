





#include "builtin/SymbolObject.h"

#include "jsobjinlines.h"

#include "vm/Symbol-inl.h"

using namespace js;

const Class SymbolObject::class_ = {
    "Symbol",
    JSCLASS_HAS_RESERVED_SLOTS(RESERVED_SLOTS) | JSCLASS_HAS_CACHED_PROTO(JSProto_Symbol),
    JS_PropertyStub,         
    JS_DeletePropertyStub,   
    JS_PropertyStub,         
    JS_StrictPropertyStub,   
    JS_EnumerateStub,
    JS_ResolveStub,
    JS_ConvertStub
};

SymbolObject *
SymbolObject::create(JSContext *cx, JS::Symbol *symbol)
{
    JSObject *obj = NewBuiltinClassInstance(cx, &class_);
    if (!obj)
        return nullptr;
    SymbolObject &symobj = obj->as<SymbolObject>();
    symobj.setPrimitiveValue(symbol);
    return &symobj;
}

const JSPropertySpec SymbolObject::properties[] = {
    JS_PS_END
};

const JSFunctionSpec SymbolObject::methods[] = {
    JS_FS_END
};

const JSFunctionSpec SymbolObject::staticMethods[] = {
    JS_FS_END
};

JSObject *
SymbolObject::initClass(JSContext *cx, HandleObject obj)
{
    Rooted<GlobalObject*> global(cx, &obj->as<GlobalObject>());

    
    
    
    RootedObject proto(cx, global->createBlankPrototype(cx, &JSObject::class_));
    if (!proto)
        return nullptr;

    RootedFunction ctor(cx, global->createConstructor(cx, construct,
                                                      ClassName(JSProto_Symbol, cx), 1));
    if (!ctor ||
        !LinkConstructorAndPrototype(cx, ctor, proto) ||
        !DefinePropertiesAndFunctions(cx, proto, properties, methods) ||
        !DefinePropertiesAndFunctions(cx, ctor, nullptr, staticMethods) ||
        !GlobalObject::initBuiltinConstructor(cx, global, JSProto_Symbol, ctor, proto))
    {
        return nullptr;
    }
    return proto;
}


bool
SymbolObject::construct(JSContext *cx, unsigned argc, Value *vp)
{
    
    
    
    
    CallArgs args = CallArgsFromVp(argc, vp);
    if (args.isConstructing()) {
        JS_ReportErrorNumber(cx, js_GetErrorMessage, nullptr, JSMSG_NOT_CONSTRUCTOR, "Symbol");
        return false;
    }

    
    RootedString desc(cx);
    if (!args.get(0).isUndefined()) {
        desc = ToString(cx, args.get(0));
        if (!desc)
            return false;
    }

    
    RootedSymbol symbol(cx, JS::Symbol::new_(cx, desc));
    if (!symbol)
        return false;
    args.rval().setSymbol(symbol);
    return true;
}

JSObject *
js_InitSymbolClass(JSContext *cx, HandleObject obj)
{
    return SymbolObject::initClass(cx, obj);
}
