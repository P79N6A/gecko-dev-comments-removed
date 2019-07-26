









#include <stdlib.h>
#include <string.h>

#include "mozilla/Util.h"

#include "jstypes.h"
#include "jsutil.h"
#include "jsprf.h"
#include "jsapi.h"
#include "jsarray.h"
#include "jsatom.h"
#include "jsbool.h"
#include "jscntxt.h"
#include "jsdate.h"
#include "jsversion.h"
#include "jsfun.h"
#include "jsgc.h"
#include "jsinterp.h"
#include "jsiter.h"
#include "jslock.h"
#include "jsmath.h"
#include "jsnum.h"
#include "jsobj.h"
#include "jsonparser.h"
#include "jsopcode.h"
#include "jsprobes.h"
#include "jsprototypes.h"
#include "jsproxy.h"
#include "jsscript.h"
#include "jsstr.h"
#include "jsdbgapi.h"
#include "json.h"
#include "jswatchpoint.h"
#include "jswrapper.h"

#include "builtin/MapObject.h"
#include "builtin/Module.h"
#include "builtin/ParallelArray.h"
#include "frontend/BytecodeCompiler.h"
#include "frontend/Parser.h"
#include "gc/Marking.h"
#include "js/MemoryMetrics.h"
#include "vm/Shape.h"
#include "vm/StringBuffer.h"
#include "vm/Xdr.h"

#include "jsatominlines.h"
#include "jsboolinlines.h"
#include "jscntxtinlines.h"
#include "jsinterpinlines.h"
#include "jsobjinlines.h"
#include "jsscriptinlines.h"
#include "jscompartmentinlines.h"

#include "vm/BooleanObject-inl.h"
#include "vm/NumberObject-inl.h"
#include "vm/Shape-inl.h"
#include "vm/StringObject-inl.h"

#include "jsautooplen.h"

using namespace js;
using namespace js::gc;
using namespace js::types;

using js::frontend::IsIdentifier;
using mozilla::ArrayLength;

JS_STATIC_ASSERT(int32_t((JSObject::NELEMENTS_LIMIT - 1) * sizeof(Value)) == int64_t((JSObject::NELEMENTS_LIMIT - 1) * sizeof(Value)));

Class js::ObjectClass = {
    js_Object_str,
    JSCLASS_HAS_CACHED_PROTO(JSProto_Object),
    JS_PropertyStub,         
    JS_PropertyStub,         
    JS_PropertyStub,         
    JS_StrictPropertyStub,   
    JS_EnumerateStub,
    JS_ResolveStub,
    JS_ConvertStub
};

JS_FRIEND_API(JSObject *)
JS_ObjectToInnerObject(JSContext *cx, JSObject *objArg)
{
    RootedObject obj(cx, objArg);
    if (!obj) {
        JS_ReportErrorNumber(cx, js_GetErrorMessage, NULL, JSMSG_INACTIVE);
        return NULL;
    }
    return GetInnerObject(cx, obj);
}

JS_FRIEND_API(JSObject *)
JS_ObjectToOuterObject(JSContext *cx, JSObject *obj_)
{
    Rooted<JSObject*> obj(cx, obj_);
    assertSameCompartment(cx, obj);
    return GetOuterObject(cx, obj);
}

JSObject *
js::NonNullObject(JSContext *cx, const Value &v)
{
    if (v.isPrimitive()) {
        JS_ReportErrorNumber(cx, js_GetErrorMessage, NULL, JSMSG_NOT_NONNULL_OBJECT);
        return NULL;
    }
    return &v.toObject();
}

const char *
js::InformalValueTypeName(const Value &v)
{
    if (v.isObject())
        return v.toObject().getClass()->name;
    if (v.isString())
        return "string";
    if (v.isNumber())
        return "number";
    if (v.isBoolean())
        return "boolean";
    if (v.isNull())
        return "null";
    if (v.isUndefined())
        return "undefined";
    return "value";
}

template <AllowGC allowGC>
JSBool
js::HasOwnProperty(JSContext *cx, LookupGenericOp lookup,
                   typename MaybeRooted<JSObject*, allowGC>::HandleType obj,
                   typename MaybeRooted<jsid, allowGC>::HandleType id,
                   typename MaybeRooted<JSObject*, allowGC>::MutableHandleType objp,
                   typename MaybeRooted<Shape*, allowGC>::MutableHandleType propp)
{
    JSAutoResolveFlags rf(cx, 0);
    if (lookup) {
        if (!allowGC)
            return false;
        if (!lookup(cx,
                    MaybeRooted<JSObject*, allowGC>::toHandle(obj),
                    MaybeRooted<jsid, allowGC>::toHandle(id),
                    MaybeRooted<JSObject*, allowGC>::toMutableHandle(objp),
                    MaybeRooted<Shape*, allowGC>::toMutableHandle(propp)))
        {
            return false;
        }
    } else {
        if (!baseops::LookupProperty<allowGC>(cx, obj, id, objp, propp))
            return false;
    }
    if (!propp)
        return true;

    if (objp == obj)
        return true;

    JSObject *outer = NULL;
    if (JSObjectOp op = objp->getClass()->ext.outerObject) {
        if (!allowGC)
            return false;
        RootedObject inner(cx, objp);
        outer = op(cx, inner);
        if (!outer)
            return false;
    }

    if (outer != objp)
        propp.set(NULL);
    return true;
}

template JSBool
js::HasOwnProperty<CanGC>(JSContext *cx, LookupGenericOp lookup,
                          HandleObject obj, HandleId id,
                          MutableHandleObject objp, MutableHandleShape propp);

template JSBool
js::HasOwnProperty<NoGC>(JSContext *cx, LookupGenericOp lookup,
                         JSObject *obj, jsid id,
                         FakeMutableHandle<JSObject*> objp, FakeMutableHandle<Shape*> propp);

bool
js::NewPropertyDescriptorObject(JSContext *cx, const PropertyDescriptor *desc,
                                MutableHandleValue vp)
{
    if (!desc->obj) {
        vp.setUndefined();
        return true;
    }

    
    PropDesc d;
    PropDesc::AutoRooter dRoot(cx, &d);

    d.initFromPropertyDescriptor(*desc);
    if (!d.makeObject(cx))
        return false;
    vp.set(d.pd());
    return true;
}

void
PropDesc::initFromPropertyDescriptor(const PropertyDescriptor &desc)
{
    isUndefined_ = false;
    pd_.setUndefined();
    attrs = uint8_t(desc.attrs);
    JS_ASSERT_IF(attrs & JSPROP_READONLY, !(attrs & (JSPROP_GETTER | JSPROP_SETTER)));
    if (desc.attrs & (JSPROP_GETTER | JSPROP_SETTER)) {
        hasGet_ = true;
        get_ = ((desc.attrs & JSPROP_GETTER) && desc.getter)
               ? CastAsObjectJsval(desc.getter)
               : UndefinedValue();
        hasSet_ = true;
        set_ = ((desc.attrs & JSPROP_SETTER) && desc.setter)
               ? CastAsObjectJsval(desc.setter)
               : UndefinedValue();
        hasValue_ = false;
        value_.setUndefined();
        hasWritable_ = false;
    } else {
        hasGet_ = false;
        get_.setUndefined();
        hasSet_ = false;
        set_.setUndefined();
        hasValue_ = true;
        value_ = desc.value;
        hasWritable_ = true;
    }
    hasEnumerable_ = true;
    hasConfigurable_ = true;
}

bool
PropDesc::makeObject(JSContext *cx)
{
    MOZ_ASSERT(!isUndefined());

    RootedObject obj(cx, NewBuiltinClassInstance(cx, &ObjectClass));
    if (!obj)
        return false;

    const JSAtomState &names = cx->names();
    RootedValue configurableVal(cx, BooleanValue((attrs & JSPROP_PERMANENT) == 0));
    RootedValue enumerableVal(cx, BooleanValue((attrs & JSPROP_ENUMERATE) != 0));
    RootedValue writableVal(cx, BooleanValue((attrs & JSPROP_READONLY) == 0));
    if ((hasConfigurable() &&
         !JSObject::defineProperty(cx, obj, names.configurable, configurableVal)) ||
        (hasEnumerable() &&
         !JSObject::defineProperty(cx, obj, names.enumerable, enumerableVal)) ||
        (hasGet() &&
         !JSObject::defineProperty(cx, obj, names.get, getterValue())) ||
        (hasSet() &&
         !JSObject::defineProperty(cx, obj, names.set, setterValue())) ||
        (hasValue() &&
         !JSObject::defineProperty(cx, obj, names.value, value())) ||
        (hasWritable() &&
         !JSObject::defineProperty(cx, obj, names.writable, writableVal)))
    {
        return false;
    }

    pd_.setObject(*obj);
    return true;
}

bool
js::GetOwnPropertyDescriptor(JSContext *cx, HandleObject obj, HandleId id,
                             PropertyDescriptor *desc)
{
    
    if (obj->isProxy())
        return Proxy::getOwnPropertyDescriptor(cx, obj, id, desc, 0);

    RootedObject pobj(cx);
    RootedShape shape(cx);
    if (!HasOwnProperty<CanGC>(cx, obj->getOps()->lookupGeneric, obj, id, &pobj, &shape))
        return false;
    if (!shape) {
        desc->obj = NULL;
        return true;
    }

    bool doGet = true;
    if (pobj->isNative()) {
        desc->attrs = GetShapeAttributes(shape);
        if (desc->attrs & (JSPROP_GETTER | JSPROP_SETTER)) {
            doGet = false;
            if (desc->attrs & JSPROP_GETTER)
                desc->getter = CastAsPropertyOp(shape->getterObject());
            if (desc->attrs & JSPROP_SETTER)
                desc->setter = CastAsStrictPropertyOp(shape->setterObject());
        }
    } else {
        if (!JSObject::getGenericAttributes(cx, pobj, id, &desc->attrs))
            return false;
    }

    RootedValue value(cx);
    if (doGet && !JSObject::getGeneric(cx, obj, obj, id, &value))
        return false;

    desc->value = value;
    desc->obj = obj;
    return true;
}

bool
js::GetOwnPropertyDescriptor(JSContext *cx, HandleObject obj, HandleId id, MutableHandleValue vp)
{
    AutoPropertyDescriptorRooter desc(cx);
    return GetOwnPropertyDescriptor(cx, obj, id, &desc) &&
           NewPropertyDescriptorObject(cx, &desc, vp);
}

bool
js::GetFirstArgumentAsObject(JSContext *cx, unsigned argc, Value *vp, const char *method,
                             MutableHandleObject objp)
{
    if (argc == 0) {
        JS_ReportErrorNumber(cx, js_GetErrorMessage, NULL, JSMSG_MORE_ARGS_NEEDED,
                             method, "0", "s");
        return false;
    }

    RootedValue v(cx, vp[2]);
    if (!v.isObject()) {
        char *bytes = DecompileValueGenerator(cx, JSDVG_SEARCH_STACK, v, NullPtr());
        if (!bytes)
            return false;
        JS_ReportErrorNumber(cx, js_GetErrorMessage, NULL, JSMSG_UNEXPECTED_TYPE,
                             bytes, "not an object");
        js_free(bytes);
        return false;
    }

    objp.set(&v.toObject());
    return true;
}

static bool
HasProperty(JSContext *cx, HandleObject obj, HandleId id, MutableHandleValue vp, bool *foundp)
{
    if (!JSObject::hasProperty(cx, obj, id, foundp, 0))
        return false;
    if (!*foundp) {
        vp.setUndefined();
        return true;
    }

    





    return !!JSObject::getGeneric(cx, obj, obj, id, vp);
}

bool
PropDesc::initialize(JSContext *cx, const Value &origval, bool checkAccessors)
{
    RootedValue v(cx, origval);

    
    if (v.isPrimitive()) {
        JS_ReportErrorNumber(cx, js_GetErrorMessage, NULL, JSMSG_NOT_NONNULL_OBJECT);
        return false;
    }
    RootedObject desc(cx, &v.toObject());

    
    pd_ = v;

    isUndefined_ = false;

    



    attrs = JSPROP_PERMANENT | JSPROP_READONLY;

    bool found = false;
    RootedId id(cx);

    
    id = NameToId(cx->names().enumerable);
    if (!HasProperty(cx, desc, id, &v, &found))
        return false;
    if (found) {
        hasEnumerable_ = true;
        if (ToBoolean(v))
            attrs |= JSPROP_ENUMERATE;
    }

    
    id = NameToId(cx->names().configurable);
    if (!HasProperty(cx, desc, id, &v, &found))
        return false;
    if (found) {
        hasConfigurable_ = true;
        if (ToBoolean(v))
            attrs &= ~JSPROP_PERMANENT;
    }

    
    id = NameToId(cx->names().value);
    if (!HasProperty(cx, desc, id, &v, &found))
        return false;
    if (found) {
        hasValue_ = true;
        value_ = v;
    }

    
    id = NameToId(cx->names().writable);
    if (!HasProperty(cx, desc, id, &v, &found))
        return false;
    if (found) {
        hasWritable_ = true;
        if (ToBoolean(v))
            attrs &= ~JSPROP_READONLY;
    }

    
    id = NameToId(cx->names().get);
    if (!HasProperty(cx, desc, id, &v, &found))
        return false;
    if (found) {
        hasGet_ = true;
        get_ = v;
        attrs |= JSPROP_GETTER | JSPROP_SHARED;
        attrs &= ~JSPROP_READONLY;
        if (checkAccessors && !checkGetter(cx))
            return false;
    }

    
    id = NameToId(cx->names().set);
    if (!HasProperty(cx, desc, id, &v, &found))
        return false;
    if (found) {
        hasSet_ = true;
        set_ = v;
        attrs |= JSPROP_SETTER | JSPROP_SHARED;
        attrs &= ~JSPROP_READONLY;
        if (checkAccessors && !checkSetter(cx))
            return false;
    }

    
    if ((hasGet() || hasSet()) && (hasValue() || hasWritable())) {
        JS_ReportErrorNumber(cx, js_GetErrorMessage, NULL, JSMSG_INVALID_DESCRIPTOR);
        return false;
    }

    JS_ASSERT_IF(attrs & JSPROP_READONLY, !(attrs & (JSPROP_GETTER | JSPROP_SETTER)));

    return true;
}

void
PropDesc::complete()
{
    if (isGenericDescriptor() || isDataDescriptor()) {
        if (!hasValue_) {
            hasValue_ = true;
            value_.setUndefined();
        }
        if (!hasWritable_) {
            hasWritable_ = true;
            attrs |= JSPROP_READONLY;
        }
    } else {
        if (!hasGet_) {
            hasGet_ = true;
            get_.setUndefined();
        }
        if (!hasSet_) {
            hasSet_ = true;
            set_.setUndefined();
        }
    }
    if (!hasEnumerable_) {
        hasEnumerable_ = true;
        attrs &= ~JSPROP_ENUMERATE;
    }
    if (!hasConfigurable_) {
        hasConfigurable_ = true;
        attrs |= JSPROP_PERMANENT;
    }
}

bool
js::Throw(JSContext *cx, jsid id, unsigned errorNumber)
{
    JS_ASSERT(js_ErrorFormatString[errorNumber].argCount == 1);

    JSString *idstr = IdToString(cx, id);
    if (!idstr)
       return false;
    JSAutoByteString bytes(cx, idstr);
    if (!bytes)
        return false;
    JS_ReportErrorNumber(cx, js_GetErrorMessage, NULL, errorNumber, bytes.ptr());
    return false;
}

bool
js::Throw(JSContext *cx, JSObject *obj, unsigned errorNumber)
{
    if (js_ErrorFormatString[errorNumber].argCount == 1) {
        RootedValue val(cx, ObjectValue(*obj));
        js_ReportValueErrorFlags(cx, JSREPORT_ERROR, errorNumber,
                                 JSDVG_IGNORE_STACK, val, NullPtr(),
                                 NULL, NULL);
    } else {
        JS_ASSERT(js_ErrorFormatString[errorNumber].argCount == 0);
        JS_ReportErrorNumber(cx, js_GetErrorMessage, NULL, errorNumber);
    }
    return false;
}

static JSBool
Reject(JSContext *cx, unsigned errorNumber, bool throwError, jsid id, bool *rval)
{
    if (throwError)
        return Throw(cx, id, errorNumber);

    *rval = false;
    return true;
}

static JSBool
Reject(JSContext *cx, JSObject *obj, unsigned errorNumber, bool throwError, bool *rval)
{
    if (throwError)
        return Throw(cx, obj, errorNumber);

    *rval = false;
    return JS_TRUE;
}





bool
js::CheckDefineProperty(JSContext *cx, HandleObject obj, HandleId id, HandleValue value,
                        PropertyOp getter, StrictPropertyOp setter, unsigned attrs)
{
    if (!obj->isNative())
        return true;

    
    
    AutoPropertyDescriptorRooter desc(cx);
    if (!GetOwnPropertyDescriptor(cx, obj, id, &desc))
        return false;

    
    
    
    if (desc.obj && (desc.attrs & JSPROP_PERMANENT)) {
        
        
        
        if (getter != desc.getter ||
            setter != desc.setter ||
            (attrs != desc.attrs && attrs != (desc.attrs | JSPROP_READONLY)))
        {
            return Throw(cx, id, JSMSG_CANT_REDEFINE_PROP);
        }

        
        
        if ((desc.attrs & (JSPROP_GETTER | JSPROP_SETTER | JSPROP_READONLY)) == JSPROP_READONLY) {
            bool same;
            if (!SameValue(cx, value, desc.value, &same))
                return false;
            if (!same)
                return JSObject::reportReadOnly(cx, id);
        }
    }
    return true;
}

