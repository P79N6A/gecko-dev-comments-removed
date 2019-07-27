





#ifndef vm_Shape_inl_h
#define vm_Shape_inl_h

#include "vm/Shape.h"

#include "mozilla/TypeTraits.h"

#include "jsobj.h"

#include "gc/Allocator.h"
#include "vm/Interpreter.h"
#include "vm/ScopeObject.h"
#include "vm/TypedArrayCommon.h"

#include "jsatominlines.h"
#include "jscntxtinlines.h"

namespace js {

inline
StackBaseShape::StackBaseShape(ExclusiveContext *cx, const Class *clasp,
                               JSObject *metadata, uint32_t objectFlags)
  : flags(objectFlags),
    clasp(clasp),
    metadata(metadata),
    compartment(cx->compartment_)
{}

inline Shape *
Shape::search(ExclusiveContext *cx, jsid id)
{
    ShapeTable::Entry *_;
    return search(cx, this, id, &_);
}

inline bool
Shape::set(JSContext* cx, HandleNativeObject obj, HandleObject receiver, MutableHandleValue vp,
           ObjectOpResult &result)
{
    MOZ_ASSERT_IF(hasDefaultSetter(), hasGetterValue());
    MOZ_ASSERT(!obj->is<DynamicWithObject>());  

    if (attrs & JSPROP_SETTER) {
        Value fval = setterValue();
        if (!InvokeGetterOrSetter(cx, receiver, fval, 1, vp.address(), vp))
            return false;
        return result.succeed();
    }

    if (attrs & JSPROP_GETTER)
        return result.fail(JSMSG_GETTER_ONLY);

    if (!setterOp())
        return result.succeed();

    RootedId id(cx, propid());
    return CallJSSetterOp(cx, setterOp(), obj, id, vp, result);
}

 inline Shape *
Shape::search(ExclusiveContext *cx, Shape *start, jsid id, ShapeTable::Entry **pentry, bool adding)
{
    if (start->inDictionary()) {
        *pentry = &start->table().search(id, adding);
        return (*pentry)->shape();
    }

    *pentry = nullptr;

    if (start->hasTable()) {
        ShapeTable::Entry &entry = start->table().search(id, adding);
        return entry.shape();
    }

    if (start->numLinearSearches() == LINEAR_SEARCHES_MAX) {
        if (start->isBigEnoughForAShapeTable()) {
            if (Shape::hashify(cx, start)) {
                ShapeTable::Entry &entry = start->table().search(id, adding);
                return entry.shape();
            } else {
                cx->recoverFromOutOfMemory();
            }
        }
        



        MOZ_ASSERT(!start->hasTable());
    } else {
        start->incrementNumLinearSearches();
    }

    for (Shape *shape = start; shape; shape = shape->parent) {
        if (shape->propidRef() == id)
            return shape;
    }

    return nullptr;
}

inline Shape *
Shape::new_(ExclusiveContext *cx, StackShape &unrootedOther, uint32_t nfixed)
{
    RootedGeneric<StackShape*> other(cx, &unrootedOther);
    Shape *shape = other->isAccessorShape()
                   ? js::Allocate<AccessorShape>(cx)
                   : js::Allocate<Shape>(cx);
    if (!shape) {
        ReportOutOfMemory(cx);
        return nullptr;
    }

    if (other->isAccessorShape())
        new (shape) AccessorShape(*other, nfixed);
    else
        new (shape) Shape(*other, nfixed);

    return shape;
}

template<class ObjectSubclass>
 inline bool
EmptyShape::ensureInitialCustomShape(ExclusiveContext *cx, Handle<ObjectSubclass*> obj)
{
    static_assert(mozilla::IsBaseOf<JSObject, ObjectSubclass>::value,
                  "ObjectSubclass must be a subclass of JSObject");

    
    
    if (!obj->empty())
        return true;

    
    RootedShape shape(cx, ObjectSubclass::assignInitialShape(cx, obj));
    if (!shape)
        return false;
    MOZ_ASSERT(!obj->empty());

    
    
    
    
    
    if (obj->isDelegate())
        return true;

    
    
    RootedObject proto(cx, obj->getProto());
    EmptyShape::insertInitialShape(cx, shape, proto);
    return true;
}

inline
AutoRooterGetterSetter::Inner::Inner(ExclusiveContext *cx, uint8_t attrs,
                                     GetterOp *pgetter_, SetterOp *psetter_)
  : CustomAutoRooter(cx), attrs(attrs),
    pgetter(pgetter_), psetter(psetter_)
{}

inline
AutoRooterGetterSetter::AutoRooterGetterSetter(ExclusiveContext *cx, uint8_t attrs,
                                               GetterOp *pgetter, SetterOp *psetter
                                               MOZ_GUARD_OBJECT_NOTIFIER_PARAM_IN_IMPL)
{
    if (attrs & (JSPROP_GETTER | JSPROP_SETTER))
        inner.emplace(cx, attrs, pgetter, psetter);
    MOZ_GUARD_OBJECT_NOTIFIER_INIT;
}

inline
AutoRooterGetterSetter::AutoRooterGetterSetter(ExclusiveContext *cx, uint8_t attrs,
                                               JSNative *pgetter, JSNative *psetter
                                               MOZ_GUARD_OBJECT_NOTIFIER_PARAM_IN_IMPL)
{
    if (attrs & (JSPROP_GETTER | JSPROP_SETTER)) {
        inner.emplace(cx, attrs, reinterpret_cast<GetterOp *>(pgetter),
                      reinterpret_cast<SetterOp *>(psetter));
    }
    MOZ_GUARD_OBJECT_NOTIFIER_INIT;
}

static inline uint8_t
GetShapeAttributes(JSObject *obj, Shape *shape)
{
    MOZ_ASSERT(obj->isNative());

    if (IsImplicitDenseOrTypedArrayElement(shape)) {
        if (IsAnyTypedArray(obj))
            return JSPROP_ENUMERATE | JSPROP_PERMANENT;
        return JSPROP_ENUMERATE;
    }

    return shape->attributes();
}

} 

#endif 
