





#ifndef vm_Shape_inl_h
#define vm_Shape_inl_h

#include "vm/Shape.h"

#include "mozilla/TypeTraits.h"

#include "jsobj.h"

#include "vm/Interpreter.h"
#include "vm/ScopeObject.h"

#include "jsatominlines.h"
#include "jscntxtinlines.h"
#include "jsgcinlines.h"

namespace js {

inline
StackBaseShape::StackBaseShape(ThreadSafeContext *cx, const Class *clasp,
                               JSObject *parent, JSObject *metadata, uint32_t objectFlags)
  : flags(objectFlags),
    clasp(clasp),
    parent(parent),
    metadata(metadata),
    rawGetter(nullptr),
    rawSetter(nullptr),
    compartment(cx->compartment_)
{}

inline bool
Shape::getUserId(JSContext *cx, MutableHandleId idp) const
{
    const Shape *self = this;
#ifdef DEBUG
    {
        SkipRoot skip(cx, &self);
        MaybeCheckStackRoots(cx);
    }
#endif
    if (self->hasShortID()) {
        int16_t id = self->shortid();
        if (id < 0) {
            RootedValue v(cx, Int32Value(id));
            return ValueToId<CanGC>(cx, v, idp);
        }
        idp.set(INT_TO_JSID(id));
    } else {
        idp.set(self->propid());
    }
    return true;
}

inline bool
Shape::get(JSContext* cx, HandleObject receiver, JSObject* obj, JSObject *pobj,
           MutableHandleValue vp)
{
    JS_ASSERT(!hasDefaultGetter());

    if (hasGetterValue()) {
        Value fval = getterValue();
        return InvokeGetterOrSetter(cx, receiver, fval, 0, 0, vp);
    }

    Rooted<Shape *> self(cx, this);
    RootedId id(cx);
    if (!self->getUserId(cx, &id))
        return false;

    return CallJSPropertyOp(cx, self->getterOp(), receiver, id, vp);
}

inline Shape *
Shape::search(ExclusiveContext *cx, jsid id)
{
    Shape **_;
    return search(cx, this, id, &_);
}

inline Shape *
Shape::searchThreadLocal(ThreadSafeContext *cx, Shape *start, jsid id,
                         Shape ***pspp, bool adding)
{
    










    JS_ASSERT_IF(adding, cx->isThreadLocal(start) && start->inDictionary());

    if (start->inDictionary()) {
        *pspp = start->table().search(id, adding);
        return SHAPE_FETCH(*pspp);
    }

    *pspp = nullptr;

    return searchNoHashify(start, id);
}

inline bool
Shape::set(JSContext* cx, HandleObject obj, HandleObject receiver, bool strict,
           MutableHandleValue vp)
{
    JS_ASSERT_IF(hasDefaultSetter(), hasGetterValue());

    if (attrs & JSPROP_SETTER) {
        Value fval = setterValue();
        return InvokeGetterOrSetter(cx, receiver, fval, 1, vp.address(), vp);
    }

    if (attrs & JSPROP_GETTER)
        return js_ReportGetterOnlyAssignment(cx, strict);

    Rooted<Shape *> self(cx, this);
    RootedId id(cx);
    if (!self->getUserId(cx, &id))
        return false;

    



    if (obj->is<DynamicWithObject>()) {
        RootedObject nobj(cx, &obj->as<DynamicWithObject>().object());
        return CallJSPropertyOpSetter(cx, self->setterOp(), nobj, id, strict, vp);
    }

    return CallJSPropertyOpSetter(cx, self->setterOp(), obj, id, strict, vp);
}

 inline Shape *
Shape::search(ExclusiveContext *cx, Shape *start, jsid id, Shape ***pspp, bool adding)
{
    if (start->inDictionary()) {
        *pspp = start->table().search(id, adding);
        return SHAPE_FETCH(*pspp);
    }

    *pspp = nullptr;

    if (start->hasTable()) {
        Shape **spp = start->table().search(id, adding);
        return SHAPE_FETCH(spp);
    }

    if (start->numLinearSearches() == LINEAR_SEARCHES_MAX) {
        if (start->isBigEnoughForAShapeTable()) {
            if (Shape::hashify(cx, start)) {
                Shape **spp = start->table().search(id, adding);
                return SHAPE_FETCH(spp);
            }
        }
        



        JS_ASSERT(!start->hasTable());
    } else {
        start->incrementNumLinearSearches();
    }

    for (Shape *shape = start; shape; shape = shape->parent) {
        if (shape->propidRef() == id)
            return shape;
    }

    return nullptr;
}

template<class ObjectSubclass>
 inline bool
EmptyShape::ensureInitialCustomShape(ExclusiveContext *cx, Handle<ObjectSubclass*> obj)
{
    static_assert(mozilla::IsBaseOf<JSObject, ObjectSubclass>::value,
                  "ObjectSubclass must be a subclass of JSObject");

    
    
    if (!obj->nativeEmpty())
        return true;

    
    RootedShape shape(cx, ObjectSubclass::assignInitialShape(cx, obj));
    if (!shape)
        return false;
    MOZ_ASSERT(!obj->nativeEmpty());

    
    
    
    
    
    if (obj->isDelegate())
        return true;

    
    
    RootedObject proto(cx, obj->getProto());
    EmptyShape::insertInitialShape(cx, shape, proto);
    return true;
}

inline
AutoRooterGetterSetter::Inner::Inner(ThreadSafeContext *cx, uint8_t attrs,
                                     PropertyOp *pgetter_, StrictPropertyOp *psetter_)
  : CustomAutoRooter(cx), attrs(attrs),
    pgetter(pgetter_), psetter(psetter_),
    getterRoot(cx, pgetter_), setterRoot(cx, psetter_)
{
    JS_ASSERT_IF(attrs & JSPROP_GETTER, !IsPoisonedPtr(*pgetter));
    JS_ASSERT_IF(attrs & JSPROP_SETTER, !IsPoisonedPtr(*psetter));
}

inline
AutoRooterGetterSetter::AutoRooterGetterSetter(ThreadSafeContext *cx, uint8_t attrs,
                                               PropertyOp *pgetter, StrictPropertyOp *psetter
                                               MOZ_GUARD_OBJECT_NOTIFIER_PARAM_IN_IMPL)
{
    if (attrs & (JSPROP_GETTER | JSPROP_SETTER))
        inner.construct(cx, attrs, pgetter, psetter);
    MOZ_GUARD_OBJECT_NOTIFIER_INIT;
}

} 

#endif 