static JSBool
DefinePropertyOnObject(JSContext *cx, HandleObject obj, HandleId id, const PropDesc &desc,
                       bool throwError, bool *rval)
{
    
    RootedShape shape(cx);
    RootedObject obj2(cx);
    JS_ASSERT(!obj->getOps()->lookupGeneric);
    if (!HasOwnProperty<CanGC>(cx, NULL, obj, id, &obj2, &shape))
        return JS_FALSE;

    JS_ASSERT(!obj->getOps()->defineProperty);

    
    if (!shape) {
        if (!obj->isExtensible())
            return Reject(cx, obj, JSMSG_OBJECT_NOT_EXTENSIBLE, throwError, rval);

        *rval = true;

        if (desc.isGenericDescriptor() || desc.isDataDescriptor()) {
            JS_ASSERT(!obj->getOps()->defineProperty);
            RootedValue v(cx, desc.hasValue() ? desc.value() : UndefinedValue());
            return baseops::DefineGeneric(cx, obj, id, v,
                                          JS_PropertyStub, JS_StrictPropertyStub,
                                          desc.attributes());
        }

        JS_ASSERT(desc.isAccessorDescriptor());

        



        RootedValue dummy(cx);
        unsigned dummyAttrs;
        if (!CheckAccess(cx, obj, id, JSACC_WATCH, &dummy, &dummyAttrs))
            return JS_FALSE;

        RootedValue tmp(cx, UndefinedValue());
        return baseops::DefineGeneric(cx, obj, id, tmp,
                                      desc.getter(), desc.setter(), desc.attributes());
    }

    
    RootedValue v(cx, UndefinedValue());

    JS_ASSERT(obj == obj2);

    bool shapeDataDescriptor = true,
         shapeAccessorDescriptor = false,
         shapeWritable = true,
         shapeConfigurable = true,
         shapeEnumerable = true,
         shapeHasDefaultGetter = true,
         shapeHasDefaultSetter = true,
         shapeHasGetterValue = false,
         shapeHasSetterValue = false;
    uint8_t shapeAttributes = JSPROP_ENUMERATE;
    if (!IsImplicitDenseElement(shape)) {
        shapeDataDescriptor = shape->isDataDescriptor();
        shapeAccessorDescriptor = shape->isAccessorDescriptor();
        shapeWritable = shape->writable();
        shapeConfigurable = shape->configurable();
        shapeEnumerable = shape->enumerable();
        shapeHasDefaultGetter = shape->hasDefaultGetter();
        shapeHasDefaultSetter = shape->hasDefaultSetter();
        shapeHasGetterValue = shape->hasGetterValue();
        shapeHasSetterValue = shape->hasSetterValue();
        shapeAttributes = shape->attributes();
    }

    do {
        if (desc.isAccessorDescriptor()) {
            if (!shapeAccessorDescriptor)
                break;

            if (desc.hasGet()) {
                bool same;
                if (!SameValue(cx, desc.getterValue(), shape->getterOrUndefined(), &same))
                    return false;
                if (!same)
                    break;
            }

            if (desc.hasSet()) {
                bool same;
                if (!SameValue(cx, desc.setterValue(), shape->setterOrUndefined(), &same))
                    return false;
                if (!same)
                    break;
            }
        } else {
            






            if (IsImplicitDenseElement(shape)) {
                v = obj->getDenseElement(JSID_TO_INT(id));
            } else if (shape->isDataDescriptor()) {
                









                if (!shape->configurable() &&
                    (!shape->hasDefaultGetter() || !shape->hasDefaultSetter()) &&
                    desc.isDataDescriptor() &&
                    (desc.hasWritable() ? desc.writable() : shape->writable()))
                {
                    return Reject(cx, JSMSG_CANT_REDEFINE_PROP, throwError, id, rval);
                }

                if (!js_NativeGet(cx, obj, obj2, shape, 0, &v))
                    return JS_FALSE;
            }

            if (desc.isDataDescriptor()) {
                if (!shapeDataDescriptor)
                    break;

                bool same;
                if (desc.hasValue()) {
                    if (!SameValue(cx, desc.value(), v, &same))
                        return false;
                    if (!same) {
                        















                        if (!shapeConfigurable &&
                            (!shape->hasDefaultGetter() || !shape->hasDefaultSetter()))
                        {
                            return Reject(cx, JSMSG_CANT_REDEFINE_PROP, throwError, id, rval);
                        }
                        break;
                    }
                }
                if (desc.hasWritable() && desc.writable() != shapeWritable)
                    break;
            } else {
                
                JS_ASSERT(desc.isGenericDescriptor());
            }
        }

        if (desc.hasConfigurable() && desc.configurable() != shapeConfigurable)
            break;
        if (desc.hasEnumerable() && desc.enumerable() != shapeEnumerable)
            break;

        
        *rval = true;
        return true;
    } while (0);

    
    if (!shapeConfigurable) {
        if ((desc.hasConfigurable() && desc.configurable()) ||
            (desc.hasEnumerable() && desc.enumerable() != shape->enumerable())) {
            return Reject(cx, JSMSG_CANT_REDEFINE_PROP, throwError, id, rval);
        }
    }

    bool callDelProperty = false;

    if (desc.isGenericDescriptor()) {
        
    } else if (desc.isDataDescriptor() != shapeDataDescriptor) {
        
        if (!shapeConfigurable)
            return Reject(cx, JSMSG_CANT_REDEFINE_PROP, throwError, id, rval);
    } else if (desc.isDataDescriptor()) {
        
        JS_ASSERT(shapeDataDescriptor);
        if (!shapeConfigurable && !shape->writable()) {
            if (desc.hasWritable() && desc.writable())
                return Reject(cx, JSMSG_CANT_REDEFINE_PROP, throwError, id, rval);
            if (desc.hasValue()) {
                bool same;
                if (!SameValue(cx, desc.value(), v, &same))
                    return false;
                if (!same)
                    return Reject(cx, JSMSG_CANT_REDEFINE_PROP, throwError, id, rval);
            }
        }

        callDelProperty = !shapeHasDefaultGetter || !shapeHasDefaultSetter;
    } else {
        
        JS_ASSERT(desc.isAccessorDescriptor() && shape->isAccessorDescriptor());
        if (!shape->configurable()) {
            if (desc.hasSet()) {
                bool same;
                if (!SameValue(cx, desc.setterValue(), shape->setterOrUndefined(), &same))
                    return false;
                if (!same)
                    return Reject(cx, JSMSG_CANT_REDEFINE_PROP, throwError, id, rval);
            }

            if (desc.hasGet()) {
                bool same;
                if (!SameValue(cx, desc.getterValue(), shape->getterOrUndefined(), &same))
                    return false;
                if (!same)
                    return Reject(cx, JSMSG_CANT_REDEFINE_PROP, throwError, id, rval);
            }
        }
    }

    
    unsigned attrs;
    PropertyOp getter;
    StrictPropertyOp setter;
    if (desc.isGenericDescriptor()) {
        unsigned changed = 0;
        if (desc.hasConfigurable())
            changed |= JSPROP_PERMANENT;
        if (desc.hasEnumerable())
            changed |= JSPROP_ENUMERATE;

        attrs = (shapeAttributes & ~changed) | (desc.attributes() & changed);
        getter = IsImplicitDenseElement(shape) ? JS_PropertyStub : shape->getter();
        setter = IsImplicitDenseElement(shape) ? JS_StrictPropertyStub : shape->setter();
    } else if (desc.isDataDescriptor()) {
        unsigned unchanged = 0;
        if (!desc.hasConfigurable())
            unchanged |= JSPROP_PERMANENT;
        if (!desc.hasEnumerable())
            unchanged |= JSPROP_ENUMERATE;
        
        if (!desc.hasWritable() && shapeDataDescriptor)
            unchanged |= JSPROP_READONLY;

        if (desc.hasValue())
            v = desc.value();
        attrs = (desc.attributes() & ~unchanged) | (shapeAttributes & unchanged);
        getter = JS_PropertyStub;
        setter = JS_StrictPropertyStub;
    } else {
        JS_ASSERT(desc.isAccessorDescriptor());

        



        RootedValue dummy(cx);
        if (!CheckAccess(cx, obj2, id, JSACC_WATCH, &dummy, &attrs))
             return JS_FALSE;

        
        unsigned changed = 0;
        if (desc.hasConfigurable())
            changed |= JSPROP_PERMANENT;
        if (desc.hasEnumerable())
            changed |= JSPROP_ENUMERATE;
        if (desc.hasGet())
            changed |= JSPROP_GETTER | JSPROP_SHARED | JSPROP_READONLY;
        if (desc.hasSet())
            changed |= JSPROP_SETTER | JSPROP_SHARED | JSPROP_READONLY;

        attrs = (desc.attributes() & changed) | (shapeAttributes & ~changed);
        if (desc.hasGet()) {
            getter = desc.getter();
        } else {
            getter = (shapeHasDefaultGetter && !shapeHasGetterValue)
                     ? JS_PropertyStub
                     : shape->getter();
        }
        if (desc.hasSet()) {
            setter = desc.setter();
        } else {
            setter = (shapeHasDefaultSetter && !shapeHasSetterValue)
                     ? JS_StrictPropertyStub
                     : shape->setter();
        }
    }

    *rval = true;

    








    if (callDelProperty) {
        RootedValue dummy(cx, UndefinedValue());
        if (!CallJSPropertyOp(cx, obj2->getClass()->delProperty, obj2, id, &dummy))
            return false;
    }

    return baseops::DefineGeneric(cx, obj, id, v, getter, setter, attrs);
}

static JSBool
DefinePropertyOnArray(JSContext *cx, HandleObject obj, HandleId id, const PropDesc &desc,
                      bool throwError, bool *rval)
{
    
    

    if (JSID_IS_ATOM(id, cx->names().length)) {
        






        JS_ReportErrorNumber(cx, js_GetErrorMessage, NULL, JSMSG_CANT_DEFINE_ARRAY_LENGTH);
        return JS_FALSE;
    }

    uint32_t index;
    if (js_IdIsIndex(id, &index)) {
        




        if (!DefinePropertyOnObject(cx, obj, id, desc, false, rval))
            return JS_FALSE;
        if (!*rval)
            return Reject(cx, obj, JSMSG_CANT_DEFINE_ARRAY_INDEX, throwError, rval);

        *rval = true;
        return JS_TRUE;
    }

    return DefinePropertyOnObject(cx, obj, id, desc, throwError, rval);
}

bool
js::DefineProperty(JSContext *cx, HandleObject obj, HandleId id, const PropDesc &desc,
                   bool throwError, bool *rval)
{
    if (obj->isArray())
        return DefinePropertyOnArray(cx, obj, id, desc, throwError, rval);

    if (obj->getOps()->lookupGeneric) {
        



        if (obj->isProxy())
            return Proxy::defineProperty(cx, obj, id, desc.pd());
        return Reject(cx, obj, JSMSG_OBJECT_NOT_EXTENSIBLE, throwError, rval);
    }

    return DefinePropertyOnObject(cx, obj, id, desc, throwError, rval);
}

JSBool
js_DefineOwnProperty(JSContext *cx, HandleObject obj, HandleId id, const Value &descriptor,
                     JSBool *bp)
{
    AutoPropDescArrayRooter descs(cx);
    PropDesc *desc = descs.append();
    if (!desc || !desc->initialize(cx, descriptor))
        return false;

    bool rval;
    if (!DefineProperty(cx, obj, id, *desc, true, &rval))
        return false;
    *bp = !!rval;
    return true;
}


bool
js::ReadPropertyDescriptors(JSContext *cx, HandleObject props, bool checkAccessors,
                            AutoIdVector *ids, AutoPropDescArrayRooter *descs)
{
    if (!GetPropertyNames(cx, props, JSITER_OWNONLY, ids))
        return false;

    RootedId id(cx);
    for (size_t i = 0, len = ids->length(); i < len; i++) {
        id = (*ids)[i];
        PropDesc* desc = descs->append();
        RootedValue v(cx);
        if (!desc ||
            !JSObject::getGeneric(cx, props, props, id, &v) ||
            !desc->initialize(cx, v, checkAccessors))
        {
            return false;
        }
    }
    return true;
}


static bool
DefineProperties(JSContext *cx, HandleObject obj, HandleObject props)
{
    AutoIdVector ids(cx);
    AutoPropDescArrayRooter descs(cx);
    if (!ReadPropertyDescriptors(cx, props, true, &ids, &descs))
        return false;

    bool dummy;
    for (size_t i = 0, len = ids.length(); i < len; i++) {
        if (!DefineProperty(cx, obj, ids.handleAt(i), descs[i], true, &dummy))
            return false;
    }

    return true;
}

extern JSBool
js_PopulateObject(JSContext *cx, HandleObject newborn, HandleObject props)
{
    return DefineProperties(cx, newborn, props);
}

 inline unsigned
JSObject::getSealedOrFrozenAttributes(unsigned attrs, ImmutabilityType it)
{
    
    if (it == FREEZE && !(attrs & (JSPROP_GETTER | JSPROP_SETTER)))
        return JSPROP_PERMANENT | JSPROP_READONLY;
    return JSPROP_PERMANENT;
}

 bool
JSObject::sealOrFreeze(JSContext *cx, HandleObject obj, ImmutabilityType it)
{
    assertSameCompartment(cx, obj);
    JS_ASSERT(it == SEAL || it == FREEZE);

    if (obj->isExtensible() && !obj->preventExtensions(cx))
        return false;

    AutoIdVector props(cx);
    if (!GetPropertyNames(cx, obj, JSITER_HIDDEN | JSITER_OWNONLY, &props))
        return false;

    
    JS_ASSERT_IF(obj->isNative(), obj->getDenseCapacity() == 0);

    if (obj->isNative() && !obj->inDictionaryMode()) {
        






        RootedShape last(cx, EmptyShape::getInitialShape(cx, obj->getClass(),
                                                         obj->getTaggedProto(),
                                                         obj->getParent(),
                                                         obj->getAllocKind(),
                                                         obj->lastProperty()->getObjectFlags()));
        if (!last)
            return false;

        
        AutoShapeVector shapes(cx);
        for (Shape::Range<NoGC> r(obj->lastProperty()); !r.empty(); r.popFront()) {
            if (!shapes.append(&r.front()))
                return false;
        }
        Reverse(shapes.begin(), shapes.end());

        for (size_t i = 0; i < shapes.length(); i++) {
            StackShape child(shapes[i]);
            StackShape::AutoRooter rooter(cx, &child);
            child.attrs |= getSealedOrFrozenAttributes(child.attrs, it);

            if (!JSID_IS_EMPTY(child.propid))
                MarkTypePropertyConfigured(cx, obj, child.propid);

            last = cx->propertyTree().getChild(cx, last, obj->numFixedSlots(), child);
            if (!last)
                return false;
        }

        JS_ASSERT(obj->lastProperty()->slotSpan() == last->slotSpan());
        JS_ALWAYS_TRUE(setLastProperty(cx, obj, last));
    } else {
        RootedId id(cx);
        for (size_t i = 0; i < props.length(); i++) {
            id = props[i];

            unsigned attrs;
            if (!getGenericAttributes(cx, obj, id, &attrs))
                return false;

            unsigned new_attrs = getSealedOrFrozenAttributes(attrs, it);

            
            if ((attrs | new_attrs) == attrs)
                continue;

            attrs |= new_attrs;
            if (!setGenericAttributes(cx, obj, id, &attrs))
                return false;
        }
    }

    return true;
}

 bool
JSObject::isSealedOrFrozen(JSContext *cx, HandleObject obj, ImmutabilityType it, bool *resultp)
{
    if (obj->isExtensible()) {
        *resultp = false;
        return true;
    }

    AutoIdVector props(cx);
    if (!GetPropertyNames(cx, obj, JSITER_HIDDEN | JSITER_OWNONLY, &props))
        return false;

    RootedId id(cx);
    for (size_t i = 0, len = props.length(); i < len; i++) {
        id = props[i];

        unsigned attrs;
        if (!getGenericAttributes(cx, obj, id, &attrs))
            return false;

        




        if (!(attrs & JSPROP_PERMANENT) ||
            (it == FREEZE && !(attrs & (JSPROP_READONLY | JSPROP_GETTER | JSPROP_SETTER))))
        {
            *resultp = false;
            return true;
        }
    }

    
    *resultp = true;
    return true;
}





static inline gc::AllocKind
NewObjectGCKind(js::Class *clasp)
{
    if (clasp == &ArrayClass)
        return gc::FINALIZE_OBJECT8;
    if (clasp == &FunctionClass)
        return gc::FINALIZE_OBJECT2;
    return gc::FINALIZE_OBJECT4;
}

static inline JSObject *
NewObject(JSContext *cx, Class *clasp, types::TypeObject *type_, JSObject *parent,
          gc::AllocKind kind, NewObjectKind newKind)
{
    JS_ASSERT(clasp != &ArrayClass);
    JS_ASSERT_IF(clasp == &FunctionClass,
                 kind == JSFunction::FinalizeKind || kind == JSFunction::ExtendedFinalizeKind);
    JS_ASSERT_IF(parent, &parent->global() == cx->compartment->maybeGlobal());

    RootedTypeObject type(cx, type_);

    RootedShape shape(cx, EmptyShape::getInitialShape(cx, clasp, TaggedProto(type->proto),
                                                      parent, kind));
    if (!shape)
        return NULL;

    gc::InitialHeap heap = GetInitialHeap(newKind, clasp);
    JSObject *obj = JSObject::create(cx, kind, heap, shape, type);
    if (!obj)
        return NULL;

    if (newKind == SingletonObject) {
        RootedObject nobj(cx, obj);
        if (!JSObject::setSingletonType(cx, nobj))
            return NULL;
        obj = nobj;
    }

    



    if (clasp->trace && !(clasp->flags & JSCLASS_IMPLEMENTS_BARRIERS))
        cx->runtime->gcIncrementalEnabled = false;

    Probes::createObject(cx, obj);
    return obj;
}

JSObject *
js::NewObjectWithGivenProto(JSContext *cx, js::Class *clasp,
                            js::TaggedProto proto_, JSObject *parent_,
                            gc::AllocKind allocKind, NewObjectKind newKind)
{
    Rooted<TaggedProto> proto(cx, proto_);
    RootedObject parent(cx, parent_);

    if (CanBeFinalizedInBackground(allocKind, clasp))
        allocKind = GetBackgroundAllocKind(allocKind);

    NewObjectCache &cache = cx->runtime->newObjectCache;

    NewObjectCache::EntryIndex entry = -1;
    if (proto.isObject() && newKind != SingletonObject &&
        (!parent || parent == proto.toObject()->getParent()) && !proto.toObject()->isGlobal())
    {
        if (cache.lookupProto(clasp, proto.toObject(), allocKind, &entry)) {
            JSObject *obj = cache.newObjectFromHit(cx, entry, GetInitialHeap(newKind, clasp));
            if (obj)
                return obj;
        }
    }

    types::TypeObject *type = cx->compartment->getNewType(cx, clasp, proto, NULL);
    if (!type)
        return NULL;

    



    if (!parent && proto.isObject())
        parent = proto.toObject()->getParent();

    RootedObject obj(cx, NewObject(cx, clasp, type, parent, allocKind, newKind));
    if (!obj)
        return NULL;

    if (entry != -1 && !obj->hasDynamicSlots())
        cache.fillProto(entry, clasp, proto, allocKind, obj);

    return obj;
}

