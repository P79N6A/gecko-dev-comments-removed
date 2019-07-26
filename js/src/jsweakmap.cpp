





#include "jsweakmap.h"

#include <string.h>

#include "jsapi.h"
#include "jscntxt.h"
#include "jsfriendapi.h"
#include "jsobj.h"
#include "jswrapper.h"

#include "vm/GlobalObject.h"
#include "vm/WeakMapObject.h"

#include "jsobjinlines.h"

#include "gc/Barrier-inl.h"

using namespace js;

WeakMapBase::WeakMapBase(JSObject *memOf, JSCompartment *c)
  : memberOf(memOf),
    compartment(c),
    next(WeakMapNotInList)
{
    JS_ASSERT_IF(memberOf, memberOf->compartment() == c);
}

WeakMapBase::~WeakMapBase()
{
    JS_ASSERT(next == WeakMapNotInList);
}

bool
WeakMapBase::markCompartmentIteratively(JSCompartment *c, JSTracer *tracer)
{
    bool markedAny = false;
    for (WeakMapBase *m = c->gcWeakMapList; m; m = m->next) {
        if (m->markIteratively(tracer))
            markedAny = true;
    }
    return markedAny;
}

void
WeakMapBase::sweepCompartment(JSCompartment *c)
{
    for (WeakMapBase *m = c->gcWeakMapList; m; m = m->next)
        m->sweep();
}

void
WeakMapBase::traceAllMappings(WeakMapTracer *tracer)
{
    JSRuntime *rt = tracer->runtime;
    for (CompartmentsIter c(rt); !c.done(); c.next()) {
        for (WeakMapBase *m = c->gcWeakMapList; m; m = m->next)
            m->traceMappings(tracer);
    }
}

void
WeakMapBase::resetCompartmentWeakMapList(JSCompartment *c)
{
    JS_ASSERT(WeakMapNotInList != NULL);

    WeakMapBase *m = c->gcWeakMapList;
    c->gcWeakMapList = NULL;
    while (m) {
        WeakMapBase *n = m->next;
        m->next = WeakMapNotInList;
        m = n;
    }
}

bool
WeakMapBase::saveCompartmentWeakMapList(JSCompartment *c, WeakMapVector &vector)
{
    WeakMapBase *m = c->gcWeakMapList;
    while (m) {
        if (!vector.append(m))
            return false;
        m = m->next;
    }
    return true;
}

void
WeakMapBase::restoreCompartmentWeakMapLists(WeakMapVector &vector)
{
    for (WeakMapBase **p = vector.begin(); p != vector.end(); p++) {
        WeakMapBase *m = *p;
        JS_ASSERT(m->next == WeakMapNotInList);
        JSCompartment *c = m->compartment;
        m->next = c->gcWeakMapList;
        c->gcWeakMapList = m;
    }
}

void
WeakMapBase::removeWeakMapFromList(WeakMapBase *weakmap)
{
    JSCompartment *c = weakmap->compartment;
    for (WeakMapBase **p = &c->gcWeakMapList; *p; p = &(*p)->next) {
        if (*p == weakmap) {
            *p = (*p)->next;
            weakmap->next = WeakMapNotInList;
            break;
        }
    }
}

static JSObject *
GetKeyArg(JSContext *cx, CallArgs &args)
{
    if (args[0].isPrimitive()) {
        JS_ReportErrorNumber(cx, js_GetErrorMessage, NULL, JSMSG_NOT_NONNULL_OBJECT);
        return NULL;
    }
    return &args[0].toObject();
}

JS_ALWAYS_INLINE bool
IsWeakMap(HandleValue v)
{
    return v.isObject() && v.toObject().is<WeakMapObject>();
}

JS_ALWAYS_INLINE bool
WeakMap_has_impl(JSContext *cx, CallArgs args)
{
    JS_ASSERT(IsWeakMap(args.thisv()));

    if (args.length() < 1) {
        JS_ReportErrorNumber(cx, js_GetErrorMessage, NULL, JSMSG_MORE_ARGS_NEEDED,
                             "WeakMap.has", "0", "s");
        return false;
    }
    JSObject *key = GetKeyArg(cx, args);
    if (!key)
        return false;

    if (ObjectValueMap *map = args.thisv().toObject().as<WeakMapObject>().getMap()) {
        if (map->has(key)) {
            args.rval().setBoolean(true);
            return true;
        }
    }

    args.rval().setBoolean(false);
    return true;
}

static bool
WeakMap_has(JSContext *cx, unsigned argc, Value *vp)
{
    CallArgs args = CallArgsFromVp(argc, vp);
    return CallNonGenericMethod<IsWeakMap, WeakMap_has_impl>(cx, args);
}

