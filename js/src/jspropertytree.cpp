






































#include <new>

#include "jstypes.h"
#include "jsprf.h"
#include "jsapi.h"
#include "jscntxt.h"
#include "jsgc.h"
#include "jspropertytree.h"
#include "jsscope.h"

#include "jsobjinlines.h"
#include "jsscopeinlines.h"

using namespace js;

inline HashNumber
ShapeHasher::hash(const Lookup l)
{
    return l->hash();
}

inline bool
ShapeHasher::match(const Key k, const Lookup l)
{
    return k->matches(l);
}

Shape *
PropertyTree::newShape(JSContext *cx)
{
    Shape *shape = js_NewGCShape(cx);
    if (!shape) {
        JS_ReportOutOfMemory(cx);
        return NULL;
    }
    return shape;
}

static KidsHash *
HashChildren(Shape *kid1, Shape *kid2)
{
    KidsHash *hash = OffTheBooks::new_<KidsHash>();
    if (!hash || !hash->init(2)) {
        Foreground::delete_(hash);
        return NULL;
    }

    JS_ALWAYS_TRUE(hash->putNew(kid1));
    JS_ALWAYS_TRUE(hash->putNew(kid2));
    return hash;
}

bool
PropertyTree::insertChild(JSContext *cx, Shape *parent, Shape *child)
{
    JS_ASSERT(!parent->inDictionary());
    JS_ASSERT(!child->parent);
    JS_ASSERT(!child->inDictionary());
    JS_ASSERT(cx->compartment == compartment);
    JS_ASSERT(child->compartment() == parent->compartment());

    KidsPointer *kidp = &parent->kids;

    if (kidp->isNull()) {
        child->setParent(parent);
        kidp->setShape(child);
        return true;
    }

    if (kidp->isShape()) {
        Shape *shape = kidp->toShape();
        JS_ASSERT(shape != child);
        JS_ASSERT(!shape->matches(child));

        KidsHash *hash = HashChildren(shape, child);
        if (!hash) {
            JS_ReportOutOfMemory(cx);
            return false;
        }
        kidp->setHash(hash);
        child->setParent(parent);
        return true;
    }

    if (!kidp->toHash()->putNew(child)) {
        JS_ReportOutOfMemory(cx);
        return false;
    }

    child->setParent(parent);
    return true;
}

void
Shape::removeChild(Shape *child)
{
    JS_ASSERT(!child->inDictionary());

    KidsPointer *kidp = &kids;

    if (kidp->isShape()) {
        JS_ASSERT(kidp->toShape() == child);
        kidp->setNull();
        return;
    }

    KidsHash *hash = kidp->toHash();
    JS_ASSERT(hash->count() >= 2);      
    hash->remove(child);
    if (hash->count() == 1) {
        
        KidsHash::Range r = hash->all(); 
        Shape *otherChild = r.front();
        JS_ASSERT((r.popFront(), r.empty()));    
        kidp->setShape(otherChild);
        js::UnwantedForeground::delete_(hash);
    }
}




static Shape *
ReadBarrier(Shape *shape)
{
#ifdef JSGC_INCREMENTAL
    JSCompartment *comp = shape->compartment();
    if (comp->needsBarrier())
        MarkShapeUnbarriered(comp->barrierTracer(), shape, "read barrier");
#endif
    return shape;
}

Shape *
PropertyTree::getChild(JSContext *cx, Shape *parent, const Shape &child)
{
    Shape *shape;

    JS_ASSERT(parent);

    







    KidsPointer *kidp = &parent->kids;
    if (kidp->isShape()) {
        shape = kidp->toShape();
        if (shape->matches(&child))
            return ReadBarrier(shape);
    } else if (kidp->isHash()) {
        shape = *kidp->toHash()->lookup(&child);
        if (shape)
            return ReadBarrier(shape);
    } else {
        
    }

    shape = newShape(cx);
    if (!shape)
        return NULL;

    UnownedBaseShape *base = child.base()->unowned();

    new (shape) Shape(&child);
    shape->base_.init(base);

    if (!insertChild(cx, parent, shape))
        return NULL;

    return shape;
}