JSObject *
js::NewObjectWithClassProtoCommon(JSContext *cx, js::Class *clasp, JSObject *protoArg, JSObject *parentArg,
                                  gc::AllocKind allocKind, NewObjectKind newKind)
{
    if (protoArg)
        return NewObjectWithGivenProto(cx, clasp, protoArg, parentArg, allocKind, newKind);

    if (CanBeFinalizedInBackground(allocKind, clasp))
        allocKind = GetBackgroundAllocKind(allocKind);

    if (!parentArg)
        parentArg = cx->global();

    








    JSProtoKey protoKey = GetClassProtoKey(clasp);

    NewObjectCache &cache = cx->runtime->newObjectCache;

    NewObjectCache::EntryIndex entry = -1;
    if (parentArg->isGlobal() && protoKey != JSProto_Null && newKind != SingletonObject) {
        if (cache.lookupGlobal(clasp, &parentArg->asGlobal(), allocKind, &entry)) {
            JSObject *obj = cache.newObjectFromHit(cx, entry, GetInitialHeap(newKind, clasp));
            if (obj)
                return obj;
        }
    }

    RootedObject parent(cx, parentArg);
    RootedObject proto(cx, protoArg);

    if (!FindProto(cx, clasp, &proto))
        return NULL;

    types::TypeObject *type = proto->getNewType(cx, clasp);
    if (!type)
        return NULL;

    JSObject *obj = NewObject(cx, clasp, type, parent, allocKind, newKind);
    if (!obj)
        return NULL;

    if (entry != -1 && !obj->hasDynamicSlots())
        cache.fillGlobal(entry, clasp, &parent->asGlobal(), allocKind, obj);

    return obj;
}

JSObject *
js::NewObjectWithType(JSContext *cx, HandleTypeObject type, JSObject *parent, gc::AllocKind allocKind,
                      NewObjectKind newKind )
{
    JS_ASSERT(type->proto->hasNewType(&ObjectClass, type));
    JS_ASSERT(parent);

    JS_ASSERT(allocKind <= gc::FINALIZE_OBJECT_LAST);
    if (CanBeFinalizedInBackground(allocKind, &ObjectClass))
        allocKind = GetBackgroundAllocKind(allocKind);

    NewObjectCache &cache = cx->runtime->newObjectCache;

    NewObjectCache::EntryIndex entry = -1;
    if (parent == type->proto->getParent() && newKind != SingletonObject) {
        if (cache.lookupType(&ObjectClass, type, allocKind, &entry)) {
            JSObject *obj = cache.newObjectFromHit(cx, entry, GetInitialHeap(newKind, &ObjectClass));
            if (obj)
                return obj;
        }
    }

    JSObject *obj = NewObject(cx, &ObjectClass, type, parent, allocKind, newKind);
    if (!obj)
        return NULL;

    if (entry != -1 && !obj->hasDynamicSlots())
        cache.fillType(entry, &ObjectClass, type, allocKind, obj);

    return obj;
}

bool
js::NewObjectScriptedCall(JSContext *cx, MutableHandleObject pobj)
{
    jsbytecode *pc;
    RootedScript script(cx, cx->stack.currentScript(&pc));
    gc::AllocKind allocKind = NewObjectGCKind(&ObjectClass);
    NewObjectKind newKind = script
                            ? UseNewTypeForInitializer(cx, script, pc, &ObjectClass)
                            : GenericObject;
    RootedObject obj(cx, NewBuiltinClassInstance(cx, &ObjectClass, allocKind, newKind));
    if (!obj)
        return false;

    if (script) {
        
        if (!types::SetInitializerObjectType(cx, script, pc, obj, newKind))
            return false;
    }

    pobj.set(obj);
    return true;
}

JSObject *
js::NewReshapedObject(JSContext *cx, HandleTypeObject type, JSObject *parent,
                      gc::AllocKind kind, HandleShape shape)
{
    RootedObject res(cx, NewObjectWithType(cx, type, parent, kind));
    if (!res)
        return NULL;

    if (shape->isEmptyShape())
        return res;

    
    js::AutoIdVector ids(cx);
    {
        for (unsigned i = 0; i <= shape->slot(); i++) {
            if (!ids.append(JSID_VOID))
                return NULL;
        }
        RawShape nshape = shape;
        while (!nshape->isEmptyShape()) {
            ids[nshape->slot()] = nshape->propid();
            nshape = nshape->previous();
        }
    }

    
    RootedId id(cx);
    RootedValue undefinedValue(cx, UndefinedValue());
    for (unsigned i = 0; i < ids.length(); i++) {
        id = ids[i];
        if (!DefineNativeProperty(cx, res, id, undefinedValue, NULL, NULL,
                                  JSPROP_ENUMERATE, 0, 0, DNP_SKIP_TYPE)) {
            return NULL;
        }
    }
    JS_ASSERT(!res->inDictionaryMode());

    return res;
}

JSObject*
js::CreateThis(JSContext *cx, Class *newclasp, HandleObject callee)
{
    RootedValue protov(cx);
    if (!JSObject::getProperty(cx, callee, callee, cx->names().classPrototype, &protov))
        return NULL;

    JSObject *proto = protov.isObjectOrNull() ? protov.toObjectOrNull() : NULL;
    JSObject *parent = callee->getParent();
    gc::AllocKind kind = NewObjectGCKind(newclasp);
    return NewObjectWithClassProto(cx, newclasp, proto, parent, kind);
}

static inline JSObject *
CreateThisForFunctionWithType(JSContext *cx, HandleTypeObject type, JSObject *parent,
                              NewObjectKind newKind)
{
    if (type->newScript) {
        




        gc::AllocKind kind = type->newScript->allocKind;
        RootedObject res(cx, NewObjectWithType(cx, type, parent, kind, newKind));
        if (res) {
            RootedShape shape(cx, type->newScript->shape);
            JS_ALWAYS_TRUE(JSObject::setLastProperty(cx, res, shape));
        }
        return res;
    }

    gc::AllocKind allocKind = NewObjectGCKind(&ObjectClass);
    return NewObjectWithType(cx, type, parent, allocKind, newKind);
}

JSObject *
js::CreateThisForFunctionWithProto(JSContext *cx, HandleObject callee, JSObject *proto,
                                  NewObjectKind newKind )
{
    JSObject *res;

    if (proto) {
        RootedTypeObject type(cx, proto->getNewType(cx, &ObjectClass, callee->toFunction()));
        if (!type)
            return NULL;
        res = CreateThisForFunctionWithType(cx, type, callee->getParent(), newKind);
    } else {
        gc::AllocKind allocKind = NewObjectGCKind(&ObjectClass);
        res = NewObjectWithClassProto(cx, &ObjectClass, proto, callee->getParent(), allocKind, newKind);
    }

    if (res && cx->typeInferenceEnabled()) {
        RootedScript script(cx, callee->toFunction()->nonLazyScript());
        TypeScript::SetThis(cx, script, types::Type::ObjectType(res));
    }

    return res;
}

JSObject *
js::CreateThisForFunction(JSContext *cx, HandleObject callee, bool newType)
{
    RootedValue protov(cx);
    if (!JSObject::getProperty(cx, callee, callee, cx->names().classPrototype, &protov))
        return NULL;
    JSObject *proto;
    if (protov.isObject())
        proto = &protov.toObject();
    else
        proto = NULL;
    NewObjectKind newKind = newType ? SingletonObject : GenericObject;
    JSObject *obj = CreateThisForFunctionWithProto(cx, callee, proto, newKind);

    if (obj && newType) {
        RootedObject nobj(cx, obj);

        
        JSObject::clear(cx, nobj);

        RootedScript calleeScript(cx, callee->toFunction()->nonLazyScript());
        TypeScript::SetThis(cx, calleeScript, types::Type::ObjectType(nobj));

        return nobj;
    }

    return obj;
}






static bool
Detecting(JSContext *cx, RawScript script, jsbytecode *pc)
{
    
    JSOp op = JSOp(*pc);
    if (js_CodeSpec[op].format & JOF_DETECTING)
        return true;

    jsbytecode *endpc = script->code + script->length;
    JS_ASSERT(script->code <= pc && pc < endpc);

    if (op == JSOP_NULL) {
        



        if (++pc < endpc) {
            op = JSOp(*pc);
            return op == JSOP_EQ || op == JSOP_NE;
        }
        return false;
    }

    if (op == JSOP_GETGNAME || op == JSOP_NAME) {
        




        JSAtom *atom = script->getAtom(GET_UINT32_INDEX(pc));
        if (atom == cx->names().undefined &&
            (pc += js_CodeSpec[op].length) < endpc) {
            op = JSOp(*pc);
            return op == JSOP_EQ || op == JSOP_NE || op == JSOP_STRICTEQ || op == JSOP_STRICTNE;
        }
    }

    return false;
}





unsigned
js_InferFlags(JSContext *cx, unsigned defaultFlags)
{
    



    jsbytecode *pc;
    RawScript script = cx->stack.currentScript(&pc, ContextStack::ALLOW_CROSS_COMPARTMENT);
    if (!script)
        return defaultFlags;

    uint32_t format = js_CodeSpec[*pc].format;
    unsigned flags = 0;
    if (format & JOF_SET)
        flags |= JSRESOLVE_ASSIGNING;
    return flags;
}

 JSBool
JSObject::nonNativeSetProperty(JSContext *cx, HandleObject obj,
                               HandleId id, MutableHandleValue vp, JSBool strict)
{
    if (JS_UNLIKELY(obj->watched())) {
        WatchpointMap *wpmap = cx->compartment->watchpointMap;
        if (wpmap && !wpmap->triggerWatchpoint(cx, obj, id, vp))
            return false;
    }
    return obj->getOps()->setGeneric(cx, obj, id, vp, strict);
}

 JSBool
JSObject::nonNativeSetElement(JSContext *cx, HandleObject obj,
                              uint32_t index, MutableHandleValue vp, JSBool strict)
{
    if (JS_UNLIKELY(obj->watched())) {
        RootedId id(cx);
        if (!IndexToId(cx, index, &id))
            return false;

        WatchpointMap *wpmap = cx->compartment->watchpointMap;
        if (wpmap && !wpmap->triggerWatchpoint(cx, obj, id, vp))
            return false;
    }
    return obj->getOps()->setElement(cx, obj, index, vp, strict);
}

 bool
JSObject::deleteByValue(JSContext *cx, HandleObject obj,
                        const Value &property, MutableHandleValue rval, bool strict)
{
    uint32_t index;
    if (IsDefinitelyIndex(property, &index))
        return deleteElement(cx, obj, index, rval, strict);

    RootedValue propval(cx, property);
    Rooted<SpecialId> sid(cx);
    if (ValueIsSpecial(obj, &propval, &sid, cx))
        return deleteSpecial(cx, obj, sid, rval, strict);

    JSAtom *name = ToAtom<CanGC>(cx, propval);
    if (!name)
        return false;

    if (name->isIndex(&index))
        return deleteElement(cx, obj, index, rval, strict);

    Rooted<PropertyName*> propname(cx, name->asPropertyName());
    return deleteProperty(cx, obj, propname, rval, strict);
}

JS_FRIEND_API(bool)
JS_CopyPropertiesFrom(JSContext *cx, JSObject *targetArg, JSObject *objArg)
{
    RootedObject target(cx, targetArg);
    RootedObject obj(cx, objArg);

    
    JS_ASSERT(target->isNative() == obj->isNative());
    if (!target->isNative())
        return true;

    AutoShapeVector shapes(cx);
    for (Shape::Range<NoGC> r(obj->lastProperty()); !r.empty(); r.popFront()) {
        if (!shapes.append(&r.front()))
            return false;
    }

    RootedShape shape(cx);
    RootedValue v(cx);
    RootedId id(cx);
    size_t n = shapes.length();
    while (n > 0) {
        shape = shapes[--n];
        unsigned attrs = shape->attributes();
        PropertyOp getter = shape->getter();
        StrictPropertyOp setter = shape->setter();
        AutoRooterGetterSetter gsRoot(cx, attrs, &getter, &setter);
        if ((attrs & JSPROP_GETTER) && !cx->compartment->wrap(cx, &getter))
            return false;
        if ((attrs & JSPROP_SETTER) && !cx->compartment->wrap(cx, &setter))
            return false;
        v = shape->hasSlot() ? obj->getSlot(shape->slot()) : UndefinedValue();
        if (!cx->compartment->wrap(cx, &v))
            return false;
        id = shape->propid();
        if (!JSObject::defineGeneric(cx, target, id, v, getter, setter, attrs))
            return false;
    }
    return true;
}

static bool
CopySlots(JSContext *cx, HandleObject from, HandleObject to)
{
    JS_ASSERT(!from->isNative() && !to->isNative());
    JS_ASSERT(from->getClass() == to->getClass());

    size_t n = 0;
    if (from->isWrapper() &&
        (Wrapper::wrapperHandler(from)->flags() &
         Wrapper::CROSS_COMPARTMENT)) {
        to->setSlot(0, from->getSlot(0));
        to->setSlot(1, from->getSlot(1));
        n = 2;
    }

    size_t span = JSCLASS_RESERVED_SLOTS(from->getClass());
    RootedValue v(cx);
    for (; n < span; ++n) {
        v = from->getSlot(n);
        if (!cx->compartment->wrap(cx, &v))
            return false;
        to->setSlot(n, v);
    }
    return true;
}

JSObject *
js::CloneObject(JSContext *cx, HandleObject obj, Handle<js::TaggedProto> proto, HandleObject parent)
{
    if (!obj->isNative() && !obj->isProxy()) {
        JS_ReportErrorNumber(cx, js_GetErrorMessage, NULL,
                             JSMSG_CANT_CLONE_OBJECT);
        return NULL;
    }
    RootedObject clone(cx, NewObjectWithGivenProto(cx, obj->getClass(), proto, parent));
    if (!clone)
        return NULL;
    if (obj->isNative()) {
        if (clone->isFunction() && (obj->compartment() != clone->compartment())) {
            JS_ReportErrorNumber(cx, js_GetErrorMessage, NULL,
                                 JSMSG_CANT_CLONE_OBJECT);
            return NULL;
        }

        if (obj->hasPrivate())
            clone->setPrivate(obj->getPrivate());
    } else {
        JS_ASSERT(obj->isProxy());
        if (!CopySlots(cx, obj, clone))
            return NULL;
    }

    return clone;
}

JSObject *
js::CloneObjectLiteral(JSContext *cx, HandleObject parent, HandleObject srcObj)
{
    Rooted<TypeObject*> typeObj(cx);
    typeObj = cx->global()->getOrCreateObjectPrototype(cx)->getNewType(cx, &ObjectClass);
    RootedShape shape(cx, srcObj->lastProperty());
    return NewReshapedObject(cx, typeObj, parent, srcObj->getAllocKind(), shape);
}

struct JSObject::TradeGutsReserved {
    Vector<Value> avals;
    Vector<Value> bvals;
    int newafixed;
    int newbfixed;
    RootedShape newashape;
    RootedShape newbshape;
    HeapSlot *newaslots;
    HeapSlot *newbslots;

    TradeGutsReserved(JSContext *cx)
        : avals(cx), bvals(cx),
          newafixed(0), newbfixed(0),
          newashape(cx), newbshape(cx),
          newaslots(NULL), newbslots(NULL)
    {}

    ~TradeGutsReserved()
    {
        if (newaslots)
            js_free(newaslots);
        if (newbslots)
            js_free(newbslots);
    }
};

bool
JSObject::ReserveForTradeGuts(JSContext *cx, JSObject *aArg, JSObject *bArg,
                              TradeGutsReserved &reserved)
{
    RootedObject a(cx, aArg);
    RootedObject b(cx, bArg);
    JS_ASSERT(a->compartment() == b->compartment());
    AutoCompartment ac(cx, a);

    





#ifdef JSGC_INCREMENTAL
    




    JS::Zone *zone = a->zone();
    if (zone->needsBarrier()) {
        MarkChildren(zone->barrierTracer(), a);
        MarkChildren(zone->barrierTracer(), b);
    }
#endif

    



    Class *aClass = a->getClass();
    Class *bClass = b->getClass();
    Rooted<TaggedProto> aProto(cx, a->getTaggedProto());
    Rooted<TaggedProto> bProto(cx, b->getTaggedProto());
    if (!SetClassAndProto(cx, a, bClass, bProto, false))
        return false;
    if (!SetClassAndProto(cx, b, aClass, aProto, false))
        return false;

    if (a->sizeOfThis() == b->sizeOfThis())
        return true;

    





    if (a->isNative()) {
        if (!a->generateOwnShape(cx))
            return false;
    } else {
        reserved.newbshape = EmptyShape::getInitialShape(cx, aClass, aProto, a->getParent(),
                                                         b->getAllocKind());
        if (!reserved.newbshape)
            return false;
    }
    if (b->isNative()) {
        if (!b->generateOwnShape(cx))
            return false;
    } else {
        reserved.newashape = EmptyShape::getInitialShape(cx, bClass, bProto, b->getParent(),
                                                         a->getAllocKind());
        if (!reserved.newashape)
            return false;
    }

    

    if (!reserved.avals.reserve(a->slotSpan()))
        return false;
    if (!reserved.bvals.reserve(b->slotSpan()))
        return false;

    





    reserved.newafixed = a->numFixedSlots();
    reserved.newbfixed = b->numFixedSlots();

    if (aClass->hasPrivate()) {
        reserved.newafixed++;
        reserved.newbfixed--;
    }
    if (bClass->hasPrivate()) {
        reserved.newbfixed++;
        reserved.newafixed--;
    }

    JS_ASSERT(reserved.newafixed >= 0);
    JS_ASSERT(reserved.newbfixed >= 0);

    





    unsigned adynamic = dynamicSlotsCount(reserved.newafixed, b->slotSpan());
    unsigned bdynamic = dynamicSlotsCount(reserved.newbfixed, a->slotSpan());

    if (adynamic) {
        reserved.newaslots = cx->pod_malloc<HeapSlot>(adynamic);
        if (!reserved.newaslots)
            return false;
        Debug_SetSlotRangeToCrashOnTouch(reserved.newaslots, adynamic);
    }
    if (bdynamic) {
        reserved.newbslots = cx->pod_malloc<HeapSlot>(bdynamic);
        if (!reserved.newbslots)
            return false;
        Debug_SetSlotRangeToCrashOnTouch(reserved.newbslots, bdynamic);
    }

    return true;
}