JS_ALWAYS_INLINE bool
WeakMap_clear_impl(JSContext *cx, CallArgs args)
{
    JS_ASSERT(IsWeakMap(args.thisv()));

    
    
    if (ObjectValueMap *map = args.thisv().toObject().as<WeakMapObject>().getMap())
        map->clear();

    args.rval().setUndefined();
    return true;
}

static bool
WeakMap_clear(JSContext *cx, unsigned argc, Value *vp)
{
    CallArgs args = CallArgsFromVp(argc, vp);
    return CallNonGenericMethod<IsWeakMap, WeakMap_clear_impl>(cx, args);
}

JS_ALWAYS_INLINE bool
WeakMap_get_impl(JSContext *cx, CallArgs args)
{
    JS_ASSERT(IsWeakMap(args.thisv()));

    if (args.length() < 1) {
        JS_ReportErrorNumber(cx, js_GetErrorMessage, NULL, JSMSG_MORE_ARGS_NEEDED,
                             "WeakMap.get", "0", "s");
        return false;
    }
    JSObject *key = GetKeyArg(cx, args);
    if (!key)
        return false;

    if (ObjectValueMap *map = args.thisv().toObject().as<WeakMapObject>().getMap()) {
        if (ObjectValueMap::Ptr ptr = map->lookup(key)) {
            
            
            ExposeValueToActiveJS(ptr->value.get());

            args.rval().set(ptr->value);
            return true;
        }
    }

    args.rval().set((args.length() > 1) ? args[1] : UndefinedValue());
    return true;
}

static bool
WeakMap_get(JSContext *cx, unsigned argc, Value *vp)
{
    CallArgs args = CallArgsFromVp(argc, vp);
    return CallNonGenericMethod<IsWeakMap, WeakMap_get_impl>(cx, args);
}

JS_ALWAYS_INLINE bool
WeakMap_delete_impl(JSContext *cx, CallArgs args)
{
    JS_ASSERT(IsWeakMap(args.thisv()));

    if (args.length() < 1) {
        JS_ReportErrorNumber(cx, js_GetErrorMessage, NULL, JSMSG_MORE_ARGS_NEEDED,
                             "WeakMap.delete", "0", "s");
        return false;
    }
    JSObject *key = GetKeyArg(cx, args);
    if (!key)
        return false;

    if (ObjectValueMap *map = args.thisv().toObject().as<WeakMapObject>().getMap()) {
        if (ObjectValueMap::Ptr ptr = map->lookup(key)) {
            map->remove(ptr);
            args.rval().setBoolean(true);
            return true;
        }
    }

    args.rval().setBoolean(false);
    return true;
}

static bool
WeakMap_delete(JSContext *cx, unsigned argc, Value *vp)
{
    CallArgs args = CallArgsFromVp(argc, vp);
    return CallNonGenericMethod<IsWeakMap, WeakMap_delete_impl>(cx, args);
}

static bool
TryPreserveReflector(JSContext *cx, HandleObject obj)
{
    if (obj->getClass()->ext.isWrappedNative ||
        (obj->getClass()->flags & JSCLASS_IS_DOMJSCLASS) ||
        (obj->is<ProxyObject>() &&
         obj->as<ProxyObject>().handler()->family() == GetDOMProxyHandlerFamily()))
    {
        JS_ASSERT(cx->runtime()->preserveWrapperCallback);
        if (!cx->runtime()->preserveWrapperCallback(cx, obj)) {
            JS_ReportErrorNumber(cx, js_GetErrorMessage, NULL, JSMSG_BAD_WEAKMAP_KEY);
            return false;
        }
    }
    return true;
}

JS_ALWAYS_INLINE bool
WeakMap_set_impl(JSContext *cx, CallArgs args)
{
    JS_ASSERT(IsWeakMap(args.thisv()));

    if (args.length() < 1) {
        JS_ReportErrorNumber(cx, js_GetErrorMessage, NULL, JSMSG_MORE_ARGS_NEEDED,
                             "WeakMap.set", "0", "s");
        return false;
    }
    RootedObject key(cx, GetKeyArg(cx, args));
    if (!key)
        return false;

    RootedValue value(cx, (args.length() > 1) ? args[1] : UndefinedValue());

    Rooted<JSObject*> thisObj(cx, &args.thisv().toObject());
    ObjectValueMap *map = thisObj->as<WeakMapObject>().getMap();
    if (!map) {
        map = cx->new_<ObjectValueMap>(cx, thisObj.get());
        if (!map->init()) {
            js_delete(map);
            JS_ReportOutOfMemory(cx);
            return false;
        }
        thisObj->setPrivate(map);
    }

    
    if (!TryPreserveReflector(cx, key))
        return false;

    if (JSWeakmapKeyDelegateOp op = key->getClass()->ext.weakmapKeyDelegateOp) {
        RootedObject delegate(cx, op(key));
        if (delegate && !TryPreserveReflector(cx, delegate))
            return false;
    }

    JS_ASSERT(key->compartment() == thisObj->compartment());
    JS_ASSERT_IF(value.isObject(), value.toObject().compartment() == thisObj->compartment());
    if (!map->put(key, value)) {
        JS_ReportOutOfMemory(cx);
        return false;
    }
    HashTableWriteBarrierPost(cx->runtime(), map, key.get());

    args.rval().setUndefined();
    return true;
}

