






































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

#include "vm/ArgumentsObject.h"
#include "vm/StringObject.h"

#include "jscntxtinlines.h"
#include "jsgcinlines.h"
#include "jsobjinlines.h"

inline js::EmptyShape *
js::types::TypeObject::getEmptyShape(JSContext *cx, js::Class *aclasp,
                                     gc::AllocKind kind)
{
    JS_ASSERT(!singleton);

    




    JS_ASSERT(proto->hasNewType(this));

    JS_ASSERT(kind >= js::gc::FINALIZE_OBJECT0 && kind <= js::gc::FINALIZE_OBJECT_LAST);
    int i = kind - js::gc::FINALIZE_OBJECT0;

    if (!emptyShapes) {
        emptyShapes = (js::EmptyShape**)
            cx->calloc_(sizeof(js::EmptyShape*) * js::gc::FINALIZE_FUNCTION_AND_OBJECT_LAST);
        if (!emptyShapes)
            return NULL;

        



        emptyShapes[0] = js::EmptyShape::create(cx, aclasp);
        if (!emptyShapes[0]) {
            cx->free_(emptyShapes);
            emptyShapes = NULL;
            return NULL;
        }
    }

    JS_ASSERT(aclasp == emptyShapes[0]->getClass());

    if (!emptyShapes[i]) {
        emptyShapes[i] = js::EmptyShape::create(cx, aclasp);
        if (!emptyShapes[i])
            return NULL;
    }

    return emptyShapes[i];
}

inline bool
js::types::TypeObject::canProvideEmptyShape(js::Class *aclasp)
{
    return proto && !singleton && (!emptyShapes || emptyShapes[0]->getClass() == aclasp);
}

inline void
JSObject::updateFlags(const js::Shape *shape, bool isDefinitelyAtom)
{
    jsuint index;
    if (!isDefinitelyAtom && js_IdIsIndex(shape->propid(), &index))
        setIndexed();
}

inline bool
JSObject::extend(JSContext *cx, const js::Shape *shape, bool isDefinitelyAtom)
{
    if (!setLastProperty(cx, shape))
        return false;
    updateFlags(shape, isDefinitelyAtom);
    return true;
}

namespace js {

inline bool
StringObject::init(JSContext *cx, JSString *str)
{
    JS_ASSERT(nativeEmpty());

    const Shape **shapep = &cx->compartment->initialStringShape;
    if (*shapep) {
        setLastPropertyInfallible(*shapep);
    } else {
        *shapep = assignInitialShape(cx);
        if (!*shapep)
            return false;
    }
    JS_ASSERT(*shapep == lastProperty());
    JS_ASSERT(!nativeEmpty());
    JS_ASSERT(nativeLookup(cx, ATOM_TO_JSID(cx->runtime->atomState.lengthAtom))->slot() == LENGTH_SLOT);

    setStringThis(str);
    return true;
}

inline
Shape::Shape(BaseShape *base, jsid propid, uint32 slot,
             uintN attrs, uintN flags, intN shortid)
  : base_(base),
    propid_(propid),
    numLinearSearches(0),
    slot_(slot),
    attrs(uint8(attrs)),
    flags(uint8(flags)),
    shortid_(int16(shortid)),
    parent(NULL)
{
    JS_ASSERT(base);
    JS_ASSERT(!JSID_IS_VOID(propid));
    JS_ASSERT_IF(isMethod(), !base->rawGetter);
    kids.setNull();
}

inline
Shape::Shape(BaseShape *base)
  : base_(base),
    propid_(JSID_EMPTY),
    numLinearSearches(0),
    slot_(-1),
    attrs(JSPROP_SHARED),
    flags(0),
    shortid_(0),
    parent(NULL)
{
    JS_ASSERT(base);
    kids.setNull();
}

inline JSDHashNumber
Shape::hash() const
{
    JSDHashNumber hash = jsuword(base()->unowned());

    
    hash = JS_ROTATE_LEFT32(hash, 4) ^ (flags & PUBLIC_FLAGS);
    hash = JS_ROTATE_LEFT32(hash, 4) ^ attrs;
    hash = JS_ROTATE_LEFT32(hash, 4) ^ shortid_;
    hash = JS_ROTATE_LEFT32(hash, 4) ^ slot_;
    hash = JS_ROTATE_LEFT32(hash, 4) ^ JSID_BITS(propid_);
    return hash;
}

inline bool
Shape::matches(const js::Shape *other) const
{
    return propid_ == other->propid_ &&
           matchesParamsAfterId(other->base(), other->slot_, other->attrs,
                                other->flags, other->shortid_);
}

inline bool
Shape::matchesParamsAfterId(BaseShape *base, uint32 aslot,
                            uintN aattrs, uintN aflags, intN ashortid) const
{
    return base->unowned() == this->base()->unowned() &&
           slot_ == aslot &&
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
        obj = js_UnwrapWithObject(cx, obj);
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
        obj = js_UnwrapWithObject(cx, obj);
    return js::CallJSPropertyOpSetter(cx, setterOp(), obj, getUserId(), strict, vp);
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
Shape::insertIntoDictionary(js::Shape **dictp)
{
    



    JS_ASSERT(inDictionary());
    JS_ASSERT(!listp);

    JS_ASSERT_IF(*dictp, (*dictp)->inDictionary());
    JS_ASSERT_IF(*dictp, (*dictp)->listp == dictp);
    JS_ASSERT_IF(*dictp, compartment() == (*dictp)->compartment());

    setParent(*dictp);
    if (parent)
        parent->listp = &parent;
    listp = dictp;
    *dictp = this;
}

void
Shape::initDictionaryShape(const Shape &child, Shape **dictp)
{
    UnownedBaseShape *base = child.base()->unowned();

    new (this) Shape(base, child.maybePropid(), child.maybeSlot(), child.attrs,
                     child.flags | IN_DICTIONARY, child.maybeShortid());

    this->listp = NULL;
    insertIntoDictionary(dictp);
}

inline
EmptyShape::EmptyShape(BaseShape *base)
  : js::Shape(base)
{
    
    if (!getClass()->isNative())
        flags |= NON_NATIVE;
}

 inline EmptyShape *
EmptyShape::getEmptyArgumentsShape(JSContext *cx, bool strict)
{
    if (strict)
        return ensure(cx, &StrictArgumentsObjectClass, &cx->compartment->emptyStrictArgumentsShape);
    return ensure(cx, &NormalArgumentsObjectClass, &cx->compartment->emptyNormalArgumentsShape);
}

 inline EmptyShape *
EmptyShape::getEmptyBlockShape(JSContext *cx)
{
    return ensure(cx, &BlockClass, &cx->compartment->emptyBlockShape);
}

 inline EmptyShape *
EmptyShape::getEmptyCallShape(JSContext *cx)
{
    return ensure(cx, &CallClass, &cx->compartment->emptyCallShape);
}

 inline EmptyShape *
EmptyShape::getEmptyDeclEnvShape(JSContext *cx)
{
    return ensure(cx, &DeclEnvClass, &cx->compartment->emptyDeclEnvShape);
}

 inline EmptyShape *
EmptyShape::getEmptyEnumeratorShape(JSContext *cx)
{
    return ensure(cx, &IteratorClass, &cx->compartment->emptyEnumeratorShape);
}

 inline EmptyShape *
EmptyShape::getEmptyWithShape(JSContext *cx)
{
    return ensure(cx, &WithClass, &cx->compartment->emptyWithShape);
}

} 

#endif 