void
JSObject::TradeGuts(JSContext *cx, JSObject *a, JSObject *b, TradeGutsReserved &reserved)
{
    JS_ASSERT(a->compartment() == b->compartment());
    JS_ASSERT(a->isFunction() == b->isFunction());

    



    TypeObject *tmp = a->type_;
    a->type_ = b->type_;
    b->type_ = tmp;

    
    JS_ASSERT_IF(a->isFunction(), a->sizeOfThis() == b->sizeOfThis());

    




    JS_ASSERT(!a->isRegExp() && !b->isRegExp());

    
    JS_ASSERT(!a->isArray() && !b->isArray());

    



    JS_ASSERT(!a->isArrayBuffer() && !b->isArrayBuffer());

    
    const size_t size = a->sizeOfThis();
    if (size == b->sizeOfThis()) {
        




        char tmp[tl::Max<sizeof(JSFunction), sizeof(JSObject_Slots16)>::result];
        JS_ASSERT(size <= sizeof(tmp));

        js_memcpy(tmp, a, size);
        js_memcpy(a, b, size);
        js_memcpy(b, tmp, size);

#ifdef JSGC_GENERATIONAL
        



        for (size_t i = 0; i < a->numFixedSlots(); ++i) {
            HeapSlot::writeBarrierPost(cx->runtime, a, HeapSlot::Slot, i);
            HeapSlot::writeBarrierPost(cx->runtime, b, HeapSlot::Slot, i);
        }
#endif
    } else {
        





        unsigned acap = a->slotSpan();
        unsigned bcap = b->slotSpan();

        for (size_t i = 0; i < acap; i++)
            reserved.avals.infallibleAppend(a->getSlot(i));

        for (size_t i = 0; i < bcap; i++)
            reserved.bvals.infallibleAppend(b->getSlot(i));

        
        if (a->hasDynamicSlots())
            js_free(a->slots);
        if (b->hasDynamicSlots())
            js_free(b->slots);

        void *apriv = a->hasPrivate() ? a->getPrivate() : NULL;
        void *bpriv = b->hasPrivate() ? b->getPrivate() : NULL;

        char tmp[sizeof(JSObject)];
        js_memcpy(&tmp, a, sizeof tmp);
        js_memcpy(a, b, sizeof tmp);
        js_memcpy(b, &tmp, sizeof tmp);

        if (a->isNative())
            a->shape_->setNumFixedSlots(reserved.newafixed);
        else
            a->shape_ = reserved.newashape;

        a->slots = reserved.newaslots;
        a->initSlotRange(0, reserved.bvals.begin(), bcap);
        if (a->hasPrivate())
            a->initPrivate(bpriv);

        if (b->isNative())
            b->shape_->setNumFixedSlots(reserved.newbfixed);
        else
            b->shape_ = reserved.newbshape;

        b->slots = reserved.newbslots;
        b->initSlotRange(0, reserved.avals.begin(), acap);
        if (b->hasPrivate())
            b->initPrivate(apriv);

        
        reserved.newaslots = NULL;
        reserved.newbslots = NULL;
    }

#ifdef JSGC_GENERATIONAL
    Shape::writeBarrierPost(a->shape_, &a->shape_);
    Shape::writeBarrierPost(b->shape_, &b->shape_);
    types::TypeObject::writeBarrierPost(a->type_, &a->type_);
    types::TypeObject::writeBarrierPost(b->type_, &b->type_);
#endif

    if (a->inDictionaryMode())
        a->lastProperty()->listp = &a->shape_;
    if (b->inDictionaryMode())
        b->lastProperty()->listp = &b->shape_;
}


bool
JSObject::swap(JSContext *cx, HandleObject a, HandleObject b)
{
    AutoMarkInDeadZone adc1(a->zone());
    AutoMarkInDeadZone adc2(b->zone());

    
    JS_ASSERT(IsBackgroundFinalized(a->getAllocKind()) ==
              IsBackgroundFinalized(b->getAllocKind()));
    JS_ASSERT(a->compartment() == b->compartment());

    unsigned r = NotifyGCPreSwap(a, b);

    TradeGutsReserved reserved(cx);
    if (!ReserveForTradeGuts(cx, a, b, reserved)) {
        NotifyGCPostSwap(b, a, r);
        return false;
    }
    TradeGuts(cx, a, b, reserved);

    NotifyGCPostSwap(a, b, r);
    return true;
}

static bool
DefineStandardSlot(JSContext *cx, HandleObject obj, JSProtoKey key, JSAtom *atom,
                   HandleValue v, uint32_t attrs, bool &named)
{
    RootedId id(cx, AtomToId(atom));

    if (key != JSProto_Null) {
        




        JS_ASSERT(obj->isGlobal());
        JS_ASSERT(obj->isNative());

        if (!obj->nativeLookup(cx, id)) {
            uint32_t slot = 2 * JSProto_LIMIT + key;
            obj->setReservedSlot(slot, v);
            if (!JSObject::addProperty(cx, obj, id, JS_PropertyStub, JS_StrictPropertyStub, slot, attrs, 0, 0))
                return false;
            AddTypePropertyId(cx, obj, id, v);

            named = true;
            return true;
        }
    }

    named = JSObject::defineGeneric(cx, obj, id,
                                    v, JS_PropertyStub, JS_StrictPropertyStub, attrs);
    return named;
}

static void
SetClassObject(JSObject *obj, JSProtoKey key, JSObject *cobj, JSObject *proto)
{
    JS_ASSERT(!obj->getParent());
    if (!obj->isGlobal())
        return;

    obj->setReservedSlot(key, ObjectOrNullValue(cobj));
    obj->setReservedSlot(JSProto_LIMIT + key, ObjectOrNullValue(proto));
}

static void
ClearClassObject(JSObject *obj, JSProtoKey key)
{
    JS_ASSERT(!obj->getParent());
    if (!obj->isGlobal())
        return;

    obj->setSlot(key, UndefinedValue());
    obj->setSlot(JSProto_LIMIT + key, UndefinedValue());
}

JSObject *
js::DefineConstructorAndPrototype(JSContext *cx, HandleObject obj, JSProtoKey key, HandleAtom atom,
                                  JSObject *protoProto, Class *clasp,
                                  Native constructor, unsigned nargs,
                                  JSPropertySpec *ps, JSFunctionSpec *fs,
                                  JSPropertySpec *static_ps, JSFunctionSpec *static_fs,
                                  JSObject **ctorp, AllocKind ctorKind)
{
    





















    






    RootedObject proto(cx, NewObjectWithClassProto(cx, clasp, protoProto, obj, SingletonObject));
    if (!proto)
        return NULL;

    
    RootedObject ctor(cx);
    bool named = false;
    bool cached = false;
    if (!constructor) {
        





        if (!(clasp->flags & JSCLASS_IS_ANONYMOUS) || !obj->isGlobal() || key == JSProto_Null) {
            uint32_t attrs = (clasp->flags & JSCLASS_IS_ANONYMOUS)
                           ? JSPROP_READONLY | JSPROP_PERMANENT
                           : 0;
            RootedValue value(cx, ObjectValue(*proto));
            if (!DefineStandardSlot(cx, obj, key, atom, value, attrs, named))
                goto bad;
        }

        ctor = proto;
    } else {
        





        RootedFunction fun(cx, NewFunction(cx, NullPtr(), constructor, nargs,
                                           JSFunction::NATIVE_CTOR, obj, atom, ctorKind));
        if (!fun)
            goto bad;

        




        if (key != JSProto_Null) {
            SetClassObject(obj, key, fun, proto);
            cached = true;
        }

        RootedValue value(cx, ObjectValue(*fun));
        if (!DefineStandardSlot(cx, obj, key, atom, value, 0, named))
            goto bad;

        




        ctor = fun;
        if (!LinkConstructorAndPrototype(cx, ctor, proto))
            goto bad;

        
        Rooted<TaggedProto> tagged(cx, TaggedProto(proto));
        if (ctor->getClass() == clasp && !ctor->splicePrototype(cx, clasp, tagged))
            goto bad;
    }

    if (!DefinePropertiesAndBrand(cx, proto, ps, fs) ||
        (ctor != proto && !DefinePropertiesAndBrand(cx, ctor, static_ps, static_fs)))
    {
        goto bad;
    }

    if (clasp->flags & (JSCLASS_FREEZE_PROTO|JSCLASS_FREEZE_CTOR)) {
        JS_ASSERT_IF(ctor == proto, !(clasp->flags & JSCLASS_FREEZE_CTOR));
        if (proto && (clasp->flags & JSCLASS_FREEZE_PROTO) && !JSObject::freeze(cx, proto))
            goto bad;
        if (ctor && (clasp->flags & JSCLASS_FREEZE_CTOR) && !JSObject::freeze(cx, ctor))
            goto bad;
    }

    
    if (!cached && key != JSProto_Null)
        SetClassObject(obj, key, ctor, proto);

    if (ctorp)
        *ctorp = ctor;
    return proto;

bad:
    if (named) {
        RootedValue rval(cx);
        JSObject::deleteByValue(cx, obj, StringValue(atom), &rval, false);
    }
    if (cached)
        ClearClassObject(obj, key);
    return NULL;
}










bool
js::IsStandardClassResolved(JSObject *obj, js::Class *clasp)
{
    JSProtoKey key = JSCLASS_CACHED_PROTO_KEY(clasp);

    
    return (obj->getReservedSlot(key) != UndefinedValue());
}

void
js::MarkStandardClassInitializedNoProto(JSObject *obj, js::Class *clasp)
{
    JSProtoKey key = JSCLASS_CACHED_PROTO_KEY(clasp);

    



    if (obj->getReservedSlot(key) == UndefinedValue())
        obj->setSlot(key, BooleanValue(true));
}

JSObject *
js_InitClass(JSContext *cx, HandleObject obj, JSObject *protoProto_,
             Class *clasp, Native constructor, unsigned nargs,
             JSPropertySpec *ps, JSFunctionSpec *fs,
             JSPropertySpec *static_ps, JSFunctionSpec *static_fs,
             JSObject **ctorp, AllocKind ctorKind)
{
    RootedObject protoProto(cx, protoProto_);

    RootedAtom atom(cx, Atomize(cx, clasp->name, strlen(clasp->name)));
    if (!atom)
        return NULL;

    











    JSProtoKey key = JSCLASS_CACHED_PROTO_KEY(clasp);
    if (key != JSProto_Null &&
        !protoProto &&
        !js_GetClassPrototype(cx, JSProto_Object, &protoProto)) {
        return NULL;
    }

    return DefineConstructorAndPrototype(cx, obj, key, atom, protoProto, clasp, constructor, nargs,
                                         ps, fs, static_ps, static_fs, ctorp, ctorKind);
}

 inline bool
JSObject::updateSlotsForSpan(JSContext *cx, HandleObject obj, size_t oldSpan, size_t newSpan)
{
    JS_ASSERT(oldSpan != newSpan);

    size_t oldCount = dynamicSlotsCount(obj->numFixedSlots(), oldSpan);
    size_t newCount = dynamicSlotsCount(obj->numFixedSlots(), newSpan);

    if (oldSpan < newSpan) {
        if (oldCount < newCount && !JSObject::growSlots(cx, obj, oldCount, newCount))
            return false;

        if (newSpan == oldSpan + 1)
            obj->initSlotUnchecked(oldSpan, UndefinedValue());
        else
            obj->initializeSlotRange(oldSpan, newSpan - oldSpan);
    } else {
        
        obj->prepareSlotRangeForOverwrite(newSpan, oldSpan);
        obj->invalidateSlotRange(newSpan, oldSpan - newSpan);

        if (oldCount > newCount)
            JSObject::shrinkSlots(cx, obj, oldCount, newCount);
    }

    return true;
}

 bool
JSObject::setLastProperty(JSContext *cx, HandleObject obj, HandleShape shape)
{
    JS_ASSERT(!obj->inDictionaryMode());
    JS_ASSERT(!shape->inDictionary());
    JS_ASSERT(shape->compartment() == obj->compartment());
    JS_ASSERT(shape->numFixedSlots() == obj->numFixedSlots());

    size_t oldSpan = obj->lastProperty()->slotSpan();
    size_t newSpan = shape->slotSpan();

    if (oldSpan == newSpan) {
        obj->shape_ = shape;
        return true;
    }

    if (!updateSlotsForSpan(cx, obj, oldSpan, newSpan))
        return false;

    obj->shape_ = shape;
    return true;
}

 bool
JSObject::setSlotSpan(JSContext *cx, HandleObject obj, uint32_t span)
{
    JS_ASSERT(obj->inDictionaryMode());

    size_t oldSpan = obj->lastProperty()->base()->slotSpan();
    if (oldSpan == span)
        return true;

    if (!JSObject::updateSlotsForSpan(cx, obj, oldSpan, span))
        return false;

    obj->lastProperty()->base()->setSlotSpan(span);
    return true;
}

 bool
JSObject::growSlots(JSContext *cx, HandleObject obj, uint32_t oldCount, uint32_t newCount)
{
    JS_ASSERT(newCount > oldCount);
    JS_ASSERT(newCount >= SLOT_CAPACITY_MIN);

    




    JS_ASSERT(newCount < NELEMENTS_LIMIT);

    size_t oldSize = Probes::objectResizeActive() ? obj->computedSizeOfThisSlotsElements() : 0;
    size_t newSize = oldSize + (newCount - oldCount) * sizeof(Value);

    





    if (!obj->hasLazyType() && !oldCount && obj->type()->newScript) {
        gc::AllocKind kind = obj->type()->newScript->allocKind;
        unsigned newScriptSlots = gc::GetGCKindSlots(kind);
        if (newScriptSlots == obj->numFixedSlots() && gc::TryIncrementAllocKind(&kind)) {
            AutoEnterAnalysis enter(cx);

            Rooted<TypeObject*> typeObj(cx, obj->type());
            RootedShape shape(cx, typeObj->newScript->shape);
            JSObject *reshapedObj = NewReshapedObject(cx, typeObj, obj->getParent(), kind, shape);
            if (!reshapedObj)
                return false;

            typeObj->newScript->allocKind = kind;
            typeObj->newScript->shape = reshapedObj->lastProperty();
            typeObj->markStateChange(cx);
        }
    }

    if (!oldCount) {
        obj->slots = cx->pod_malloc<HeapSlot>(newCount);
        if (!obj->slots)
            return false;
        Debug_SetSlotRangeToCrashOnTouch(obj->slots, newCount);
        if (Probes::objectResizeActive())
            Probes::resizeObject(cx, obj, oldSize, newSize);
        return true;
    }

    HeapSlot *newslots = (HeapSlot*) cx->realloc_(obj->slots, oldCount * sizeof(HeapSlot),
                                                  newCount * sizeof(HeapSlot));
    if (!newslots)
        return false;  

    bool changed = obj->slots != newslots;
    obj->slots = newslots;

    Debug_SetSlotRangeToCrashOnTouch(obj->slots + oldCount, newCount - oldCount);

    
    if (changed && obj->isGlobal())
        types::MarkObjectStateChange(cx, obj);

    if (Probes::objectResizeActive())
        Probes::resizeObject(cx, obj, oldSize, newSize);

    return true;
}

 void
JSObject::shrinkSlots(JSContext *cx, HandleObject obj, uint32_t oldCount, uint32_t newCount)
{
    JS_ASSERT(newCount < oldCount);

    





    if (obj->isCall())
        return;

    size_t oldSize = Probes::objectResizeActive() ? obj->computedSizeOfThisSlotsElements() : 0;
    size_t newSize = oldSize - (oldCount - newCount) * sizeof(Value);

    if (newCount == 0) {
        js_free(obj->slots);
        obj->slots = NULL;
        if (Probes::objectResizeActive())
            Probes::resizeObject(cx, obj, oldSize, newSize);
        return;
    }

    JS_ASSERT(newCount >= SLOT_CAPACITY_MIN);

    HeapSlot *newslots = (HeapSlot *) cx->realloc_(obj->slots, newCount * sizeof(HeapSlot));
    if (!newslots)
        return;  

    bool changed = obj->slots != newslots;
    obj->slots = newslots;

    
    if (changed && obj->isGlobal())
        types::MarkObjectStateChange(cx, obj);

    if (Probes::objectResizeActive())
        Probes::resizeObject(cx, obj, oldSize, newSize);
}

 bool
JSObject::sparsifyDenseElement(JSContext *cx, HandleObject obj, unsigned index)
{
    RootedValue value(cx, obj->getDenseElement(index));
    JS_ASSERT(!value.isMagic(JS_ELEMENTS_HOLE));

    JSObject::removeDenseElementForSparseIndex(cx, obj, index);

    uint32_t slot = obj->slotSpan();
    if (!obj->addDataProperty(cx, INT_TO_JSID(index), slot, JSPROP_ENUMERATE)) {
        obj->setDenseElement(index, value);
        return false;
    }

    JS_ASSERT(slot == obj->slotSpan() - 1);
    obj->initSlot(slot, value);

    return true;
}

 bool
JSObject::sparsifyDenseElements(JSContext *cx, HandleObject obj)
{
    uint32_t initialized = obj->getDenseInitializedLength();

    
    for (uint32_t i = 0; i < initialized; i++) {
        if (obj->getDenseElement(i).isMagic(JS_ELEMENTS_HOLE))
            continue;

        if (!sparsifyDenseElement(cx, obj, i))
            return false;
    }

    if (initialized)
        obj->setDenseInitializedLength(0);

    




    if (obj->getDenseCapacity()) {
        obj->shrinkElements(cx, 0);
        obj->getElementsHeader()->capacity = 0;
    }

    return true;
}

bool
JSObject::willBeSparseElements(unsigned requiredCapacity, unsigned newElementsHint)
{
    JS_ASSERT(isNative());
    JS_ASSERT(requiredCapacity > MIN_SPARSE_INDEX);

    unsigned cap = getDenseCapacity();
    JS_ASSERT(requiredCapacity >= cap);

    if (requiredCapacity >= NELEMENTS_LIMIT)
        return true;

    unsigned minimalDenseCount = requiredCapacity / SPARSE_DENSITY_RATIO;
    if (newElementsHint >= minimalDenseCount)
        return false;
    minimalDenseCount -= newElementsHint;

    if (minimalDenseCount > cap)
        return true;

    unsigned len = getDenseInitializedLength();
    const Value *elems = getDenseElements();
    for (unsigned i = 0; i < len; i++) {
        if (!elems[i].isMagic(JS_ELEMENTS_HOLE) && !--minimalDenseCount)
            return false;
    }
    return true;
}

 JSObject::EnsureDenseResult
