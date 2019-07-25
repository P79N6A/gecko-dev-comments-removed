






































#ifndef jsscopeinlines_h___
#define jsscopeinlines_h___

#include <new>

#include "jsarray.h"
#include "jsbool.h"
#include "jscntxt.h"
#include "jsdbgapi.h"
#include "jsfun.h"
#include "jsobj.h"
#include "jsscope.h"
#include "jsgc.h"
#include "jsgcmark.h"

#include "vm/ArgumentsObject.h"
#include "vm/ScopeObject.h"
#include "vm/StringObject.h"

#include "jscntxtinlines.h"
#include "jsgcinlines.h"
#include "jsobjinlines.h"

#include "vm/ScopeObject-inl.h"

namespace js {

inline
BaseShape::BaseShape(Class *clasp, JSObject *parent, uint32_t objectFlags)
{
    JS_ASSERT(!(objectFlags & ~OBJECT_FLAG_MASK));
    PodZero(this);
    this->clasp = clasp;
    this->parent = parent;
    this->flags = objectFlags;
}

inline
BaseShape::BaseShape(Class *clasp, JSObject *parent, uint32_t objectFlags,
                     uint8_t attrs, js::PropertyOp rawGetter, js::StrictPropertyOp rawSetter)
{
    JS_ASSERT(!(objectFlags & ~OBJECT_FLAG_MASK));
    PodZero(this);
    this->clasp = clasp;
    this->parent = parent;
    this->flags = objectFlags;
    this->rawGetter = rawGetter;
    this->rawSetter = rawSetter;
    if ((attrs & JSPROP_GETTER) && rawGetter) {
        flags |= HAS_GETTER_OBJECT;
        JSObject::writeBarrierPost(this->getterObj, &this->getterObj);
    }
    if ((attrs & JSPROP_SETTER) && rawSetter) {
        flags |= HAS_SETTER_OBJECT;
        JSObject::writeBarrierPost(this->setterObj, &this->setterObj);
    }
}

inline
BaseShape::BaseShape(const StackBaseShape &base)
{
    PodZero(this);
    this->clasp = base.clasp;
    this->parent = base.parent;
    this->flags = base.flags;
    this->rawGetter = base.rawGetter;
    this->rawSetter = base.rawSetter;
}

inline bool
BaseShape::matchesGetterSetter(PropertyOp rawGetter, StrictPropertyOp rawSetter) const
{
    return rawGetter == this->rawGetter && rawSetter == this->rawSetter;
}

inline
StackBaseShape::StackBaseShape(Shape *shape)
  : flags(shape->getObjectFlags()),
    clasp(shape->getObjectClass()),
    parent(shape->getObjectParent())
{
    updateGetterSetter(shape->attrs, shape->getter(), shape->setter());
}

inline void
StackBaseShape::updateGetterSetter(uint8_t attrs,
                                   PropertyOp rawGetter,
                                   StrictPropertyOp rawSetter)
{
    flags &= ~(BaseShape::HAS_GETTER_OBJECT | BaseShape::HAS_SETTER_OBJECT);
    if ((attrs & JSPROP_GETTER) && rawGetter)
        flags |= BaseShape::HAS_GETTER_OBJECT;
    if ((attrs & JSPROP_SETTER) && rawSetter)
        flags |= BaseShape::HAS_SETTER_OBJECT;

    this->rawGetter = rawGetter;
    this->rawSetter = rawSetter;
}

inline void
BaseShape::adoptUnowned(UnownedBaseShape *other)
{
    



    JS_ASSERT(isOwned());
    DebugOnly<uint32_t> flags = getObjectFlags();
    JS_ASSERT((flags & other->getObjectFlags()) == flags);

    uint32_t span = slotSpan();
    PropertyTable *table = &this->table();

    *this = *other;
    setOwned(other);
    setTable(table);
    setSlotSpan(span);

    assertConsistency();
}

inline void
BaseShape::setOwned(UnownedBaseShape *unowned)
{
    flags |= OWNED_SHAPE;
    this->unowned_ = unowned;
}

inline void
BaseShape::assertConsistency()
{
#ifdef DEBUG
    if (isOwned()) {
        UnownedBaseShape *unowned = baseUnowned();
        JS_ASSERT(hasGetterObject() == unowned->hasGetterObject());
        JS_ASSERT(hasSetterObject() == unowned->hasSetterObject());
        JS_ASSERT_IF(hasGetterObject(), getterObject() == unowned->getterObject());
        JS_ASSERT_IF(hasSetterObject(), setterObject() == unowned->setterObject());
        JS_ASSERT(getObjectParent() == unowned->getObjectParent());
        JS_ASSERT(getObjectFlags() == unowned->getObjectFlags());
    }
#endif
}

inline
Shape::Shape(const StackShape &other, uint32_t nfixed)
  : base_(other.base),
    propid_(other.propid),
    slotInfo(other.maybeSlot() | (nfixed << FIXED_SLOTS_SHIFT)),
    attrs(other.attrs),
    flags(other.flags),
    shortid_(other.shortid),
    parent(NULL)
{
    kids.setNull();
}

inline
Shape::Shape(UnownedBaseShape *base, uint32_t nfixed)
  : base_(base),
    propid_(JSID_EMPTY),
    slotInfo(SHAPE_INVALID_SLOT | (nfixed << FIXED_SLOTS_SHIFT)),
    attrs(JSPROP_SHARED),
    flags(0),
    shortid_(0),
    parent(NULL)
{
    JS_ASSERT(base);
    kids.setNull();
}

inline JSDHashNumber
StackShape::hash() const
{
    JSDHashNumber hash = jsuword(base);

    
    hash = JS_ROTATE_LEFT32(hash, 4) ^ (flags & Shape::PUBLIC_FLAGS);
    hash = JS_ROTATE_LEFT32(hash, 4) ^ attrs;
    hash = JS_ROTATE_LEFT32(hash, 4) ^ shortid;
    hash = JS_ROTATE_LEFT32(hash, 4) ^ slot_;
    hash = JS_ROTATE_LEFT32(hash, 4) ^ JSID_BITS(propid);
    return hash;
}

inline bool
Shape::matches(const js::Shape *other) const
{
    return propid_.get() == other->propid_.get() &&
           matchesParamsAfterId(other->base(), other->maybeSlot(), other->attrs,
                                other->flags, other->shortid_);
}

inline bool
Shape::matches(const StackShape &other) const
{
    return propid_.get() == other.propid &&
           matchesParamsAfterId(other.base, other.slot_, other.attrs, other.flags, other.shortid);
}

inline bool
Shape::matchesParamsAfterId(BaseShape *base, uint32_t aslot,
                            uintN aattrs, uintN aflags, intN ashortid) const
{
    return base->unowned() == this->base()->unowned() &&
           maybeSlot() == aslot &&
           attrs == aattrs &&
           ((flags ^ aflags) & PUBLIC_FLAGS) == 0 &&
           shortid_ == ashortid;
}

inline bool
Shape::get(JSContext* cx, JSObject *receiver, JSObject* obj, JSObject *pobj, js::Value* vp) const
{
    JS_ASSERT(!hasDefaultGetter());

    if (hasGetterValue()) {
        JS_ASSERT(!isMethod());
        js::Value fval = getterValue();
        return js::InvokeGetterOrSetter(cx, receiver, fval, 0, 0, vp);
    }

    if (isMethod()) {
        vp->setObject(*pobj->nativeGetMethod(this));
        return pobj->methodReadBarrier(cx, *this, vp);
    }

    



    if (obj->isWith())
        obj = &obj->asWith().object();
    return js::CallJSPropertyOp(cx, getterOp(), receiver, getUserId(), vp);
}

inline bool
Shape::set(JSContext* cx, JSObject* obj, bool strict, js::Value* vp) const
{
    JS_ASSERT_IF(hasDefaultSetter(), hasGetterValue());

    if (attrs & JSPROP_SETTER) {
        js::Value fval = setterValue();
        return js::InvokeGetterOrSetter(cx, obj, fval, 1, vp, vp);
    }

    if (attrs & JSPROP_GETTER)
        return js_ReportGetterOnlyAssignment(cx);

    
    if (obj->isWith())
        obj = &obj->asWith().object();
    return js::CallJSPropertyOpSetter(cx, setterOp(), obj, getUserId(), strict, vp);
}

inline void
Shape::setParent(js::Shape *p)
{
    JS_ASSERT_IF(p && !p->hasMissingSlot() && !inDictionary(),
                 p->maybeSlot() <= maybeSlot());
    JS_ASSERT_IF(p && !inDictionary(),
                 hasSlot() == (p->maybeSlot() != maybeSlot()));
    parent = p;
}

inline void
Shape::removeFromDictionary(JSObject *obj)
{
    JS_ASSERT(inDictionary());
    JS_ASSERT(obj->inDictionaryMode());
    JS_ASSERT(listp);

    JS_ASSERT(obj->shape_->inDictionary());
    JS_ASSERT(obj->shape_->listp == &obj->shape_);

    if (parent)
        parent->listp = listp;
    *listp = parent;
    listp = NULL;
}

inline void
Shape::insertIntoDictionary(HeapPtrShape *dictp)
{
    



    JS_ASSERT(inDictionary());
    JS_ASSERT(!listp);

    JS_ASSERT_IF(*dictp, (*dictp)->inDictionary());
    JS_ASSERT_IF(*dictp, (*dictp)->listp == dictp);
    JS_ASSERT_IF(*dictp, compartment() == (*dictp)->compartment());

    setParent(*dictp);
    if (parent)
        parent->listp = &parent;
    listp = (HeapPtrShape *) dictp;
    *dictp = this;
}

void
Shape::initDictionaryShape(const StackShape &child, uint32_t nfixed, HeapPtrShape *dictp)
{
    new (this) Shape(child, nfixed);
    this->flags |= IN_DICTIONARY;

    this->listp = NULL;
    insertIntoDictionary(dictp);
}

inline
EmptyShape::EmptyShape(UnownedBaseShape *base, uint32_t nfixed)
  : js::Shape(base, nfixed)
{
    
    if (!getObjectClass()->isNative())
        flags |= NON_NATIVE;
}

inline void
Shape::writeBarrierPre(const js::Shape *shape)
{
#ifdef JSGC_INCREMENTAL
    if (!shape)
        return;

    JSCompartment *comp = shape->compartment();
    if (comp->needsBarrier())
        MarkShapeUnbarriered(comp->barrierTracer(), shape, "write barrier");
#endif
}

inline void
Shape::writeBarrierPost(const js::Shape *shape, void *addr)
{
}

inline void
Shape::readBarrier(const Shape *shape)
{
#ifdef JSGC_INCREMENTAL
    JSCompartment *comp = shape->compartment();
    if (comp->needsBarrier())
        MarkShapeUnbarriered(comp->barrierTracer(), shape, "read barrier");
#endif
}

inline void
BaseShape::writeBarrierPre(BaseShape *base)
{
#ifdef JSGC_INCREMENTAL
    if (!base)
        return;

    JSCompartment *comp = base->compartment();
    if (comp->needsBarrier())
        MarkBaseShapeUnbarriered(comp->barrierTracer(), base, "write barrier");
#endif
}

inline void
BaseShape::writeBarrierPost(BaseShape *shape, void *addr)
{
}

inline void
BaseShape::readBarrier(BaseShape *base)
{
#ifdef JSGC_INCREMENTAL
    JSCompartment *comp = base->compartment();
    if (comp->needsBarrier())
        MarkBaseShapeUnbarriered(comp->barrierTracer(), base, "read barrier");
#endif
}

} 

#endif 