void
Shape::finalize(JSContext *cx, bool background)
{
    if (!inDictionary()) {
        if (parent && parent->isMarked())
            parent->removeChild(this);

        if (kids.isHash())
            cx->delete_(kids.toHash());
    }
}

#ifdef DEBUG

void
KidsPointer::checkConsistency(const Shape *aKid) const
{
    if (isShape()) {
        JS_ASSERT(toShape() == aKid);
    } else {
        JS_ASSERT(isHash());
        KidsHash *hash = toHash();
        KidsHash::Ptr ptr = hash->lookup(aKid);
        JS_ASSERT(*ptr == aKid);
    }
}

void
Shape::dump(JSContext *cx, FILE *fp) const
{
    jsid propid = this->propid();

    JS_ASSERT(!JSID_IS_VOID(propid));

    if (JSID_IS_INT(propid)) {
        fprintf(fp, "[%ld]", (long) JSID_TO_INT(propid));
    } else if (JSID_IS_DEFAULT_XML_NAMESPACE(propid)) {
        fprintf(fp, "<default XML namespace>");
    } else {
        JSLinearString *str;
        if (JSID_IS_ATOM(propid)) {
            str = JSID_TO_ATOM(propid);
        } else {
            JS_ASSERT(JSID_IS_OBJECT(propid));
            JSString *s = js_ValueToString(cx, IdToValue(propid));
            fputs("object ", fp);
            str = s ? s->ensureLinear(cx) : NULL;
        }
        if (!str)
            fputs("<error>", fp);
        else
            FileEscapedString(fp, str, '"');
    }

    fprintf(fp, " g/s %p/%p slot %d attrs %x ",
            JS_FUNC_TO_DATA_PTR(void *, base()->rawGetter),
            JS_FUNC_TO_DATA_PTR(void *, base()->rawSetter),
            hasSlot() ? slot() : -1, attrs);

    if (attrs) {
        int first = 1;
        fputs("(", fp);
#define DUMP_ATTR(name, display) if (attrs & JSPROP_##name) fputs(" " #display + first, fp), first = 0
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
#define DUMP_FLAG(name, display) if (flags & name) fputs(" " #display + first, fp), first = 0
        DUMP_FLAG(HAS_SHORTID, has_shortid);
        DUMP_FLAG(METHOD, method);
        DUMP_FLAG(IN_DICTIONARY, in_dictionary);
#undef  DUMP_FLAG
        fputs(") ", fp);
    }

    fprintf(fp, "shortid %d\n", shortid());
}

void
Shape::dumpSubtree(JSContext *cx, int level, FILE *fp) const
{
    if (!parent) {
        JS_ASSERT(level == 0);
        JS_ASSERT(JSID_IS_EMPTY(propid_));
        fprintf(fp, "class %s emptyShape\n", getObjectClass()->name);
    } else {
        fprintf(fp, "%*sid ", level, "");
        dump(cx, fp);
    }

    if (!kids.isNull()) {
        ++level;
        if (kids.isShape()) {
            Shape *kid = kids.toShape();
            JS_ASSERT(kid->parent == this);
            kid->dumpSubtree(cx, level, fp);
        } else {
            const KidsHash &hash = *kids.toHash();
            for (KidsHash::Range range = hash.all(); !range.empty(); range.popFront()) {
                Shape *kid = range.front();

                JS_ASSERT(kid->parent == this);
                kid->dumpSubtree(cx, level, fp);
            }
        }
    }
}

void
js::PropertyTree::dumpShapes(JSContext *cx)
{
    static bool init = false;
    static FILE *dumpfp = NULL;
    if (!init) {
        init = true;
        const char *name = getenv("JS_DUMP_SHAPES_FILE");
        if (!name)
            return;
        dumpfp = fopen(name, "a");
    }

    if (!dumpfp)
        return;

    JSRuntime *rt = cx->runtime;
    fprintf(dumpfp, "rt->gcNumber = %lu", (unsigned long)rt->gcNumber);

    for (JSCompartment **c = rt->compartments.begin(); c != rt->compartments.end(); ++c) {
        if (rt->gcCurrentCompartment != NULL && rt->gcCurrentCompartment != *c)
            continue;

        fprintf(dumpfp, "*** Compartment %p ***\n", (void *)*c);

        








    }
}
#endif
