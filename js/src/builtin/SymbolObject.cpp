





#include "builtin/SymbolObject.h"

#include "vm/StringBuffer.h"
#include "vm/Symbol.h"

#include "jsobjinlines.h"

#include "vm/NativeObject-inl.h"

using JS::Symbol;
using namespace js;

const Class SymbolObject::class_ = {
    "Symbol",
    JSCLASS_HAS_RESERVED_SLOTS(RESERVED_SLOTS) | JSCLASS_HAS_CACHED_PROTO(JSProto_Symbol),
    nullptr, 
    nullptr, 
    nullptr, 
    nullptr, 
    nullptr, 
    nullptr, 
    nullptr, 
    convert
};

SymbolObject*
SymbolObject::create(JSContext* cx, JS::HandleSymbol symbol)
{
    JSObject* obj = NewBuiltinClassInstance(cx, &class_);
    if (!obj)
        return nullptr;
    SymbolObject& symobj = obj->as<SymbolObject>();
    symobj.setPrimitiveValue(symbol);
    return &symobj;
}

const JSPropertySpec SymbolObject::properties[] = {
    JS_PS_END
};

const JSFunctionSpec SymbolObject::methods[] = {
    JS_FN(js_toString_str, toString, 0, 0),
    JS_FN(js_valueOf_str, valueOf, 0, 0),
    JS_FS_END
};

const JSFunctionSpec SymbolObject::staticMethods[] = {
    JS_FN("for", for_, 1, 0),
    JS_FN("keyFor", keyFor, 1, 0),
    JS_FS_END
};

JSObject*
SymbolObject::initClass(JSContext* cx, HandleObject obj)
{
    Rooted<GlobalObject*> global(cx, &obj->as<GlobalObject>());

    
    
    
    RootedObject proto(cx, global->createBlankPrototype<PlainObject>(cx));
    if (!proto)
        return nullptr;

    RootedFunction ctor(cx, global->createConstructor(cx, construct,
                                                      ClassName(JSProto_Symbol, cx), 1));
    if (!ctor)
        return nullptr;

    
    ImmutablePropertyNamePtr* names = &cx->names().iterator;
    RootedValue value(cx);
    unsigned attrs = JSPROP_READONLY | JSPROP_PERMANENT;
    WellKnownSymbols* wks = cx->runtime()->wellKnownSymbols;
    for (size_t i = 0; i < JS::WellKnownSymbolLimit; i++) {
        value.setSymbol(wks->get(i));
        if (!NativeDefineProperty(cx, ctor, names[i], value, nullptr, nullptr, attrs))
            return nullptr;
    }

    if (!LinkConstructorAndPrototype(cx, ctor, proto) ||
        !DefinePropertiesAndFunctions(cx, proto, properties, methods) ||
        !DefinePropertiesAndFunctions(cx, ctor, nullptr, staticMethods) ||
        !GlobalObject::initBuiltinConstructor(cx, global, JSProto_Symbol, ctor, proto))
    {
        return nullptr;
    }
    return proto;
}


bool
SymbolObject::construct(JSContext* cx, unsigned argc, Value* vp)
{
    
    
    
    
    CallArgs args = CallArgsFromVp(argc, vp);
    if (args.isConstructing()) {
        JS_ReportErrorNumber(cx, GetErrorMessage, nullptr, JSMSG_NOT_CONSTRUCTOR, "Symbol");
        return false;
    }

    
    RootedString desc(cx);
    if (!args.get(0).isUndefined()) {
        desc = ToString(cx, args.get(0));
        if (!desc)
            return false;
    }

    
    RootedSymbol symbol(cx, JS::Symbol::new_(cx, JS::SymbolCode::UniqueSymbol, desc));
    if (!symbol)
        return false;
    args.rval().setSymbol(symbol);
    return true;
}


bool
SymbolObject::convert(JSContext* cx, HandleObject obj, JSType hint, MutableHandleValue vp)
{
    vp.setSymbol(obj->as<SymbolObject>().unbox());
    return true;
}


bool
SymbolObject::for_(JSContext* cx, unsigned argc, Value* vp)
{
    CallArgs args = CallArgsFromVp(argc, vp);

    
    RootedString stringKey(cx, ToString(cx, args.get(0)));
    if (!stringKey)
        return false;

    
    JS::Symbol* symbol = JS::Symbol::for_(cx, stringKey);
    if (!symbol)
        return false;
    args.rval().setSymbol(symbol);
    return true;
}


bool
SymbolObject::keyFor(JSContext* cx, unsigned argc, Value* vp)
{
    CallArgs args = CallArgsFromVp(argc, vp);

    
    HandleValue arg = args.get(0);
    if (!arg.isSymbol()) {
        ReportValueErrorFlags(cx, JSREPORT_ERROR, JSMSG_UNEXPECTED_TYPE, JSDVG_SEARCH_STACK,
                              arg, js::NullPtr(), "not a symbol", nullptr);
        return false;
    }

    
    if (arg.toSymbol()->code() == JS::SymbolCode::InSymbolRegistry) {
#ifdef DEBUG
        RootedString desc(cx, arg.toSymbol()->description());
        MOZ_ASSERT(Symbol::for_(cx, desc) == arg.toSymbol());
#endif
        args.rval().setString(arg.toSymbol()->description());
        return true;
    }

    
    
    args.rval().setUndefined();
    return true;
}

MOZ_ALWAYS_INLINE bool
IsSymbol(HandleValue v)
{
    return v.isSymbol() || (v.isObject() && v.toObject().is<SymbolObject>());
}


bool
SymbolObject::toString_impl(JSContext* cx, CallArgs args)
{
    
    HandleValue thisv = args.thisv();
    MOZ_ASSERT(IsSymbol(thisv));
    Rooted<Symbol*> sym(cx, thisv.isSymbol()
                            ? thisv.toSymbol()
                            : thisv.toObject().as<SymbolObject>().unbox());

    
    return SymbolDescriptiveString(cx, sym, args.rval());
}

bool
SymbolObject::toString(JSContext* cx, unsigned argc, Value* vp)
{
    CallArgs args = CallArgsFromVp(argc, vp);
    return CallNonGenericMethod<IsSymbol, toString_impl>(cx, args);
}


bool
SymbolObject::valueOf_impl(JSContext* cx, CallArgs args)
{
    
    HandleValue thisv = args.thisv();
    MOZ_ASSERT(IsSymbol(thisv));
    if (thisv.isSymbol())
        args.rval().set(thisv);
    else
        args.rval().setSymbol(thisv.toObject().as<SymbolObject>().unbox());
    return true;
}

bool
SymbolObject::valueOf(JSContext* cx, unsigned argc, Value* vp)
{
    CallArgs args = CallArgsFromVp(argc, vp);
    return CallNonGenericMethod<IsSymbol, valueOf_impl>(cx, args);
}

JSObject*
js::InitSymbolClass(JSContext* cx, HandleObject obj)
{
    return SymbolObject::initClass(cx, obj);
}