static bool
WeakMap_set(JSContext *cx, unsigned argc, Value *vp)
{
    CallArgs args = CallArgsFromVp(argc, vp);
    return CallNonGenericMethod<IsWeakMap, WeakMap_set_impl>(cx, args);
}

JS_FRIEND_API(bool)
JS_NondeterministicGetWeakMapKeys(JSContext *cx, JSObject *objArg, JSObject **ret)
{
    RootedObject obj(cx, objArg);
    obj = UncheckedUnwrap(obj);
    if (!obj || !obj->is<WeakMapObject>()) {
        *ret = NULL;
        return true;
    }
    RootedObject arr(cx, NewDenseEmptyArray(cx));
    if (!arr)
        return false;
    ObjectValueMap *map = obj->as<WeakMapObject>().getMap();
    if (map) {
        
        gc::AutoSuppressGC suppress(cx);
        for (ObjectValueMap::Base::Range r = map->all(); !r.empty(); r.popFront()) {
            RootedObject key(cx, r.front().key);
            if (!JS_WrapObject(cx, key.address()))
                return false;
            if (!js_NewbornArrayPush(cx, arr, ObjectValue(*key)))
                return false;
        }
    }
    *ret = arr;
    return true;
}

static void
WeakMap_mark(JSTracer *trc, JSObject *obj)
{
    if (ObjectValueMap *map = obj->as<WeakMapObject>().getMap())
        map->trace(trc);
}

static void
WeakMap_finalize(FreeOp *fop, JSObject *obj)
{
    if (ObjectValueMap *map = obj->as<WeakMapObject>().getMap()) {
        map->check();
#ifdef DEBUG
        map->~ObjectValueMap();
        memset(static_cast<void *>(map), 0xdc, sizeof(*map));
        fop->free_(map);
#else
        fop->delete_(map);
#endif
    }
}

static bool
WeakMap_construct(JSContext *cx, unsigned argc, Value *vp)
{
    JSObject *obj = NewBuiltinClassInstance(cx, &WeakMapObject::class_);
    if (!obj)
        return false;

    vp->setObject(*obj);
    return true;
}

const Class WeakMapObject::class_ = {
    "WeakMap",
    JSCLASS_HAS_PRIVATE | JSCLASS_IMPLEMENTS_BARRIERS |
    JSCLASS_HAS_CACHED_PROTO(JSProto_WeakMap),
    JS_PropertyStub,         
    JS_DeletePropertyStub,   
    JS_PropertyStub,         
    JS_StrictPropertyStub,   
    JS_EnumerateStub,
    JS_ResolveStub,
    JS_ConvertStub,
    WeakMap_finalize,
    NULL,                    
    NULL,                    
    NULL,                    
    NULL,                    
    WeakMap_mark
};

static const JSFunctionSpec weak_map_methods[] = {
    JS_FN("has",    WeakMap_has, 1, 0),
    JS_FN("get",    WeakMap_get, 2, 0),
    JS_FN("delete", WeakMap_delete, 1, 0),
    JS_FN("set",    WeakMap_set, 2, 0),
    JS_FN("clear",  WeakMap_clear, 0, 0),
    JS_FS_END
};

JSObject *
js_InitWeakMapClass(JSContext *cx, HandleObject obj)
{
    JS_ASSERT(obj->isNative());

    Rooted<GlobalObject*> global(cx, &obj->as<GlobalObject>());

    RootedObject weakMapProto(cx, global->createBlankPrototype(cx, &WeakMapObject::class_));
    if (!weakMapProto)
        return NULL;

    RootedFunction ctor(cx, global->createConstructor(cx, WeakMap_construct,
                                                      cx->names().WeakMap, 0));
    if (!ctor)
        return NULL;

    if (!LinkConstructorAndPrototype(cx, ctor, weakMapProto))
        return NULL;

    if (!DefinePropertiesAndBrand(cx, weakMapProto, NULL, weak_map_methods))
        return NULL;

    if (!DefineConstructorAndPrototype(cx, global, JSProto_WeakMap, ctor, weakMapProto))
        return NULL;
    return weakMapProto;
}