JSObject::maybeDensifySparseElements(JSContext *cx, HandleObject obj)
{
    




    if (!obj->inDictionaryMode())
        return ED_SPARSE;

    



    uint32_t slotSpan = obj->slotSpan();
    if (slotSpan != RoundUpPow2(slotSpan))
        return ED_SPARSE;

    
    if (!obj->isExtensible() || obj->watched())
        return ED_SPARSE;

    



    uint32_t numDenseElements = 0;
    uint32_t newInitializedLength = 0;

    RootedShape shape(cx, obj->lastProperty());
    while (!shape->isEmptyShape()) {
        uint32_t index;
        if (js_IdIsIndex(shape->propid(), &index)) {
            if (shape->attributes() == JSPROP_ENUMERATE &&
                shape->hasDefaultGetter() &&
                shape->hasDefaultSetter())
            {
                numDenseElements++;
                newInitializedLength = Max(newInitializedLength, index + 1);
            } else {
                



                return ED_SPARSE;
            }
        }
        shape = shape->previous();
    }

    if (numDenseElements * SPARSE_DENSITY_RATIO < newInitializedLength)
        return ED_SPARSE;

    if (newInitializedLength >= NELEMENTS_LIMIT)
        return ED_SPARSE;

    




    if (newInitializedLength > obj->getDenseCapacity()) {
        if (!obj->growElements(cx, newInitializedLength))
            return ED_FAILED;
    }

    obj->ensureDenseInitializedLength(cx, newInitializedLength, 0);

    RootedValue value(cx);

    shape = obj->lastProperty();
    while (!shape->isEmptyShape()) {
        jsid id = shape->propid();
        uint32_t index;
        if (js_IdIsIndex(id, &index)) {
            value = obj->getSlot(shape->slot());

            






            if (shape != obj->lastProperty()) {
                shape = shape->previous();
                if (!obj->removeProperty(cx, id))
                    return ED_FAILED;
            } else {
                if (!obj->removeProperty(cx, id))
                    return ED_FAILED;
                shape = obj->lastProperty();
            }

            obj->setDenseElement(index, value);
        } else {
            shape = shape->previous();
        }
    }

    




    if (!obj->clearFlag(cx, BaseShape::INDEXED))
        return ED_FAILED;

    return ED_OK;
}

bool
JSObject::growElements(JSContext *cx, unsigned newcap)
{
    size_t oldSize = Probes::objectResizeActive() ? computedSizeOfThisSlotsElements() : 0;

    if (!growElements(&cx->zone()->allocator, newcap)) {
        JS_ReportOutOfMemory(cx);
        return false;
    }

    if (Probes::objectResizeActive())
        Probes::resizeObject(cx, this, oldSize, computedSizeOfThisSlotsElements());

    return true;
}

bool
JSObject::growElements(js::Allocator *alloc, unsigned newcap)
{
    






    JS_ASSERT(isExtensible());

    






    static const size_t CAPACITY_DOUBLING_MAX = 1024 * 1024;
    static const size_t CAPACITY_CHUNK = CAPACITY_DOUBLING_MAX / sizeof(Value);

    uint32_t oldcap = getDenseCapacity();
    JS_ASSERT(oldcap <= newcap);

    uint32_t nextsize = (oldcap <= CAPACITY_DOUBLING_MAX)
                      ? oldcap * 2
                      : oldcap + (oldcap >> 3);

    uint32_t actualCapacity = Max(newcap, nextsize);
    if (actualCapacity >= CAPACITY_CHUNK)
        actualCapacity = JS_ROUNDUP(actualCapacity, CAPACITY_CHUNK);
    else if (actualCapacity < SLOT_CAPACITY_MIN)
        actualCapacity = SLOT_CAPACITY_MIN;

    
    if (actualCapacity >= NELEMENTS_LIMIT || actualCapacity < oldcap || actualCapacity < newcap) {
        return false;
    }

    uint32_t initlen = getDenseInitializedLength();
    uint32_t newAllocated = actualCapacity + ObjectElements::VALUES_PER_HEADER;

    ObjectElements *newheader;
    if (hasDynamicElements()) {
        uint32_t oldAllocated = oldcap + ObjectElements::VALUES_PER_HEADER;
        newheader = (ObjectElements *)
            alloc->realloc_(getElementsHeader(), oldAllocated * sizeof(Value),
                            newAllocated * sizeof(Value));
        if (!newheader)
            return false;  
    } else {
        newheader = (ObjectElements *) alloc->malloc_(newAllocated * sizeof(Value));
        if (!newheader)
            return false;  
        js_memcpy(newheader, getElementsHeader(),
                  (ObjectElements::VALUES_PER_HEADER + initlen) * sizeof(Value));
    }

    newheader->capacity = actualCapacity;
    elements = newheader->elements();

    Debug_SetSlotRangeToCrashOnTouch(elements + initlen, actualCapacity - initlen);

    return true;
}

void
JSObject::shrinkElements(JSContext *cx, unsigned newcap)
{
    uint32_t oldcap = getDenseCapacity();
    JS_ASSERT(newcap <= oldcap);

    size_t oldSize = Probes::objectResizeActive() ? computedSizeOfThisSlotsElements() : 0;

    
    if (oldcap <= SLOT_CAPACITY_MIN || !hasDynamicElements())
        return;

    newcap = Max(newcap, SLOT_CAPACITY_MIN);

    uint32_t newAllocated = newcap + ObjectElements::VALUES_PER_HEADER;

    ObjectElements *newheader = (ObjectElements *)
        cx->realloc_(getElementsHeader(), newAllocated * sizeof(Value));
    if (!newheader)
        return;  

    newheader->capacity = newcap;
    elements = newheader->elements();

    if (Probes::objectResizeActive())
        Probes::resizeObject(cx, this, oldSize, computedSizeOfThisSlotsElements());
}

static JSObject *
js_InitNullClass(JSContext *cx, HandleObject obj)
{
    JS_ASSERT(0);
    return NULL;
}

#define DECLARE_PROTOTYPE_CLASS_INIT(name,code,init) \
    extern JSObject *init(JSContext *cx, Handle<JSObject*> obj);
JS_FOR_EACH_PROTOTYPE(DECLARE_PROTOTYPE_CLASS_INIT)
#undef DECLARE_PROTOTYPE_CLASS_INIT

static JSClassInitializerOp lazy_prototype_init[JSProto_LIMIT] = {
#define LAZY_PROTOTYPE_INIT(name,code,init) init,
    JS_FOR_EACH_PROTOTYPE(LAZY_PROTOTYPE_INIT)
#undef LAZY_PROTOTYPE_INIT
};

bool
js::SetClassAndProto(JSContext *cx, HandleObject obj,
                     Class *clasp, Handle<js::TaggedProto> proto, bool checkForCycles)
{
    JS_ASSERT_IF(!checkForCycles, obj.get() != proto.raw());

    


















    RootedObject oldproto(cx, obj);
    while (oldproto && oldproto->isNative()) {
        if (oldproto->hasSingletonType()) {
            if (!oldproto->generateOwnShape(cx))
                return false;
        } else {
            if (!oldproto->setUncacheableProto(cx))
                return false;
        }
        oldproto = oldproto->getProto();
    }

    if (checkForCycles) {
        JS_ASSERT(!proto.isLazy());
        RootedObject obj2(cx);
        for (obj2 = proto.toObjectOrNull(); obj2; ) {
            if (obj2 == obj) {
                JS_ReportErrorNumber(cx, js_GetErrorMessage, NULL, JSMSG_CYCLIC_VALUE,
                                     js_proto_str);
                return false;
            }

            if (!JSObject::getProto(cx, obj2, &obj2))
                return false;
        }
    }

    if (obj->hasSingletonType()) {
        



        if (!obj->splicePrototype(cx, clasp, proto))
            return false;
        MarkTypeObjectUnknownProperties(cx, obj->type());
        return true;
    }

    if (proto.isObject()) {
        RootedObject protoObj(cx, proto.toObject());
        if (!JSObject::setNewTypeUnknown(cx, clasp, protoObj))
            return false;
    }

    TypeObject *type = cx->compartment->getNewType(cx, clasp, proto);
    if (!type)
        return false;

    







    MarkTypeObjectUnknownProperties(cx, obj->type(), true);
    MarkTypeObjectUnknownProperties(cx, type, true);

    obj->setType(type);
    return true;
}

bool
js_GetClassObject(JSContext *cx, RawObject obj, JSProtoKey key, MutableHandleObject objp)
{
    RootedObject global(cx, &obj->global());
    if (!global->isGlobal()) {
        objp.set(NULL);
        return true;
    }

    Value v = global->getReservedSlot(key);
    if (v.isObject()) {
        objp.set(&v.toObject());
        return true;
    }

    RootedId name(cx, NameToId(ClassName(key, cx)));
    AutoResolving resolving(cx, global, name);
    if (resolving.alreadyStarted()) {
        
        objp.set(NULL);
        return true;
    }

    RootedObject cobj(cx, NULL);
    if (JSClassInitializerOp init = lazy_prototype_init[key]) {
        if (!init(cx, global))
            return false;
        v = global->getReservedSlot(key);
        if (v.isObject())
            cobj = &v.toObject();
    }

    objp.set(cobj);
    return true;
}

JSProtoKey
js_IdentifyClassPrototype(JSObject *obj)
{
    
    
    
    JSProtoKey key = JSCLASS_CACHED_PROTO_KEY(obj->getClass());
    if (key == JSProto_Null)
        return JSProto_Null;

    
    
    
    
    JSObject &global = obj->global();
    Value v = global.getReservedSlot(JSProto_LIMIT + key);
    if (v.isObject() && obj == &v.toObject())
        return key;

    
    return JSProto_Null;
}

bool
js_FindClassObject(JSContext *cx, JSProtoKey protoKey, MutableHandleValue vp, Class *clasp)
{
    RootedId id(cx);

    if (protoKey != JSProto_Null) {
        JS_ASSERT(JSProto_Null < protoKey);
        JS_ASSERT(protoKey < JSProto_LIMIT);
        RootedObject cobj(cx);
        if (!js_GetClassObject(cx, cx->global(), protoKey, &cobj))
            return false;
        if (cobj) {
            vp.set(ObjectValue(*cobj));
            return JS_TRUE;
        }
        id = NameToId(ClassName(protoKey, cx));
    } else {
        JSAtom *atom = Atomize(cx, clasp->name, strlen(clasp->name));
        if (!atom)
            return false;
        id = AtomToId(atom);
    }

    RootedObject pobj(cx);
    RootedShape shape(cx);
    if (!LookupPropertyWithFlags(cx, cx->global(), id, 0, &pobj, &shape))
        return false;
    RootedValue v(cx, UndefinedValue());
    if (shape && pobj->isNative()) {
        if (shape->hasSlot()) {
            v = pobj->nativeGetSlot(shape->slot());
            if (v.get().isPrimitive())
                v.get().setUndefined();
        }
    }
    vp.set(v);
    return true;
}

 bool
JSObject::allocSlot(JSContext *cx, HandleObject obj, uint32_t *slotp)
{
    uint32_t slot = obj->slotSpan();
    JS_ASSERT(slot >= JSSLOT_FREE(obj->getClass()));

    



    if (obj->inDictionaryMode()) {
        ShapeTable &table = obj->lastProperty()->table();
        uint32_t last = table.freelist;
        if (last != SHAPE_INVALID_SLOT) {
#ifdef DEBUG
            JS_ASSERT(last < slot);
            uint32_t next = obj->getSlot(last).toPrivateUint32();
            JS_ASSERT_IF(next != SHAPE_INVALID_SLOT, next < slot);
#endif

            *slotp = last;

            const Value &vref = obj->getSlot(last);
            table.freelist = vref.toPrivateUint32();
            obj->setSlot(last, UndefinedValue());
            return true;
        }
    }

    if (slot >= SHAPE_MAXIMUM_SLOT) {
        js_ReportOutOfMemory(cx);
        return false;
    }

    *slotp = slot;

    if (obj->inDictionaryMode() && !setSlotSpan(cx, obj, slot + 1))
        return false;

    return true;
}

void
JSObject::freeSlot(uint32_t slot)
{
    JS_ASSERT(slot < slotSpan());

    if (inDictionaryMode()) {
        uint32_t &last = lastProperty()->table().freelist;

        
        JS_ASSERT_IF(last != SHAPE_INVALID_SLOT, last < slotSpan() && last != slot);

        



        if (JSSLOT_FREE(getClass()) <= slot) {
            JS_ASSERT_IF(last != SHAPE_INVALID_SLOT, last < slotSpan());
            setSlot(slot, PrivateUint32Value(last));
            last = slot;
            return;
        }
    }
    setSlot(slot, UndefinedValue());
}

static bool
PurgeProtoChain(JSContext *cx, RawObject objArg, HandleId id)
{
    
    RootedObject obj(cx, objArg);

    RootedShape shape(cx);
    while (obj) {
        
        if (!obj->isNative())
            break;

        shape = obj->nativeLookup(cx, id);
        if (shape) {
            if (!obj->shadowingShapeChange(cx, *shape))
                return false;

            obj->shadowingShapeChange(cx, *shape);
            return true;
        }
        obj = obj->getProto();
    }

    return true;
}

bool
js_PurgeScopeChainHelper(JSContext *cx, HandleObject objArg, HandleId id)
{
    
    RootedObject obj(cx, objArg);

    JS_ASSERT(obj->isNative());
    JS_ASSERT(obj->isDelegate());

    
    if (JSID_IS_INT(id))
        return true;

    PurgeProtoChain(cx, obj->getProto(), id);

    





    if (obj->isCall()) {
        while ((obj = obj->enclosingScope()) != NULL) {
            if (!PurgeProtoChain(cx, obj, id))
                return false;
        }
    }

    return true;
}

RawShape
js_AddNativeProperty(JSContext *cx, HandleObject obj, HandleId id,
                     PropertyOp getter, StrictPropertyOp setter, uint32_t slot,
                     unsigned attrs, unsigned flags, int shortid)
{
    




    if (!js_PurgeScopeChain(cx, obj, id))
        return NULL;

    RawShape shape =
        JSObject::putProperty(cx, obj, id, getter, setter, slot, attrs, flags, shortid);
    if (!shape)
        return shape;

    if (JSID_IS_INT(id))
        JSObject::removeDenseElementForSparseIndex(cx, obj, JSID_TO_INT(id));

    return shape;
}

JSBool
baseops::DefineGeneric(JSContext *cx, HandleObject obj, HandleId id, HandleValue value,
                       PropertyOp getter, StrictPropertyOp setter, unsigned attrs)
{
    return DefineNativeProperty(cx, obj, id, value, getter, setter, attrs, 0, 0);
}

JSBool
baseops::DefineElement(JSContext *cx, HandleObject obj, uint32_t index, HandleValue value,
                       PropertyOp getter, StrictPropertyOp setter, unsigned attrs)
{
    Rooted<jsid> id(cx);
    if (index <= JSID_INT_MAX) {
        id = INT_TO_JSID(index);
        return DefineNativeProperty(cx, obj, id, value, getter, setter, attrs, 0, 0);
    }

    AutoRooterGetterSetter gsRoot(cx, attrs, &getter, &setter);

    if (!IndexToId(cx, index, &id))
        return false;

    return DefineNativeProperty(cx, obj, id, value, getter, setter, attrs, 0, 0);
}







static inline bool
CallAddPropertyHook(JSContext *cx, Class *clasp, HandleObject obj, HandleShape shape,
                    HandleValue nominal)
{
    if (clasp->addProperty != JS_PropertyStub) {
        
        RootedValue value(cx, nominal);

        Rooted<jsid> id(cx, shape->propid());
        if (!CallJSPropertyOp(cx, clasp->addProperty, obj, id, &value)) {
            obj->removeProperty(cx, shape->propid());
            return false;
        }
        if (value.get() != nominal) {
            if (shape->hasSlot())
                JSObject::nativeSetSlotWithType(cx, obj, shape, value);
        }
    }
    return true;
}

static inline bool
CallAddPropertyHookDense(JSContext *cx, Class *clasp, HandleObject obj, uint32_t index,
                         HandleValue nominal)
{
    
    if (obj->isArray()) {
        uint32_t length = obj->getArrayLength();
        if (index >= length)
            JSObject::setArrayLength(cx, obj, index + 1);
        return true;
    }

    if (clasp->addProperty != JS_PropertyStub) {
        
        RootedValue value(cx, nominal);

        Rooted<jsid> id(cx, INT_TO_JSID(index));
        if (!CallJSPropertyOp(cx, clasp->addProperty, obj, id, &value)) {
            JSObject::setDenseElementHole(cx, obj, index);
            return false;
        }
        if (value.get() != nominal)
            JSObject::setDenseElementWithType(cx, obj, index, value);
    }
    return true;
}

static inline bool
DefinePropertyOrElement(JSContext *cx, HandleObject obj, HandleId id,
                        PropertyOp getter, StrictPropertyOp setter,
                        unsigned attrs, unsigned flags, int shortid,
                        HandleValue value, bool callSetterAfterwards, bool setterIsStrict)
{
    
    if (JSID_IS_INT(id) &&
        getter == JS_PropertyStub &&
        setter == JS_StrictPropertyStub &&
        attrs == JSPROP_ENUMERATE &&
        (!obj->isIndexed() || !obj->nativeContains(cx, id)))
    {
        uint32_t index = JSID_TO_INT(id);
        JSObject::EnsureDenseResult result = obj->ensureDenseElements(cx, index, 1);
        if (result == JSObject::ED_FAILED)
            return false;
        if (result == JSObject::ED_OK) {
            obj->setDenseElementMaybeConvertDouble(index, value);
            return CallAddPropertyHookDense(cx, obj->getClass(), obj, index, value);
        }
    }

    AutoRooterGetterSetter gsRoot(cx, attrs, &getter, &setter);

    RootedShape shape(cx, JSObject::putProperty(cx, obj, id, getter, setter, SHAPE_INVALID_SLOT,
                                                attrs, flags, shortid));
    if (!shape)
        return false;

    if (shape->hasSlot())
        obj->nativeSetSlot(shape->slot(), value);

    



    if (JSID_IS_INT(id)) {
        uint32_t index = JSID_TO_INT(id);
        JSObject::removeDenseElementForSparseIndex(cx, obj, index);
        JSObject::EnsureDenseResult result = JSObject::maybeDensifySparseElements(cx, obj);
        if (result == JSObject::ED_FAILED)
            return false;
        if (result == JSObject::ED_OK) {
            JS_ASSERT(setter == JS_StrictPropertyStub);
            return CallAddPropertyHookDense(cx, obj->getClass(), obj, index, value);
        }
    }

    if (!CallAddPropertyHook(cx, obj->getClass(), obj, shape, value))
        return false;

    if (callSetterAfterwards && setter != JS_StrictPropertyStub) {
        RootedValue nvalue(cx, value);
        return js_NativeSet(cx, obj, obj, shape, setterIsStrict, &nvalue);
    }
    return true;
}

