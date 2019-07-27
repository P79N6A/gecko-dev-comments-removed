





#include "jspropertytree.h"

#include "jscntxt.h"
#include "jsgc.h"
#include "jstypes.h"

#include "vm/Shape.h"

#include "jsgcinlines.h"

#include "vm/Shape-inl.h"

using namespace js;
using namespace js::gc;

inline HashNumber
ShapeHasher::hash(const Lookup &l)
{
    return l.hash();
}

inline bool
ShapeHasher::match(const Key k, const Lookup &l)
{
    return k->matches(l);
}

Shape *
PropertyTree::newShape(ExclusiveContext *cx)
{
    Shape *shape = js_NewGCShape(cx);
    if (!shape)
        js_ReportOutOfMemory(cx);
    return shape;
}

static KidsHash *
HashChildren(Shape *kid1, Shape *kid2)
{
    KidsHash *hash = js_new<KidsHash>();
    if (!hash || !hash->init(2)) {
        js_delete(hash);
        return nullptr;
    }

    JS_ALWAYS_TRUE(hash->putNew(StackShape(kid1), kid1));
    JS_ALWAYS_TRUE(hash->putNew(StackShape(kid2), kid2));
    return hash;
}

bool
PropertyTree::insertChild(ExclusiveContext *cx, Shape *parent, Shape *child)
{
    MOZ_ASSERT(!parent->inDictionary());
    MOZ_ASSERT(!child->parent);
    MOZ_ASSERT(!child->inDictionary());
    MOZ_ASSERT(child->compartment() == parent->compartment());
    MOZ_ASSERT(cx->isInsideCurrentCompartment(this));

    KidsPointer *kidp = &parent->kids;

    if (kidp->isNull()) {
        child->setParent(parent);
        kidp->setShape(child);
        return true;
    }

    if (kidp->isShape()) {
        Shape *shape = kidp->toShape();
        MOZ_ASSERT(shape != child);
        MOZ_ASSERT(!shape->matches(child));

        KidsHash *hash = HashChildren(shape, child);
        if (!hash) {
            js_ReportOutOfMemory(cx);
            return false;
        }
        kidp->setHash(hash);
        child->setParent(parent);
        return true;
    }

    if (!kidp->toHash()->putNew(StackShape(child), child)) {
        js_ReportOutOfMemory(cx);
        return false;
    }

    child->setParent(parent);
    return true;
}

void
Shape::removeChild(Shape *child)
{
    MOZ_ASSERT(!child->inDictionary());
    MOZ_ASSERT(child->parent == this);

    KidsPointer *kidp = &kids;

    if (kidp->isShape()) {
        MOZ_ASSERT(kidp->toShape() == child);
        kidp->setNull();
        child->parent = nullptr;
        return;
    }

    KidsHash *hash = kidp->toHash();
    MOZ_ASSERT(hash->count() >= 2);      

    hash->remove(StackShape(child));
    child->parent = nullptr;

    if (hash->count() == 1) {
        
        KidsHash::Range r = hash->all();
        Shape *otherChild = r.front();
        MOZ_ASSERT((r.popFront(), r.empty()));    
        kidp->setShape(otherChild);
        js_delete(hash);
    }
}

Shape *
PropertyTree::getChild(ExclusiveContext *cx, Shape *parentArg, StackShape &unrootedChild)
{
    RootedShape parent(cx, parentArg);
    MOZ_ASSERT(parent);

    Shape *existingShape = nullptr;

    







    KidsPointer *kidp = &parent->kids;
    if (kidp->isShape()) {
        Shape *kid = kidp->toShape();
        if (kid->matches(unrootedChild))
            existingShape = kid;
    } else if (kidp->isHash()) {
        if (KidsHash::Ptr p = kidp->toHash()->lookup(unrootedChild))
            existingShape = *p;
    } else {
        
    }

#ifdef JSGC_INCREMENTAL
    if (existingShape) {
        JS::Zone *zone = existingShape->zone();
        if (zone->needsIncrementalBarrier()) {
            



            Shape *tmp = existingShape;
            MarkShapeUnbarriered(zone->barrierTracer(), &tmp, "read barrier");
            MOZ_ASSERT(tmp == existingShape);
        } else if (zone->isGCSweeping() && !existingShape->isMarked() &&
                   !existingShape->arenaHeader()->allocatedDuringIncremental)
        {
            



            MOZ_ASSERT(parent->isMarked());
            parent->removeChild(existingShape);
            existingShape = nullptr;
        } else if (existingShape->isMarked(gc::GRAY)) {
            JS::UnmarkGrayGCThingRecursively(existingShape, JSTRACE_SHAPE);
        }
    }
#endif

    if (existingShape)
        return existingShape;

    RootedGeneric<StackShape*> child(cx, &unrootedChild);

    Shape *shape = newShape(cx);
    if (!shape)
        return nullptr;

    new (shape) Shape(*child, parent->numFixedSlots());

    if (!insertChild(cx, parent, shape))
        return nullptr;

    return shape;
}

Shape *
PropertyTree::lookupChild(ThreadSafeContext *cx, Shape *parent, const StackShape &child)
{
    
    Shape *shape = nullptr;

    MOZ_ASSERT(parent);

    KidsPointer *kidp = &parent->kids;
    if (kidp->isShape()) {
        Shape *kid = kidp->toShape();
        if (kid->matches(child))
            shape = kid;
    } else if (kidp->isHash()) {
        if (KidsHash::Ptr p = kidp->toHash()->readonlyThreadsafeLookup(child))
            shape = *p;
    } else {
        return nullptr;
    }

#if defined(JSGC_INCREMENTAL) && defined(DEBUG)
    if (shape) {
        JS::Zone *zone = shape->arenaHeader()->zone;
        MOZ_ASSERT(!zone->needsIncrementalBarrier());
        MOZ_ASSERT(!(zone->isGCSweeping() && !shape->isMarked() &&
                     !shape->arenaHeader()->allocatedDuringIncremental));
    }
#endif

    return shape;
}

void
Shape::sweep()
{
    if (inDictionary())
        return;

    





















    if (parent && parent->isMarked() && parent->compartment() == compartment())
        parent->removeChild(this);
}

void
Shape::finalize(FreeOp *fop)
{
    if (!inDictionary() && kids.isHash())
        fop->delete_(kids.toHash());
}

#ifdef JSGC_COMPACTING

void
Shape::fixupDictionaryShapeAfterMovingGC()
{
    if (!listp)
        return;

    
    
    
    if (IsInsideNursery(reinterpret_cast<Cell *>(listp))) {
        listp = nullptr;
        return;
    }

    MOZ_ASSERT(!IsInsideNursery(reinterpret_cast<Cell *>(listp)));
    AllocKind kind = TenuredCell::fromPointer(listp)->getAllocKind();
    MOZ_ASSERT(kind == FINALIZE_SHAPE || kind <= FINALIZE_OBJECT_LAST);
    if (kind == FINALIZE_SHAPE) {
        
        Shape *next = reinterpret_cast<Shape *>(uintptr_t(listp) -
                                                offsetof(Shape, parent));
        listp = &gc::MaybeForwarded(next)->parent;
    } else {
        
        JSObject *last = reinterpret_cast<JSObject *>(uintptr_t(listp) -
                                                      offsetof(JSObject, shape_));
        listp = &gc::MaybeForwarded(last)->shape_;
    }
}

void
Shape::fixupShapeTreeAfterMovingGC()
{
    if (kids.isNull())
        return;

    if (kids.isShape()) {
        if (gc::IsForwarded(kids.toShape()))
            kids.setShape(gc::Forwarded(kids.toShape()));
        return;
    }

    MOZ_ASSERT(kids.isHash());
    KidsHash *kh = kids.toHash();
    for (KidsHash::Enum e(*kh); !e.empty(); e.popFront()) {
        Shape *key = e.front();
        if (!IsForwarded(key))
            continue;

        key = Forwarded(key);
        BaseShape *base = key->base();
        if (IsForwarded(base))
            base = Forwarded(base);
        UnownedBaseShape *unowned = base->unowned();
        if (IsForwarded(unowned))
            unowned = Forwarded(unowned);
        StackShape lookup(unowned,
                          const_cast<Shape *>(key)->propidRef(),
                          key->slotInfo & Shape::SLOT_MASK,
                          key->attrs,
                          key->flags);
        e.rekeyFront(lookup, key);
    }
}