bool
js::DefineNativeProperty(JSContext *cx, HandleObject obj, HandleId id, HandleValue value,
                         PropertyOp getter, StrictPropertyOp setter, unsigned attrs,
                         unsigned flags, int shortid, unsigned defineHow )
{
    JS_ASSERT((defineHow & ~(DNP_CACHE_RESULT | DNP_DONT_PURGE |
                             DNP_SKIP_TYPE)) == 0);
    JS_ASSERT(!(attrs & JSPROP_NATIVE_ACCESSORS));

    AutoRooterGetterSetter gsRoot(cx, attrs, &getter, &setter);

    




    RootedShape shape(cx);
    if (attrs & (JSPROP_GETTER | JSPROP_SETTER)) {
        
        AddTypePropertyId(cx, obj, id, types::Type::UnknownType());
        MarkTypePropertyConfigured(cx, obj, id);

        




        RootedObject pobj(cx);
        if (!baseops::LookupProperty<CanGC>(cx, obj, id, &pobj, &shape))
            return false;
        if (shape && pobj == obj) {
            if (IsImplicitDenseElement(shape)) {
                if (!JSObject::sparsifyDenseElement(cx, obj, JSID_TO_INT(id)))
                    return false;
                shape = obj->nativeLookup(cx, id);
            }
            if (shape->isAccessorDescriptor()) {
                shape = JSObject::changeProperty(cx, obj, shape, attrs,
                                                 JSPROP_GETTER | JSPROP_SETTER,
                                                 (attrs & JSPROP_GETTER)
                                                 ? getter
                                                 : shape->getter(),
                                                 (attrs & JSPROP_SETTER)
                                                 ? setter
                                                 : shape->setter());
                if (!shape)
                    return false;
            } else {
                shape = NULL;
            }
        } else {
            shape = NULL;
        }
    }

    




    if (!(defineHow & DNP_DONT_PURGE)) {
        if (!js_PurgeScopeChain(cx, obj, id))
            return false;
    }

    
    Class *clasp = obj->getClass();
    if (!getter && !(attrs & JSPROP_GETTER))
        getter = clasp->getProperty;
    if (!setter && !(attrs & JSPROP_SETTER))
        setter = clasp->setProperty;

    if ((getter == JS_PropertyStub) && !(defineHow & DNP_SKIP_TYPE)) {
        



        AddTypePropertyId(cx, obj, id, value);
        if (attrs & JSPROP_READONLY)
            MarkTypePropertyConfigured(cx, obj, id);
    }

    if (!shape) {
        return DefinePropertyOrElement(cx, obj, id, getter, setter,
                                       attrs, flags, shortid, value, false, false);
    }

    if (shape->hasSlot())
        obj->nativeSetSlot(shape->slot(), value);

    return CallAddPropertyHook(cx, clasp, obj, shape, value);
}





















static JS_ALWAYS_INLINE JSBool
CallResolveOp(JSContext *cx, HandleObject obj, HandleId id, unsigned flags,
              MutableHandleObject objp, MutableHandleShape propp, bool *recursedp)
{
    Class *clasp = obj->getClass();
    JSResolveOp resolve = clasp->resolve;

    







    AutoResolving resolving(cx, obj, id);
    if (resolving.alreadyStarted()) {
        
        *recursedp = true;
        return true;
    }
    *recursedp = false;

    propp.set(NULL);

    if (clasp->flags & JSCLASS_NEW_RESOLVE) {
        JSNewResolveOp newresolve = reinterpret_cast<JSNewResolveOp>(resolve);
        if (flags == RESOLVE_INFER)
            flags = js_InferFlags(cx, 0);

        RootedObject obj2(cx, NULL);
        if (!newresolve(cx, obj, id, flags, &obj2))
            return false;

        





        if (!obj2)
            return true;

        if (!obj2->isNative()) {
            
            JS_ASSERT(obj2 != obj);
            return JSObject::lookupGeneric(cx, obj2, id, objp, propp);
        }

        objp.set(obj2);
    } else {
        if (!resolve(cx, obj, id))
            return false;

        objp.set(obj);
    }

    if (JSID_IS_INT(id) && objp->containsDenseElement(JSID_TO_INT(id))) {
        MarkDenseElementFound<CanGC>(propp);
        return true;
    }

    RawShape shape;
    if (!objp->nativeEmpty() && (shape = objp->nativeLookup(cx, id)))
        propp.set(shape);
    else
        objp.set(NULL);

    return true;
}

template <AllowGC allowGC>
static JS_ALWAYS_INLINE bool
LookupPropertyWithFlagsInline(JSContext *cx,
                              typename MaybeRooted<JSObject*, allowGC>::HandleType obj,
                              typename MaybeRooted<jsid, allowGC>::HandleType id,
                              unsigned flags,
                              typename MaybeRooted<JSObject*, allowGC>::MutableHandleType objp,
                              typename MaybeRooted<Shape*, allowGC>::MutableHandleType propp)
{
    
    typename MaybeRooted<JSObject*, allowGC>::RootType current(cx, obj);

    while (true) {
        
        {
            if (JSID_IS_INT(id) && current->containsDenseElement(JSID_TO_INT(id))) {
                objp.set(current);
                MarkDenseElementFound<allowGC>(propp);
                return true;
            }

            RawShape shape = current->nativeLookup(cx, id);
            if (shape) {
                objp.set(current);
                propp.set(shape);
                return true;
            }
        }

        
        if (current->getClass()->resolve != JS_ResolveStub) {
            if (!allowGC)
                return false;
            bool recursed;
            if (!CallResolveOp(cx,
                               MaybeRooted<JSObject*, allowGC>::toHandle(current),
                               MaybeRooted<jsid, allowGC>::toHandle(id),
                               flags,
                               MaybeRooted<JSObject*, allowGC>::toMutableHandle(objp),
                               MaybeRooted<Shape*, allowGC>::toMutableHandle(propp),
                               &recursed))
            {
                return false;
            }

            if (recursed)
                break;
            if (propp) {
                



                return true;
            }
        }

        typename MaybeRooted<JSObject*, allowGC>::RootType proto(cx, current->getProto());

        if (!proto)
            break;
        if (!proto->isNative()) {
            if (!allowGC)
                return false;
            return JSObject::lookupGeneric(cx,
                                           MaybeRooted<JSObject*, allowGC>::toHandle(proto),
                                           MaybeRooted<jsid, allowGC>::toHandle(id),
                                           MaybeRooted<JSObject*, allowGC>::toMutableHandle(objp),
                                           MaybeRooted<Shape*, allowGC>::toMutableHandle(propp));
        }

        current = proto;
    }

    objp.set(NULL);
    propp.set(NULL);
    return true;
}

template <AllowGC allowGC>
JSBool
baseops::LookupProperty(JSContext *cx,
                        typename MaybeRooted<JSObject*, allowGC>::HandleType obj,
                        typename MaybeRooted<jsid, allowGC>::HandleType id,
                        typename MaybeRooted<JSObject*, allowGC>::MutableHandleType objp,
                        typename MaybeRooted<Shape*, allowGC>::MutableHandleType propp)
{
    return LookupPropertyWithFlagsInline<allowGC>(cx, obj, id, cx->resolveFlags, objp, propp);
}

template JSBool
baseops::LookupProperty<CanGC>(JSContext *cx, HandleObject obj, HandleId id,
                               MutableHandleObject objp, MutableHandleShape propp);

template JSBool
baseops::LookupProperty<NoGC>(JSContext *cx, JSObject *obj, jsid id,
                              FakeMutableHandle<JSObject*> objp,
                              FakeMutableHandle<Shape*> propp);

JSBool
baseops::LookupElement(JSContext *cx, HandleObject obj, uint32_t index,
                       MutableHandleObject objp, MutableHandleShape propp)
{
    RootedId id(cx);
    if (!IndexToId(cx, index, &id))
        return false;

    return LookupPropertyWithFlagsInline<CanGC>(cx, obj, id, cx->resolveFlags, objp, propp);
}

bool
js::LookupPropertyWithFlags(JSContext *cx, HandleObject obj, HandleId id, unsigned flags,
                            MutableHandleObject objp, MutableHandleShape propp)
{
    return LookupPropertyWithFlagsInline<CanGC>(cx, obj, id, flags, objp, propp);
}

bool
js::LookupName(JSContext *cx, HandlePropertyName name, HandleObject scopeChain,
               MutableHandleObject objp, MutableHandleObject pobjp, MutableHandleShape propp)
{
    RootedId id(cx, NameToId(name));

    for (RootedObject scope(cx, scopeChain); scope; scope = scope->enclosingScope()) {
        if (!JSObject::lookupGeneric(cx, scope, id, pobjp, propp))
            return false;
        if (propp) {
            objp.set(scope);
            return true;
        }
    }

    objp.set(NULL);
    pobjp.set(NULL);
    propp.set(NULL);
    return true;
}

bool
js::LookupNameNoGC(JSContext *cx, PropertyName *name, JSObject *scopeChain,
                   JSObject **objp, JSObject **pobjp, Shape **propp)
{
    AutoAssertNoException nogc(cx);

    JS_ASSERT(!*objp && !*pobjp && !*propp);

    for (JSObject *scope = scopeChain; scope; scope = scope->enclosingScope()) {
        if (scope->getOps()->lookupGeneric)
            return false;
        if (!LookupPropertyWithFlagsInline<NoGC>(cx, scope, NameToId(name),
                                                 cx->resolveFlags, pobjp, propp))
        {
            return false;
        }
        if (*propp) {
            *objp = scope;
            return true;
        }
    }

    return true;
}

bool
js::LookupNameWithGlobalDefault(JSContext *cx, HandlePropertyName name, HandleObject scopeChain,
                                MutableHandleObject objp)
{
    RootedId id(cx, NameToId(name));

    RootedObject pobj(cx);
    RootedShape prop(cx);

    RootedObject scope(cx, scopeChain);
    for (; !scope->isGlobal(); scope = scope->enclosingScope()) {
        if (!JSObject::lookupGeneric(cx, scope, id, &pobj, &prop))
            return false;
        if (prop)
            break;
    }

    objp.set(scope);
    return true;
}

template <AllowGC allowGC>
static JS_ALWAYS_INLINE JSBool
NativeGetInline(JSContext *cx,
                typename MaybeRooted<JSObject*, allowGC>::HandleType obj,
                typename MaybeRooted<JSObject*, allowGC>::HandleType receiver,
                typename MaybeRooted<JSObject*, allowGC>::HandleType pobj,
                typename MaybeRooted<Shape*, allowGC>::HandleType shape,
                unsigned getHow,
                typename MaybeRooted<Value, allowGC>::MutableHandleType vp)
{
    JS_ASSERT(pobj->isNative());

    if (shape->hasSlot()) {
        vp.set(pobj->nativeGetSlot(shape->slot()));
        JS_ASSERT(!vp.isMagic());
        JS_ASSERT_IF(!pobj->hasSingletonType() && shape->hasDefaultGetter(),
                     js::types::TypeHasProperty(cx, pobj->type(), shape->propid(), vp));
    } else {
        vp.setUndefined();
    }
    if (shape->hasDefaultGetter())
        return true;

    {
        jsbytecode *pc;
        RawScript script = cx->stack.currentScript(&pc);
        if (script && script->hasAnalysis()) {
            analyze::Bytecode *code = script->analysis()->maybeCode(pc);
            if (code)
                code->accessGetter = true;
        }
    }

    if (!allowGC)
        return false;

    if (!shape->get(cx,
                    MaybeRooted<JSObject*, allowGC>::toHandle(receiver),
                    MaybeRooted<JSObject*, allowGC>::toHandle(obj),
                    MaybeRooted<JSObject*, allowGC>::toHandle(pobj),
                    MaybeRooted<Value, allowGC>::toMutableHandle(vp)))
    {
        return false;
    }

    
    if (shape->hasSlot() && pobj->nativeContains(cx, shape))
        pobj->nativeSetSlot(shape->slot(), vp);

    return true;
}

JSBool
js_NativeGet(JSContext *cx, Handle<JSObject*> obj, Handle<JSObject*> pobj, Handle<Shape*> shape,
             unsigned getHow, MutableHandle<Value> vp)
{
    return NativeGetInline<CanGC>(cx, obj, obj, pobj, shape, getHow, vp);
}

JSBool
js_NativeSet(JSContext *cx, Handle<JSObject*> obj, Handle<JSObject*> receiver,
             HandleShape shape, bool strict, MutableHandleValue vp)
{
    JS_ASSERT(obj->isNative());

    if (shape->hasSlot()) {
        uint32_t slot = shape->slot();

        
        if (shape->hasDefaultSetter()) {
            AddTypePropertyId(cx, obj, shape->propid(), vp);
            obj->nativeSetSlot(slot, vp);
            return true;
        }
    } else {
        





        if (!shape->hasGetterValue() && shape->hasDefaultSetter())
            return js_ReportGetterOnlyAssignment(cx);
    }

    RootedValue ovp(cx, vp);

    uint32_t sample = cx->runtime->propertyRemovals;
    if (!shape->set(cx, obj, receiver, strict, vp))
        return false;

    



    if (shape->hasSlot() &&
        (JS_LIKELY(cx->runtime->propertyRemovals == sample) ||
         obj->nativeContains(cx, shape)))
     {
        AddTypePropertyId(cx, obj, shape->propid(), ovp);
        obj->setSlot(shape->slot(), vp);
    }

    return true;
}

template <AllowGC allowGC>
static JS_ALWAYS_INLINE JSBool
GetPropertyHelperInline(JSContext *cx,
                        typename MaybeRooted<JSObject*, allowGC>::HandleType obj,
                        typename MaybeRooted<JSObject*, allowGC>::HandleType receiver,
                        typename MaybeRooted<jsid, allowGC>::HandleType id,
                        uint32_t getHow,
                        typename MaybeRooted<Value, allowGC>::MutableHandleType vp)
{
    
    typename MaybeRooted<JSObject*, allowGC>::RootType obj2(cx);
    typename MaybeRooted<Shape*, allowGC>::RootType shape(cx);
    if (!LookupPropertyWithFlagsInline<allowGC>(cx, obj, id, cx->resolveFlags, &obj2, &shape))
        return false;

    if (!shape) {
        if (!allowGC)
            return false;

        vp.setUndefined();

        if (!CallJSPropertyOp(cx, obj->getClass()->getProperty,
                              MaybeRooted<JSObject*, allowGC>::toHandle(obj),
                              MaybeRooted<jsid, allowGC>::toHandle(id),
                              MaybeRooted<Value, allowGC>::toMutableHandle(vp)))
        {
            return false;
        }

        
        if (!vp.isUndefined())
            AddTypePropertyId(cx, obj, id, vp);

        



        jsbytecode *pc;
        if (vp.isUndefined() && ((pc = js_GetCurrentBytecodePC(cx)) != NULL)) {
            JSOp op = (JSOp) *pc;

            if (op == JSOP_GETXPROP) {
                
                JSAutoByteString printable;
                if (js_ValueToPrintable(cx, IdToValue(id), &printable))
                    js_ReportIsNotDefined(cx, printable.ptr());
                return false;
            }

            
            if (!cx->hasStrictOption() || (op != JSOP_GETPROP && op != JSOP_GETELEM))
                return true;

            
            RootedScript script(cx, cx->stack.currentScript());
            if (!script || script->warnedAboutUndefinedProp)
                return true;

            



            if (JSID_IS_ATOM(id, cx->names().iteratorIntrinsic))
                return JS_TRUE;

            
            if (cx->resolveFlags == RESOLVE_INFER) {
                pc += js_CodeSpec[op].length;
                if (Detecting(cx, script, pc))
                    return JS_TRUE;
            }

            unsigned flags = JSREPORT_WARNING | JSREPORT_STRICT;
            cx->stack.currentScript()->warnedAboutUndefinedProp = true;

            
            RootedValue val(cx, IdToValue(id));
            if (!js_ReportValueErrorFlags(cx, flags, JSMSG_UNDEFINED_PROP,
                                          JSDVG_IGNORE_STACK, val, NullPtr(),
                                          NULL, NULL))
            {
                return false;
            }
        }
        return JS_TRUE;
    }

    if (!obj2->isNative()) {
        if (!allowGC)
            return false;
        HandleObject obj2Handle = MaybeRooted<JSObject*, allowGC>::toHandle(obj2);
        HandleObject receiverHandle = MaybeRooted<JSObject*, allowGC>::toHandle(receiver);
        HandleId idHandle = MaybeRooted<jsid, allowGC>::toHandle(id);
        MutableHandleValue vpHandle = MaybeRooted<Value, allowGC>::toMutableHandle(vp);
        return obj2->isProxy()
               ? Proxy::get(cx, obj2Handle, receiverHandle, idHandle, vpHandle)
               : JSObject::getGeneric(cx, obj2Handle, obj2Handle, idHandle, vpHandle);
    }

    if (IsImplicitDenseElement(shape)) {
        vp.set(obj2->getDenseElement(JSID_TO_INT(id)));
        return true;
    }

    if (getHow & JSGET_CACHE_RESULT)
        cx->propertyCache().fill(cx, obj, obj2, shape);

    
    if (!NativeGetInline<allowGC>(cx, obj, receiver, obj2, shape, getHow, vp))
        return JS_FALSE;

    return JS_TRUE;
}

bool
js::GetPropertyHelper(JSContext *cx, HandleObject obj, HandleId id, uint32_t getHow, MutableHandleValue vp)
{
    return GetPropertyHelperInline<CanGC>(cx, obj, obj, id, getHow, vp);
}

JSBool
baseops::GetProperty(JSContext *cx, HandleObject obj, HandleObject receiver, HandleId id, MutableHandleValue vp)
{
    
    return GetPropertyHelperInline<CanGC>(cx, obj, receiver, id, 0, vp);
}

JSBool
baseops::GetPropertyNoGC(JSContext *cx, JSObject *obj, JSObject *receiver, jsid id, Value *vp)
{
    AutoAssertNoException nogc(cx);
    return GetPropertyHelperInline<NoGC>(cx, obj, receiver, id, 0, vp);
}

JSBool
baseops::GetElement(JSContext *cx, HandleObject obj, HandleObject receiver, uint32_t index,
                    MutableHandleValue vp)
{
    RootedId id(cx);
    if (!IndexToId(cx, index, &id))
        return false;

    
    return GetPropertyHelperInline<CanGC>(cx, obj, receiver, id, 0, vp);
}

JSBool
baseops::GetPropertyDefault(JSContext *cx, HandleObject obj, HandleId id, HandleValue def,
                            MutableHandleValue vp)
{
    RootedShape prop(cx);
    RootedObject obj2(cx);
    if (!LookupPropertyWithFlags(cx, obj, id, 0, &obj2, &prop))
        return false;

    if (!prop) {
        vp.set(def);
        return true;
    }

    return baseops::GetProperty(cx, obj2, id, vp);
}