void
Shape::fixupAfterMovingGC()
{
    if (inDictionary())
        fixupDictionaryShapeAfterMovingGC();
    else
        fixupShapeTreeAfterMovingGC();
}

#endif 

#ifdef DEBUG

void
KidsPointer::checkConsistency(Shape *aKid) const
{
    if (isShape()) {
        MOZ_ASSERT(toShape() == aKid);
    } else {
        MOZ_ASSERT(isHash());
        KidsHash *hash = toHash();
        KidsHash::Ptr ptr = hash->lookup(StackShape(aKid));
        MOZ_ASSERT(*ptr == aKid);
    }
}

void
Shape::dump(JSContext *cx, FILE *fp) const
{
    
    gc::AutoSuppressGC suppress(cx);

    jsid propid = this->propid();

    MOZ_ASSERT(!JSID_IS_VOID(propid));

    if (JSID_IS_INT(propid)) {
        fprintf(fp, "[%ld]", (long) JSID_TO_INT(propid));
    } else if (JSID_IS_ATOM(propid)) {
        if (JSLinearString *str = JSID_TO_ATOM(propid))
            FileEscapedString(fp, str, '"');
        else
            fputs("<error>", fp);
    } else {
        MOZ_ASSERT(JSID_IS_SYMBOL(propid));
        JSID_TO_SYMBOL(propid)->dump(fp);
    }

    fprintf(fp, " g/s %p/%p slot %d attrs %x ",
            JS_FUNC_TO_DATA_PTR(void *, base()->rawGetter),
            JS_FUNC_TO_DATA_PTR(void *, base()->rawSetter),
            hasSlot() ? slot() : -1, attrs);

    if (attrs) {
        int first = 1;
        fputs("(", fp);
#define DUMP_ATTR(name, display) if (attrs & JSPROP_##name) fputs(&(" " #display)[first], fp), first = 0
        DUMP_ATTR(ENUMERATE, enumerate);
        DUMP_ATTR(READONLY, readonly);
        DUMP_ATTR(PERMANENT, permanent);
        DUMP_ATTR(GETTER, getter);
        DUMP_ATTR(SETTER, setter);
        DUMP_ATTR(SHARED, shared);
#undef  DUMP_ATTR
        fputs(") ", fp);
    }

    fprintf(fp, "flags %x ", flags);
    if (flags) {
        int first = 1;
        fputs("(", fp);
#define DUMP_FLAG(name, display) if (flags & name) fputs(&(" " #display)[first], fp), first = 0
        DUMP_FLAG(IN_DICTIONARY, in_dictionary);
#undef  DUMP_FLAG
        fputs(") ", fp);
    }
}

void
Shape::dumpSubtree(JSContext *cx, int level, FILE *fp) const
{
    if (!parent) {
        MOZ_ASSERT(level == 0);
        MOZ_ASSERT(JSID_IS_EMPTY(propid_));
        fprintf(fp, "class %s emptyShape\n", getObjectClass()->name);
    } else {
        fprintf(fp, "%*sid ", level, "");
        dump(cx, fp);
    }

    if (!kids.isNull()) {
        ++level;
        if (kids.isShape()) {
            Shape *kid = kids.toShape();
            MOZ_ASSERT(kid->parent == this);
            kid->dumpSubtree(cx, level, fp);
        } else {
            const KidsHash &hash = *kids.toHash();
            for (KidsHash::Range range = hash.all(); !range.empty(); range.popFront()) {
                Shape *kid = range.front();

                MOZ_ASSERT(kid->parent == this);
                kid->dumpSubtree(cx, level, fp);
            }
        }
    }
}

#endif