JSBool
js::GetMethod(JSContext *cx, HandleObject obj, HandleId id, unsigned getHow, MutableHandleValue vp)
{
    JSAutoResolveFlags rf(cx, 0);

    GenericIdOp op = obj->getOps()->getGeneric;
    if (!op)
        return GetPropertyHelper(cx, obj, id, getHow, vp);

    return op(cx, obj, obj, id, vp);
}

static bool
MaybeReportUndeclaredVarAssignment(JSContext *cx, JSString *propname)
{
    {
        RawScript script = cx->stack.currentScript(NULL, ContextStack::ALLOW_CROSS_COMPARTMENT);
        if (!script)
            return true;

        
        if (!script->strict && !cx->hasStrictOption())
            return true;
    }

    JSAutoByteString bytes(cx, propname);
    return !!bytes &&
           JS_ReportErrorFlagsAndNumber(cx,
                                        (JSREPORT_WARNING | JSREPORT_STRICT
                                         | JSREPORT_STRICT_MODE_ERROR),
                                        js_GetErrorMessage, NULL,
                                        JSMSG_UNDECLARED_VAR, bytes.ptr());
}

bool
js::ReportIfUndeclaredVarAssignment(JSContext *cx, HandleString propname)
{
    {
        jsbytecode *pc;
        RawScript script = cx->stack.currentScript(&pc, ContextStack::ALLOW_CROSS_COMPARTMENT);
        if (!script)
            return true;

        
        if (!script->strict && !cx->hasStrictOption())
            return true;

        













        MOZ_ASSERT(*pc != JSOP_INCNAME);
        MOZ_ASSERT(*pc != JSOP_INCGNAME);
        MOZ_ASSERT(*pc != JSOP_NAMEINC);
        MOZ_ASSERT(*pc != JSOP_GNAMEINC);
        MOZ_ASSERT(*pc != JSOP_DECNAME);
        MOZ_ASSERT(*pc != JSOP_DECGNAME);
        MOZ_ASSERT(*pc != JSOP_NAMEDEC);
        MOZ_ASSERT(*pc != JSOP_GNAMEDEC);
        MOZ_ASSERT(*pc != JSOP_NAME);
        MOZ_ASSERT(*pc != JSOP_GETGNAME);
        if (*pc != JSOP_SETNAME && *pc != JSOP_SETGNAME)
            return true;
    }

    JSAutoByteString bytes(cx, propname);
    return !!bytes &&
           JS_ReportErrorFlagsAndNumber(cx,
                                        JSREPORT_WARNING | JSREPORT_STRICT |
                                        JSREPORT_STRICT_MODE_ERROR,
                                        js_GetErrorMessage, NULL,
                                        JSMSG_UNDECLARED_VAR, bytes.ptr());
}

bool
JSObject::reportReadOnly(JSContext *cx, jsid id, unsigned report)
{
    RootedValue val(cx, IdToValue(id));
    return js_ReportValueErrorFlags(cx, report, JSMSG_READ_ONLY,
                                    JSDVG_IGNORE_STACK, val, NullPtr(),
                                    NULL, NULL);
}

bool
JSObject::reportNotConfigurable(JSContext *cx, jsid id, unsigned report)
{
    RootedValue val(cx, IdToValue(id));
    return js_ReportValueErrorFlags(cx, report, JSMSG_CANT_DELETE,
                                    JSDVG_IGNORE_STACK, val, NullPtr(),
                                    NULL, NULL);
}

bool
JSObject::reportNotExtensible(JSContext *cx, unsigned report)
{
    RootedValue val(cx, ObjectValue(*this));
    return js_ReportValueErrorFlags(cx, report, JSMSG_OBJECT_NOT_EXTENSIBLE,
                                    JSDVG_IGNORE_STACK, val, NullPtr(),
                                    NULL, NULL);
}

bool
JSObject::callMethod(JSContext *cx, HandleId id, unsigned argc, Value *argv, MutableHandleValue vp)
{
    RootedValue fval(cx);
    Rooted<JSObject*> obj(cx, this);
    return GetMethod(cx, obj, id, 0, &fval) &&
           Invoke(cx, ObjectValue(*obj), fval, argc, argv, vp.address());
}

JSBool
baseops::SetPropertyHelper(JSContext *cx, HandleObject obj, HandleObject receiver, HandleId id,
                           unsigned defineHow, MutableHandleValue vp, JSBool strict)
{
    unsigned attrs, flags;
    int shortid;
    Class *clasp;
    PropertyOp getter;
    StrictPropertyOp setter;

    JS_ASSERT((defineHow & ~(DNP_CACHE_RESULT | DNP_UNQUALIFIED)) == 0);

    if (JS_UNLIKELY(obj->watched())) {
        
        WatchpointMap *wpmap = cx->compartment->watchpointMap;
        if (wpmap && !wpmap->triggerWatchpoint(cx, obj, id, vp))
            return false;
    }

    RootedObject pobj(cx);
    RootedShape shape(cx);
    if (!LookupPropertyWithFlags(cx, obj, id, cx->resolveFlags, &pobj, &shape))
        return false;
    if (shape) {
        if (!pobj->isNative()) {
            if (pobj->isProxy()) {
                AutoPropertyDescriptorRooter pd(cx);
                if (!Proxy::getPropertyDescriptor(cx, pobj, id, &pd, JSRESOLVE_ASSIGNING))
                    return false;

                if ((pd.attrs & (JSPROP_SHARED | JSPROP_SHADOWABLE)) == JSPROP_SHARED) {
                    return !pd.setter ||
                           CallSetter(cx, receiver, id, pd.setter, pd.attrs, pd.shortid, strict,
                                      vp);
                }

                if (pd.attrs & JSPROP_READONLY) {
                    if (strict)
                        return JSObject::reportReadOnly(cx, id, JSREPORT_ERROR);
                    if (cx->hasStrictOption())
                        return JSObject::reportReadOnly(cx, id, JSREPORT_STRICT | JSREPORT_WARNING);
                    return true;
                }
            }

            shape = NULL;
        }
    } else {
        
        JS_ASSERT(!obj->isBlock());

        if (obj->isGlobal() &&
            (defineHow & DNP_UNQUALIFIED) &&
            !MaybeReportUndeclaredVarAssignment(cx, JSID_TO_STRING(id)))
        {
            return false;
        }
    }

    



    attrs = JSPROP_ENUMERATE;
    flags = 0;
    shortid = 0;
    clasp = obj->getClass();
    getter = clasp->getProperty;
    setter = clasp->setProperty;

    if (IsImplicitDenseElement(shape)) {
        
        if (pobj != obj)
            shape = NULL;
    } else if (shape) {
        
        if (shape->isAccessorDescriptor()) {
            if (shape->hasDefaultSetter())
                return js_ReportGetterOnlyAssignment(cx);
        } else {
            JS_ASSERT(shape->isDataDescriptor());

            if (!shape->writable()) {
                
                if (strict)
                    return JSObject::reportReadOnly(cx, id, JSREPORT_ERROR);
                if (cx->hasStrictOption())
                    return JSObject::reportReadOnly(cx, id, JSREPORT_STRICT | JSREPORT_WARNING);
                return JS_TRUE;
            }
        }

        attrs = shape->attributes();
        if (pobj != obj) {
            


            if (!shape->shadowable()) {
                if (defineHow & DNP_CACHE_RESULT)
                    cx->propertyCache().fill(cx, obj, pobj, shape);

                if (shape->hasDefaultSetter() && !shape->hasGetterValue())
                    return JS_TRUE;

                return shape->set(cx, obj, receiver, strict, vp);
            }

            














            if (!shape->hasSlot()) {
                if (shape->hasShortID()) {
                    flags = Shape::HAS_SHORTID;
                    shortid = shape->shortid();
                }
                attrs &= ~JSPROP_SHARED;
                getter = shape->getter();
                setter = shape->setter();
            } else {
                
                attrs = JSPROP_ENUMERATE;
            }

            



            shape = NULL;
        }
    }

    if (IsImplicitDenseElement(shape)) {
        JSObject::setDenseElementWithType(cx, obj, JSID_TO_INT(id), vp);
        return true;
    }

    if (!shape) {
        if (!obj->isExtensible()) {
            
            if (strict)
                return obj->reportNotExtensible(cx);
            if (cx->hasStrictOption())
                return obj->reportNotExtensible(cx, JSREPORT_STRICT | JSREPORT_WARNING);
            return true;
        }

        
        if (!js_PurgeScopeChain(cx, obj, id))
            return false;

        if (getter == JS_PropertyStub)
            AddTypePropertyId(cx, obj, id, vp);

        return DefinePropertyOrElement(cx, obj, id, getter, setter,
                                       attrs, flags, shortid, vp, true, strict);
    }

    if (defineHow & DNP_CACHE_RESULT)
        cx->propertyCache().fill(cx, obj, obj, shape);

    return js_NativeSet(cx, obj, receiver, shape, strict, vp);
}

JSBool
baseops::SetElementHelper(JSContext *cx, HandleObject obj, HandleObject receiver, uint32_t index,
                          unsigned defineHow, MutableHandleValue vp, JSBool strict)
{
    RootedId id(cx);
    if (!IndexToId(cx, index, &id))
        return false;
    return baseops::SetPropertyHelper(cx, obj, receiver, id, defineHow, vp, strict);
}

JSBool
baseops::GetAttributes(JSContext *cx, HandleObject obj, HandleId id, unsigned *attrsp)
{
    RootedObject nobj(cx);
    RootedShape shape(cx);
    if (!baseops::LookupProperty<CanGC>(cx, obj, id, &nobj, &shape))
        return false;
    if (!shape) {
        *attrsp = 0;
        return true;
    }
    if (!nobj->isNative())
        return JSObject::getGenericAttributes(cx, nobj, id, attrsp);

    *attrsp = GetShapeAttributes(shape);
    return true;
}

JSBool
baseops::GetElementAttributes(JSContext *cx, HandleObject obj, uint32_t index, unsigned *attrsp)
{
    RootedObject nobj(cx);
    RootedShape shape(cx);
    if (!baseops::LookupElement(cx, obj, index, &nobj, &shape))
        return false;
    if (!shape) {
        *attrsp = 0;
        return true;
    }
    if (!nobj->isNative())
        return JSObject::getElementAttributes(cx, nobj, index, attrsp);

    *attrsp = GetShapeAttributes(shape);
    return true;
}

JSBool
baseops::SetAttributes(JSContext *cx, HandleObject obj, HandleId id, unsigned *attrsp)
{
    RootedObject nobj(cx);
    RootedShape shape(cx);
    if (!baseops::LookupProperty<CanGC>(cx, obj, id, &nobj, &shape))
        return false;
    if (!shape)
        return true;
    if (nobj->isNative() && IsImplicitDenseElement(shape)) {
        if (!JSObject::sparsifyDenseElement(cx, nobj, JSID_TO_INT(id)))
            return false;
        shape = obj->nativeLookup(cx, id);
    }
    return nobj->isNative()
           ? JSObject::changePropertyAttributes(cx, nobj, shape, *attrsp)
           : JSObject::setGenericAttributes(cx, nobj, id, attrsp);
}

JSBool
baseops::SetElementAttributes(JSContext *cx, HandleObject obj, uint32_t index, unsigned *attrsp)
{
    RootedObject nobj(cx);
    RootedShape shape(cx);
    if (!baseops::LookupElement(cx, obj, index, &nobj, &shape))
        return false;
    if (!shape)
        return true;
    if (nobj->isNative() && IsImplicitDenseElement(shape)) {
        if (!JSObject::sparsifyDenseElement(cx, obj, index))
            return false;
        RawId id = INT_TO_JSID(index);
        shape = obj->nativeLookup(cx, HandleId::fromMarkedLocation(&id)); 
    }
    return nobj->isNative()
           ? JSObject::changePropertyAttributes(cx, nobj, shape, *attrsp)
           : JSObject::setElementAttributes(cx, nobj, index, attrsp);
}

JSBool
baseops::DeleteGeneric(JSContext *cx, HandleObject obj, HandleId id, MutableHandleValue rval, JSBool strict)
{
    rval.setBoolean(true);

    RootedObject proto(cx);
    RootedShape shape(cx);
    if (!baseops::LookupProperty<CanGC>(cx, obj, id, &proto, &shape))
        return false;
    if (!shape || proto != obj) {
        



        return CallJSPropertyOp(cx, obj->getClass()->delProperty, obj, id, rval);
    }

    GCPoke(cx->runtime);

    if (IsImplicitDenseElement(shape)) {
        if (!CallJSPropertyOp(cx, obj->getClass()->delProperty, obj, id, rval))
            return false;
        if (rval.isFalse())
            return true;

        JSObject::setDenseElementHole(cx, obj, JSID_TO_INT(id));
        return js_SuppressDeletedProperty(cx, obj, id);
    }

    if (!shape->configurable()) {
        if (strict)
            return obj->reportNotConfigurable(cx, id);
        rval.setBoolean(false);
        return true;
    }

    RootedId userid(cx);
    if (!shape->getUserId(cx, &userid))
        return false;

    if (!CallJSPropertyOp(cx, obj->getClass()->delProperty, obj, userid, rval))
        return false;
    if (rval.isFalse())
        return true;

    return obj->removeProperty(cx, id) && js_SuppressDeletedProperty(cx, obj, id);
}

JSBool
baseops::DeleteProperty(JSContext *cx, HandleObject obj, HandlePropertyName name,
                        MutableHandleValue rval, JSBool strict)
{
    Rooted<jsid> id(cx, NameToId(name));
    return baseops::DeleteGeneric(cx, obj, id, rval, strict);
}

JSBool
baseops::DeleteElement(JSContext *cx, HandleObject obj, uint32_t index,
                       MutableHandleValue rval, JSBool strict)
{
    RootedId id(cx);
    if (!IndexToId(cx, index, &id))
        return false;
    return baseops::DeleteGeneric(cx, obj, id, rval, strict);
}

JSBool
baseops::DeleteSpecial(JSContext *cx, HandleObject obj, HandleSpecialId sid,
                       MutableHandleValue rval, JSBool strict)
{
    Rooted<jsid> id(cx, SPECIALID_TO_JSID(sid));
    return baseops::DeleteGeneric(cx, obj, id, rval, strict);
}

bool
js::HasDataProperty(JSContext *cx, JSObject *obj, jsid id, Value *vp)
{
    if (JSID_IS_INT(id) && obj->containsDenseElement(JSID_TO_INT(id))) {
        *vp = obj->getDenseElement(JSID_TO_INT(id));
        return true;
    }

    if (RawShape shape = obj->nativeLookup(cx, id)) {
        if (shape->hasDefaultGetter() && shape->hasSlot()) {
            *vp = obj->nativeGetSlot(shape->slot());
            return true;
        }
    }

    return false;
}









static bool
MaybeCallMethod(JSContext *cx, HandleObject obj, Handle<jsid> id, MutableHandleValue vp)
{
    if (!GetMethod(cx, obj, id, 0, vp))
        return false;
    if (!js_IsCallable(vp)) {
        vp.setObject(*obj);
        return true;
    }
    return Invoke(cx, ObjectValue(*obj), vp, 0, NULL, vp.address());
}

JSBool
js::DefaultValue(JSContext *cx, HandleObject obj, JSType hint, MutableHandleValue vp)
{
    JS_ASSERT(hint == JSTYPE_NUMBER || hint == JSTYPE_STRING || hint == JSTYPE_VOID);

    Rooted<jsid> id(cx);

    Class *clasp = obj->getClass();
    if (hint == JSTYPE_STRING) {
        id = NameToId(cx->names().toString);

        
        if (clasp == &StringClass) {
            if (ClassMethodIsNative(cx, obj, &StringClass, id, js_str_toString)) {
                vp.setString(obj->asString().unbox());
                return true;
            }
        }

        if (!MaybeCallMethod(cx, obj, id, vp))
            return false;
        if (vp.isPrimitive())
            return true;

        id = NameToId(cx->names().valueOf);
        if (!MaybeCallMethod(cx, obj, id, vp))
            return false;
        if (vp.isPrimitive())
            return true;
    } else {

        
        if (clasp == &StringClass) {
            id = NameToId(cx->names().valueOf);
            if (ClassMethodIsNative(cx, obj, &StringClass, id, js_str_toString)) {
                vp.setString(obj->asString().unbox());
                return true;
            }
        }

        
        if (clasp == &NumberClass) {
            id = NameToId(cx->names().valueOf);
            if (ClassMethodIsNative(cx, obj, &NumberClass, id, js_num_valueOf)) {
                vp.setNumber(obj->asNumber().unbox());
                return true;
            }
        }

        id = NameToId(cx->names().valueOf);
        if (!MaybeCallMethod(cx, obj, id, vp))
            return false;
        if (vp.isPrimitive())
            return true;

        id = NameToId(cx->names().toString);
        if (!MaybeCallMethod(cx, obj, id, vp))
            return false;
        if (vp.isPrimitive())
            return true;
    }

    
    RootedString str(cx);
    if (hint == JSTYPE_STRING) {
        str = JS_InternString(cx, clasp->name);
        if (!str)
            return false;
    } else {
        str = NULL;
    }

    RootedValue val(cx, ObjectValue(*obj));
    js_ReportValueError2(cx, JSMSG_CANT_CONVERT_TO, JSDVG_SEARCH_STACK, val, str,
                         (hint == JSTYPE_VOID) ? "primitive type" : TypeStrings[hint]);
    return false;
}

JS_FRIEND_API(JSBool)
JS_EnumerateState(JSContext *cx, JSHandleObject obj, JSIterateOp enum_op,
                  JSMutableHandleValue statep, JSMutableHandleId idp)
{
    
    Class *clasp = obj->getClass();
    JSEnumerateOp enumerate = clasp->enumerate;
    if (clasp->flags & JSCLASS_NEW_ENUMERATE) {
        JS_ASSERT(enumerate != JS_EnumerateStub);
        return ((JSNewEnumerateOp) enumerate)(cx, obj, enum_op, statep, idp);
    }

    if (!enumerate(cx, obj))
        return false;

    
    JS_ASSERT(enum_op == JSENUMERATE_INIT || enum_op == JSENUMERATE_INIT_ALL);
    statep.setMagic(JS_NATIVE_ENUMERATE);
    return true;
}

JSBool
js::CheckAccess(JSContext *cx, JSObject *obj_, HandleId id, JSAccessMode mode,
                MutableHandleValue vp, unsigned *attrsp)
{
    JSBool writing;
    RootedObject obj(cx, obj_), pobj(cx);

    while (JS_UNLIKELY(obj->isWith()))
        obj = obj->getProto();

    writing = (mode & JSACC_WRITE) != 0;
    switch (mode & JSACC_TYPEMASK) {
      case JSACC_PROTO:
        pobj = obj;
        if (!writing) {
            RootedObject proto(cx);
            if (!JSObject::getProto(cx, obj, &proto))
                return JS_FALSE;
            vp.setObjectOrNull(proto);
        }
        *attrsp = JSPROP_PERMANENT;
        break;

      default:
        RootedShape shape(cx);
        if (!JSObject::lookupGeneric(cx, obj, id, &pobj, &shape))
            return JS_FALSE;
        if (!shape) {
            if (!writing)
                vp.setUndefined();
            *attrsp = 0;
            pobj = obj;
            break;
        }

        if (!pobj->isNative()) {
            if (!writing) {
                    vp.setUndefined();
                *attrsp = 0;
            }
            break;
        }

        *attrsp = GetShapeAttributes(shape);

        if (!writing) {
            if (IsImplicitDenseElement(shape)) {
                vp.set(pobj->getDenseElement(JSID_TO_INT(id)));
            } else {
                if (shape->hasSlot())
                    vp.set(pobj->nativeGetSlot(shape->slot()));
                else
                    vp.setUndefined();
            }
        }
    }

    JS_ASSERT_IF(*attrsp & JSPROP_READONLY, !(*attrsp & (JSPROP_GETTER | JSPROP_SETTER)));

    











    JSCheckAccessOp check = pobj->getClass()->checkAccess;
    if (!check)
        check = cx->runtime->securityCallbacks->checkObjectAccess;
    return !check || check(cx, pobj, id, mode, vp);
}

JSType
baseops::TypeOf(JSContext *cx, HandleObject obj)
{
    if (EmulatesUndefined(obj))
        return JSTYPE_VOID;
    if (obj->isCallable())
        return JSTYPE_FUNCTION;
    return JSTYPE_OBJECT;
}

bool
js::IsDelegate(JSContext *cx, HandleObject obj, const js::Value &v, bool *result)
{
    if (v.isPrimitive()) {
        *result = false;
        return true;
    }
    RootedObject obj2(cx, &v.toObject());
    for (;;) {
        if (!JSObject::getProto(cx, obj2, &obj2))
            return false;
        if (!obj2) {
            *result = false;
            return true;
        }
        if (obj2 == obj) {
            *result = true;
            return true;
        }
    }
}





bool
js_GetClassPrototype(JSContext *cx, JSProtoKey protoKey, MutableHandleObject protop, Class *clasp)
{
    JS_ASSERT(JSProto_Null <= protoKey);
    JS_ASSERT(protoKey < JSProto_LIMIT);

    if (protoKey != JSProto_Null) {
        const Value &v = cx->global()->getReservedSlot(JSProto_LIMIT + protoKey);
        if (v.isObject()) {
            protop.set(&v.toObject());
            return true;
        }
    }

    RootedValue v(cx);
    if (!js_FindClassObject(cx, protoKey, &v, clasp))
        return false;

    if (IsFunctionObject(v)) {
        RootedObject ctor(cx, &v.get().toObject());
        if (!JSObject::getProperty(cx, ctor, ctor, cx->names().classPrototype, &v))
            return false;
    }

    protop.set(v.get().isObject() ? &v.get().toObject() : NULL);
    return true;
}

JSObject *
PrimitiveToObject(JSContext *cx, const Value &v)
{
    if (v.isString()) {
        Rooted<JSString*> str(cx, v.toString());
        return StringObject::create(cx, str);
    }
    if (v.isNumber())
        return NumberObject::create(cx, v.toNumber());

    JS_ASSERT(v.isBoolean());
    return BooleanObject::create(cx, v.toBoolean());
}

JSBool
js_PrimitiveToObject(JSContext *cx, Value *vp)
{
    JSObject *obj = PrimitiveToObject(cx, *vp);
    if (!obj)
        return false;

    vp->setObject(*obj);
    return true;
}

JSBool
js_ValueToObjectOrNull(JSContext *cx, const Value &v, MutableHandleObject objp)
{
    JSObject *obj;

    if (v.isObjectOrNull()) {
        obj = v.toObjectOrNull();
    } else if (v.isUndefined()) {
        obj = NULL;
    } else {
        obj = PrimitiveToObject(cx, v);
        if (!obj)
            return false;
    }
    objp.set(obj);
    return true;
}


JSObject *
js::ToObjectSlow(JSContext *cx, HandleValue val, bool reportScanStack)
{
    JS_ASSERT(!val.isMagic());
    JS_ASSERT(!val.isObject());

    if (val.isNullOrUndefined()) {
        if (reportScanStack) {
            js_ReportIsNullOrUndefined(cx, JSDVG_SEARCH_STACK, val, NullPtr());
        } else {
            JS_ReportErrorNumber(cx, js_GetErrorMessage, NULL, JSMSG_CANT_CONVERT_TO,
                                 val.isNull() ? "null" : "undefined", "object");
        }
        return NULL;
    }

    return PrimitiveToObject(cx, val);
}

JSObject *
js_ValueToNonNullObject(JSContext *cx, const Value &v)
{
    RootedObject obj(cx);

    if (!js_ValueToObjectOrNull(cx, v, &obj))
        return NULL;
    if (!obj) {
        RootedValue val(cx, v);
        js_ReportIsNullOrUndefined(cx, JSDVG_SEARCH_STACK, val, NullPtr());
    }
    return obj;
}

void
js_GetObjectSlotName(JSTracer *trc, char *buf, size_t bufsize)
{
    JS_ASSERT(trc->debugPrinter == js_GetObjectSlotName);

    JSObject *obj = (JSObject *)trc->debugPrintArg;
    uint32_t slot = uint32_t(trc->debugPrintIndex);

    RawShape shape;
    if (obj->isNative()) {
        shape = obj->lastProperty();
        while (shape && (!shape->hasSlot() || shape->slot() != slot))
            shape = shape->previous();
    } else {
        shape = NULL;
    }

    if (!shape) {
        const char *slotname = NULL;
        if (obj->isGlobal()) {
#define TEST_SLOT_MATCHES_PROTOTYPE(name,code,init)                           \
            if ((code) == slot) { slotname = js_##name##_str; goto found; }
            JS_FOR_EACH_PROTOTYPE(TEST_SLOT_MATCHES_PROTOTYPE)
#undef TEST_SLOT_MATCHES_PROTOTYPE
        }
      found:
        if (slotname)
            JS_snprintf(buf, bufsize, "CLASS_OBJECT(%s)", slotname);
        else
            JS_snprintf(buf, bufsize, "**UNKNOWN SLOT %ld**", (long)slot);
    } else {
        jsid propid = shape->propid();
        if (JSID_IS_INT(propid)) {
            JS_snprintf(buf, bufsize, "%ld", (long)JSID_TO_INT(propid));
        } else if (JSID_IS_ATOM(propid)) {
            PutEscapedString(buf, bufsize, JSID_TO_ATOM(propid), 0);
        } else {
            JS_snprintf(buf, bufsize, "**FINALIZED ATOM KEY**");
        }
    }
}

JSBool
js_ReportGetterOnlyAssignment(JSContext *cx)
{
    return JS_ReportErrorFlagsAndNumber(cx,
                                        JSREPORT_WARNING | JSREPORT_STRICT |
                                        JSREPORT_STRICT_MODE_ERROR,
                                        js_GetErrorMessage, NULL,
                                        JSMSG_GETTER_ONLY);
}

JS_FRIEND_API(JSBool)
js_GetterOnlyPropertyStub(JSContext *cx, JSHandleObject obj, JSHandleId id, JSBool strict, JSMutableHandleValue vp)
{
    JS_ReportErrorNumber(cx, js_GetErrorMessage, NULL, JSMSG_GETTER_ONLY);
    return JS_FALSE;
}

#ifdef DEBUG







void
dumpValue(const Value &v)
{
    if (v.isNull())
        fprintf(stderr, "null");
    else if (v.isUndefined())
        fprintf(stderr, "undefined");
    else if (v.isInt32())
        fprintf(stderr, "%d", v.toInt32());
    else if (v.isDouble())
        fprintf(stderr, "%g", v.toDouble());
    else if (v.isString())
        v.toString()->dump();
    else if (v.isObject() && v.toObject().isFunction()) {
        JSFunction *fun = v.toObject().toFunction();
        if (fun->displayAtom()) {
            fputs("<function ", stderr);
            FileEscapedString(stderr, fun->displayAtom(), 0);
        } else {
            fputs("<unnamed function", stderr);
        }
        if (fun->hasScript()) {
            RawScript script = fun->nonLazyScript();
            fprintf(stderr, " (%s:%u)",
                    script->filename ? script->filename : "", script->lineno);
        }
        fprintf(stderr, " at %p>", (void *) fun);
    } else if (v.isObject()) {
        JSObject *obj = &v.toObject();
        Class *clasp = obj->getClass();
        fprintf(stderr, "<%s%s at %p>",
                clasp->name,
                (clasp == &ObjectClass) ? "" : " object",
                (void *) obj);
    } else if (v.isBoolean()) {
        if (v.toBoolean())
            fprintf(stderr, "true");
        else
            fprintf(stderr, "false");
    } else if (v.isMagic()) {
        fprintf(stderr, "<invalid");
#ifdef DEBUG
        switch (v.whyMagic()) {
          case JS_ELEMENTS_HOLE:     fprintf(stderr, " elements hole");      break;
          case JS_NATIVE_ENUMERATE:  fprintf(stderr, " native enumeration"); break;
          case JS_NO_ITER_VALUE:     fprintf(stderr, " no iter value");      break;
          case JS_GENERATOR_CLOSING: fprintf(stderr, " generator closing");  break;
          default:                   fprintf(stderr, " ?!");                 break;
        }
#endif
        fprintf(stderr, ">");
    } else {
        fprintf(stderr, "unexpected value");
    }
}

JS_FRIEND_API(void)
js_DumpValue(const Value &val)
{
    dumpValue(val);
    fputc('\n', stderr);
}

JS_FRIEND_API(void)
js_DumpId(jsid id)
{
    fprintf(stderr, "jsid %p = ", (void *) JSID_BITS(id));
    dumpValue(IdToValue(id));
    fputc('\n', stderr);
}

static void
DumpProperty(JSObject *obj, Shape &shape)
{
    jsid id = shape.propid();
    uint8_t attrs = shape.attributes();

    fprintf(stderr, "    ((JSShape *) %p) ", (void *) &shape);
    if (attrs & JSPROP_ENUMERATE) fprintf(stderr, "enumerate ");
    if (attrs & JSPROP_READONLY) fprintf(stderr, "readonly ");
    if (attrs & JSPROP_PERMANENT) fprintf(stderr, "permanent ");
    if (attrs & JSPROP_SHARED) fprintf(stderr, "shared ");

    if (shape.hasGetterValue())
        fprintf(stderr, "getterValue=%p ", (void *) shape.getterObject());
    else if (!shape.hasDefaultGetter())
        fprintf(stderr, "getterOp=%p ", JS_FUNC_TO_DATA_PTR(void *, shape.getterOp()));

    if (shape.hasSetterValue())
        fprintf(stderr, "setterValue=%p ", (void *) shape.setterObject());
    else if (!shape.hasDefaultSetter())
        fprintf(stderr, "setterOp=%p ", JS_FUNC_TO_DATA_PTR(void *, shape.setterOp()));

    if (JSID_IS_ATOM(id))
        JSID_TO_STRING(id)->dump();
    else if (JSID_IS_INT(id))
        fprintf(stderr, "%d", (int) JSID_TO_INT(id));
    else
        fprintf(stderr, "unknown jsid %p", (void *) JSID_BITS(id));

    uint32_t slot = shape.hasSlot() ? shape.maybeSlot() : SHAPE_INVALID_SLOT;
    fprintf(stderr, ": slot %d", slot);
    if (shape.hasSlot()) {
        fprintf(stderr, " = ");
        dumpValue(obj->getSlot(slot));
    } else if (slot != SHAPE_INVALID_SLOT) {
        fprintf(stderr, " (INVALID!)");
    }
    fprintf(stderr, "\n");
}

void
JSObject::dump()
{
    JSObject *obj = this;
    fprintf(stderr, "object %p\n", (void *) obj);
    Class *clasp = obj->getClass();
    fprintf(stderr, "class %p %s\n", (void *)clasp, clasp->name);

    fprintf(stderr, "flags:");
    if (obj->isDelegate()) fprintf(stderr, " delegate");
    if (!obj->isExtensible()) fprintf(stderr, " not_extensible");
    if (obj->isIndexed()) fprintf(stderr, " indexed");

    if (obj->isNative()) {
        if (obj->inDictionaryMode())
            fprintf(stderr, " inDictionaryMode");
        if (obj->hasShapeTable())
            fprintf(stderr, " hasShapeTable");
    }
    fprintf(stderr, "\n");

    if (obj->isNative()) {
        unsigned slots = obj->getDenseInitializedLength();
        if (slots) {
            fprintf(stderr, "elements\n");
            for (unsigned i = 0; i < slots; i++) {
                fprintf(stderr, " %3d: ", i);
                dumpValue(obj->getDenseElement(i));
                fprintf(stderr, "\n");
                fflush(stderr);
            }
        }
    }

    fprintf(stderr, "proto ");
    TaggedProto proto = obj->getTaggedProto();
    if (proto.isLazy())
        fprintf(stderr, "<lazy>");
    else
        dumpValue(ObjectOrNullValue(proto.toObjectOrNull()));
    fputc('\n', stderr);

    fprintf(stderr, "parent ");
    dumpValue(ObjectOrNullValue(obj->getParent()));
    fputc('\n', stderr);

    if (clasp->flags & JSCLASS_HAS_PRIVATE)
        fprintf(stderr, "private %p\n", obj->getPrivate());

    if (!obj->isNative())
        fprintf(stderr, "not native\n");

    unsigned reservedEnd = JSCLASS_RESERVED_SLOTS(clasp);
    unsigned slots = obj->slotSpan();
    unsigned stop = obj->isNative() ? reservedEnd : slots;
    if (stop > 0)
        fprintf(stderr, obj->isNative() ? "reserved slots:\n" : "slots:\n");
    for (unsigned i = 0; i < stop; i++) {
        fprintf(stderr, " %3d ", i);
        if (i < reservedEnd)
            fprintf(stderr, "(reserved) ");
        fprintf(stderr, "= ");
        dumpValue(obj->getSlot(i));
        fputc('\n', stderr);
    }

    if (obj->isNative()) {
        fprintf(stderr, "properties:\n");
        Vector<RawShape, 8, SystemAllocPolicy> props;
        for (Shape::Range<NoGC> r(obj->lastProperty()); !r.empty(); r.popFront())
            props.append(&r.front());
        for (size_t i = props.length(); i-- != 0;)
            DumpProperty(obj, *props[i]);
    }
    fputc('\n', stderr);
}

static void
MaybeDumpObject(const char *name, JSObject *obj)
{
    if (obj) {
        fprintf(stderr, "  %s: ", name);
        dumpValue(ObjectValue(*obj));
        fputc('\n', stderr);
    }
}

static void
MaybeDumpValue(const char *name, const Value &v)
{
    if (!v.isNull()) {
        fprintf(stderr, "  %s: ", name);
        dumpValue(v);
        fputc('\n', stderr);
    }
}

JS_FRIEND_API(void)
js_DumpStackFrame(JSContext *cx, StackFrame *start)
{
    
    ScriptFrameIter i(cx, StackIter::GO_THROUGH_SAVED);
    if (!start) {
        if (i.done()) {
            fprintf(stderr, "no stack for cx = %p\n", (void*) cx);
            return;
        }
    } else {
        while (!i.done() && !i.isIon() && i.interpFrame() != start)
            ++i;

        if (i.done()) {
            fprintf(stderr, "fp = %p not found in cx = %p\n",
                    (void *)start, (void *)cx);
            return;
        }
    }

    for (; !i.done(); ++i) {
        if (i.isIon())
            fprintf(stderr, "IonFrame\n");
        else
            fprintf(stderr, "StackFrame at %p\n", (void *) i.interpFrame());

        if (i.isFunctionFrame()) {
            fprintf(stderr, "callee fun: ");
            dumpValue(i.calleev());
        } else {
            fprintf(stderr, "global frame, no callee");
        }
        fputc('\n', stderr);

        fprintf(stderr, "file %s line %u\n",
                i.script()->filename, (unsigned) i.script()->lineno);

        if (jsbytecode *pc = i.pc()) {
            fprintf(stderr, "  pc = %p\n", pc);
            fprintf(stderr, "  current op: %s\n", js_CodeName[*pc]);
        }
        if (!i.isIon())
            MaybeDumpObject("blockChain", i.interpFrame()->maybeBlockChain());
        MaybeDumpValue("this", i.thisv());
        if (!i.isIon()) {
            fprintf(stderr, "  rval: ");
            dumpValue(i.interpFrame()->returnValue());
            fputc('\n', stderr);
        }

        fprintf(stderr, "  flags:");
        if (i.isConstructing())
            fprintf(stderr, " constructing");
        if (!i.isIon() && i.interpFrame()->isDebuggerFrame())
            fprintf(stderr, " debugger");
        if (i.isEvalFrame())
            fprintf(stderr, " eval");
        if (!i.isIon() && i.interpFrame()->isYielding())
            fprintf(stderr, " yielding");
        if (!i.isIon() && i.interpFrame()->isGeneratorFrame())
            fprintf(stderr, " generator");
        fputc('\n', stderr);

        fprintf(stderr, "  scopeChain: (JSObject *) %p\n", (void *) i.scopeChain());

        fputc('\n', stderr);
    }
}

#endif 

JS_FRIEND_API(void)
js_DumpBacktrace(JSContext *cx)
{
    Sprinter sprinter(cx);
    sprinter.init();
    size_t depth = 0;
    for (StackIter i(cx); !i.done(); ++i, ++depth) {
        if (i.isScript()) {
            const char *filename = JS_GetScriptFilename(cx, i.script());
            unsigned line = JS_PCToLineNumber(cx, i.script(), i.pc());
            RawScript script = i.script();
            sprinter.printf("#%d %14p   %s:%d (%p @ %d)\n",
                            depth, (i.isIon() ? 0 : i.interpFrame()), filename, line,
                            script, i.pc() - script->code);
        } else {
            sprinter.printf("#%d ???\n", depth);
        }
    }
    fprintf(stdout, "%s", sprinter.string());
}

