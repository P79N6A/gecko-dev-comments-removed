










































#include <new>
#include <stdlib.h>
#include <string.h>
#include "jstypes.h"
#include "jsstdint.h"
#include "jsarena.h"
#include "jsbit.h"
#include "jsclist.h"
#include "jsdhash.h"
#include "jsutil.h" 
#include "jsapi.h"
#include "jsatom.h"
#include "jscntxt.h"
#include "jsdbgapi.h"
#include "jsfun.h"      
#include "jslock.h"
#include "jsnum.h"
#include "jsobj.h"
#include "jsscope.h"
#include "jsstr.h"
#include "jstracer.h"

#include "jsobjinlines.h"
#include "jsscopeinlines.h"

using namespace js;

uint32
js_GenerateShape(JSContext *cx, bool gcLocked)
{
    JSRuntime *rt;
    uint32 shape;

    rt = cx->runtime;
    shape = JS_ATOMIC_INCREMENT(&rt->shapeGen);
    JS_ASSERT(shape != 0);
    if (shape >= SHAPE_OVERFLOW_BIT) {
        





        rt->shapeGen = SHAPE_OVERFLOW_BIT;
        shape = SHAPE_OVERFLOW_BIT;
        cx->triggerGC(gcLocked);
    }
    return shape;
}

JSScope *
js_GetMutableScope(JSContext *cx, JSObject *obj)
{
    JSScope *scope = obj->scope();
    JS_ASSERT(JS_IS_SCOPE_LOCKED(cx, scope));
    if (!scope->isSharedEmpty())
        return scope;

    JSScope *newscope = JSScope::create(cx, scope->ops, obj->getClass(), obj, scope->shape);
    if (!newscope)
        return NULL;

    
    JS_ASSERT(CX_OWNS_SCOPE_TITLE(cx, newscope));
    JS_ASSERT(JS_IS_SCOPE_LOCKED(cx, newscope));
    obj->map = newscope;

    








    JS_ASSERT(newscope->freeslot >= JSSLOT_START(obj->getClass()) &&
              newscope->freeslot <= JSSLOT_FREE(obj->getClass()));
    newscope->freeslot = JSSLOT_FREE(obj->getClass());

    uint32 nslots = obj->numSlots();
    if (newscope->freeslot > nslots && !obj->allocSlots(cx, newscope->freeslot)) {
        newscope->destroy(cx);
        obj->map = scope;
        return NULL;
    }

    if (nslots > JS_INITIAL_NSLOTS && nslots > newscope->freeslot)
        newscope->freeslot = nslots;
#ifdef DEBUG
    if (newscope->freeslot < nslots)
        obj->setSlot(newscope->freeslot, UndefinedValue());
#endif

    JS_DROP_ALL_EMPTY_SCOPE_LOCKS(cx, scope);
    static_cast<JSEmptyScope *>(scope)->drop(cx);
    return newscope;
}






#define SCOPE_HASH_THRESHOLD    6
#define MIN_SCOPE_SIZE_LOG2     4
#define MIN_SCOPE_SIZE          JS_BIT(MIN_SCOPE_SIZE_LOG2)
#define SCOPE_TABLE_NBYTES(n)   ((n) * sizeof(JSScopeProperty *))

void
JSScope::initMinimal(JSContext *cx, uint32 newShape)
{
    shape = newShape;
    emptyScope = NULL;
    hashShift = JS_DHASH_BITS - MIN_SCOPE_SIZE_LOG2;
    entryCount = removedCount = 0;
    table = NULL;
    lastProp = NULL;
}

#ifdef DEBUG
JS_FRIEND_DATA(JSScopeStats) js_scope_stats = {0};

# define METER(x)       JS_ATOMIC_INCREMENT(&js_scope_stats.x)
#else
# define METER(x)
#endif

bool
JSScope::createTable(JSContext *cx, bool report)
{
    int sizeLog2;
    JSScopeProperty *sprop, **spp;

    JS_ASSERT(!table);
    JS_ASSERT(lastProp);

    if (entryCount > SCOPE_HASH_THRESHOLD) {
        






        sizeLog2 = JS_CeilingLog2(2 * entryCount);
        hashShift = JS_DHASH_BITS - sizeLog2;
    } else {
        JS_ASSERT(hashShift == JS_DHASH_BITS - MIN_SCOPE_SIZE_LOG2);
        sizeLog2 = MIN_SCOPE_SIZE_LOG2;
    }

    table = (JSScopeProperty **) js_calloc(JS_BIT(sizeLog2) * sizeof(JSScopeProperty *));
    if (!table) {
        if (report)
            JS_ReportOutOfMemory(cx);
        METER(tableAllocFails);
        return false;
    }
    cx->runtime->updateMallocCounter(JS_BIT(sizeLog2) * sizeof(JSScopeProperty *));

    hashShift = JS_DHASH_BITS - sizeLog2;
    for (sprop = lastProp; sprop; sprop = sprop->parent) {
        spp = search(sprop->id, true);
        SPROP_STORE_PRESERVING_COLLISION(spp, sprop);
    }
    return true;
}

JSScope *
JSScope::create(JSContext *cx, const JSObjectOps *ops, Class *clasp,
                JSObject *obj, uint32 shape)
{
    JS_ASSERT(ops->isNative());
    JS_ASSERT(obj);

    JSScope *scope = cx->create<JSScope>(ops, obj);
    if (!scope)
        return NULL;

    scope->freeslot = JSSLOT_START(clasp);
    scope->flags = cx->runtime->gcRegenShapesScopeFlag;
    scope->initMinimal(cx, shape);

#ifdef JS_THREADSAFE
    js_InitTitle(cx, &scope->title);
#endif
    JS_RUNTIME_METER(cx->runtime, liveScopes);
    JS_RUNTIME_METER(cx->runtime, totalScopes);
    return scope;
}

JSEmptyScope::JSEmptyScope(JSContext *cx, const JSObjectOps *ops,
                           Class *clasp)
    : JSScope(ops, NULL), clasp(clasp)
{
    



    nrefs = 2;
    freeslot = JSSLOT_START(clasp);
    flags = OWN_SHAPE | cx->runtime->gcRegenShapesScopeFlag;
    initMinimal(cx, js_GenerateShape(cx, false));

#ifdef JS_THREADSAFE
    js_InitTitle(cx, &title);
#endif
    JS_RUNTIME_METER(cx->runtime, liveScopes);
    JS_RUNTIME_METER(cx->runtime, totalScopes);
}

#ifdef DEBUG
# include "jsprf.h"
# define LIVE_SCOPE_METER(cx,expr) JS_LOCK_RUNTIME_VOID(cx->runtime,expr)
#else
# define LIVE_SCOPE_METER(cx,expr)
#endif

void
JSScope::destroy(JSContext *cx)
{
#ifdef JS_THREADSAFE
    js_FinishTitle(cx, &title);
#endif
    if (table)
        cx->free(table);

    



    if (emptyScope)
        emptyScope->dropFromGC(cx);

    LIVE_SCOPE_METER(cx, cx->runtime->liveScopeProps -= entryCount);
    JS_RUNTIME_UNMETER(cx->runtime, liveScopes);
    cx->free(this);
}


bool
JSScope::initRuntimeState(JSContext *cx)
{
    JSRuntime *rt = cx->runtime;

#define SCOPE(Name) rt->empty##Name##Scope
#define CLASP(Name) &js_##Name##Class

#define INIT_EMPTY_SCOPE(Name,NAME,ops)                                       \
    INIT_EMPTY_SCOPE_WITH_CLASS(Name, NAME, ops, CLASP(Name))

#define INIT_EMPTY_SCOPE_WITH_CLASS(Name,NAME,ops,clasp)                      \
    INIT_EMPTY_SCOPE_WITH_FREESLOT(Name, NAME, ops, clasp, JSSLOT_FREE(clasp))

#define INIT_EMPTY_SCOPE_WITH_FREESLOT(Name,NAME,ops,clasp,slot)              \
    SCOPE(Name) = cx->create<JSEmptyScope>(cx, ops, clasp);                   \
    if (!SCOPE(Name))                                                         \
        return false;                                                         \
    JS_ASSERT(SCOPE(Name)->shape == JSScope::EMPTY_##NAME##_SHAPE);           \
    JS_ASSERT(SCOPE(Name)->nrefs == 2);                                       \
    SCOPE(Name)->nrefs = 1;                                                   \
    SCOPE(Name)->freeslot = slot

    

















    INIT_EMPTY_SCOPE_WITH_FREESLOT(Arguments, ARGUMENTS, &js_ObjectOps, CLASP(Arguments),
                                   JS_INITIAL_NSLOTS + JS_ARGS_LENGTH_MAX);

    INIT_EMPTY_SCOPE(Block, BLOCK, &js_ObjectOps);

    






    INIT_EMPTY_SCOPE_WITH_FREESLOT(Call, CALL, &js_ObjectOps, CLASP(Call),
                                   JS_INITIAL_NSLOTS + JSFunction::MAX_ARGS_AND_VARS);

    
    INIT_EMPTY_SCOPE(DeclEnv, DECL_ENV, &js_ObjectOps);

    
    INIT_EMPTY_SCOPE_WITH_CLASS(Enumerator, ENUMERATOR, &js_ObjectOps, &js_IteratorClass.base);

    
    INIT_EMPTY_SCOPE(With, WITH, &js_WithObjectOps);

#undef SCOPE
#undef CLASP
#undef INIT_EMPTY_SCOPE
#undef INIT_EMPTY_SCOPE_WITH_CLASS
#undef INIT_EMPTY_SCOPE_WITH_FREESLOT

    return true;
}


void
JSScope::finishRuntimeState(JSContext *cx)
{
    JSRuntime *rt = cx->runtime;

#define FINISH_EMPTY_SCOPE(Name)                                              \
    if (rt->empty##Name##Scope) {                                             \
        rt->empty##Name##Scope->drop(cx);                                     \
        rt->empty##Name##Scope = NULL;                                        \
    }

    
    FINISH_EMPTY_SCOPE(Arguments);
    FINISH_EMPTY_SCOPE(Block);
    FINISH_EMPTY_SCOPE(Call);
    FINISH_EMPTY_SCOPE(DeclEnv);
    FINISH_EMPTY_SCOPE(Enumerator);
    FINISH_EMPTY_SCOPE(With);

#undef FINISH_EMPTY_SCOPE
}

JS_STATIC_ASSERT(sizeof(JSHashNumber) == 4);
JS_STATIC_ASSERT(sizeof(jsid) == JS_BYTES_PER_WORD);

#if JS_BYTES_PER_WORD == 4
# define HASH_ID(id) ((JSHashNumber)(JSID_BITS(id)))
#elif JS_BYTES_PER_WORD == 8
# define HASH_ID(id) ((JSHashNumber)(JSID_BITS(id)) ^ (JSHashNumber)((JSID_BITS(id)) >> 32))
#else
# error "Unsupported configuration"
#endif







#define SCOPE_HASH0(id)                 (HASH_ID(id) * JS_GOLDEN_RATIO)
#define SCOPE_HASH1(hash0,shift)        ((hash0) >> (shift))
#define SCOPE_HASH2(hash0,log2,shift)   ((((hash0) << (log2)) >> (shift)) | 1)

JSScopeProperty **
JSScope::searchTable(jsid id, bool adding)
{
    JSHashNumber hash0, hash1, hash2;
    int sizeLog2;
    JSScopeProperty *stored, *sprop, **spp, **firstRemoved;
    uint32 sizeMask;

    JS_ASSERT(table);
    JS_ASSERT(!JSID_IS_VOID(id));

    
    METER(hashes);
    hash0 = SCOPE_HASH0(id);
    hash1 = SCOPE_HASH1(hash0, hashShift);
    spp = table + hash1;

    
    stored = *spp;
    if (SPROP_IS_FREE(stored)) {
        METER(misses);
        return spp;
    }

    
    sprop = SPROP_CLEAR_COLLISION(stored);
    if (sprop && sprop->id == id) {
        METER(hits);
        return spp;
    }

    
    sizeLog2 = JS_DHASH_BITS - hashShift;
    hash2 = SCOPE_HASH2(hash0, sizeLog2, hashShift);
    sizeMask = JS_BITMASK(sizeLog2);

#ifdef DEBUG
    jsuword collision_flag = SPROP_COLLISION;
#endif

    
    if (SPROP_IS_REMOVED(stored)) {
        firstRemoved = spp;
    } else {
        firstRemoved = NULL;
        if (adding && !SPROP_HAD_COLLISION(stored))
            SPROP_FLAG_COLLISION(spp, sprop);
#ifdef DEBUG
        collision_flag &= jsuword(*spp) & SPROP_COLLISION;
#endif
    }

    for (;;) {
        METER(steps);
        hash1 -= hash2;
        hash1 &= sizeMask;
        spp = table + hash1;

        stored = *spp;
        if (SPROP_IS_FREE(stored)) {
            METER(stepMisses);
            return (adding && firstRemoved) ? firstRemoved : spp;
        }

        sprop = SPROP_CLEAR_COLLISION(stored);
        if (sprop && sprop->id == id) {
            METER(stepHits);
            JS_ASSERT(collision_flag);
            return spp;
        }

        if (SPROP_IS_REMOVED(stored)) {
            if (!firstRemoved)
                firstRemoved = spp;
        } else {
            if (adding && !SPROP_HAD_COLLISION(stored))
                SPROP_FLAG_COLLISION(spp, sprop);
#ifdef DEBUG
            collision_flag &= jsuword(*spp) & SPROP_COLLISION;
#endif
        }
    }

    
    return NULL;
}

bool
JSScope::changeTable(JSContext *cx, int change)
{
    int oldlog2, newlog2;
    uint32 oldsize, newsize, nbytes;
    JSScopeProperty **newtable, **oldtable, **spp, **oldspp, *sprop;

    if (!table)
        return createTable(cx, true);

    
    oldlog2 = JS_DHASH_BITS - hashShift;
    newlog2 = oldlog2 + change;
    oldsize = JS_BIT(oldlog2);
    newsize = JS_BIT(newlog2);
    nbytes = SCOPE_TABLE_NBYTES(newsize);
    newtable = (JSScopeProperty **) cx->calloc(nbytes);
    if (!newtable) {
        METER(tableAllocFails);
        return false;
    }

    
    hashShift = JS_DHASH_BITS - newlog2;
    removedCount = 0;
    oldtable = table;
    table = newtable;

    
    cx->runtime->updateMallocCounter(nbytes);

    
    for (oldspp = oldtable; oldsize != 0; oldspp++) {
        sprop = SPROP_FETCH(oldspp);
        if (sprop) {
            spp = search(sprop->id, true);
            JS_ASSERT(SPROP_IS_FREE(*spp));
            *spp = sprop;
        }
        oldsize--;
    }

    
    cx->free(oldtable);
    return true;
}






JSScopeProperty *
JSScope::getChildProperty(JSContext *cx, JSScopeProperty *parent,
                          JSScopeProperty &child)
{
    JS_ASSERT(!JSID_IS_VOID(child.id));
    JS_ASSERT(!child.inDictionary());

    





    if (!child.isAlias()) {
        if (child.attrs & JSPROP_SHARED) {
            child.slot = SPROP_INVALID_SLOT;
        } else {
            





            if (child.slot == SPROP_INVALID_SLOT &&
                !js_AllocSlot(cx, object, &child.slot)) {
                return NULL;
            }
        }
    }

    if (inDictionaryMode()) {
        JS_ASSERT(parent == lastProp);
        if (newDictionaryProperty(cx, child, &lastProp)) {
            updateShape(cx);
            return lastProp;
        }
        return NULL;
    }

    JSScopeProperty *sprop = JS_PROPERTY_TREE(cx).getChild(cx, parent, shape, child);
    if (sprop) {
        JS_ASSERT(sprop->parent == parent);
        if (parent == lastProp) {
            extend(cx, sprop);
        } else {
            JS_ASSERT(parent == lastProp->parent);
            setLastProperty(sprop);
            updateShape(cx);
        }
    }
    return sprop;
}

#ifdef DEBUG_notbrendan
#define CHECK_ANCESTOR_LINE(scope, sparse)                                    \
    JS_BEGIN_MACRO                                                            \
        if ((scope)->table) CheckAncestorLine(scope);                         \
    JS_END_MACRO

static void
CheckAncestorLine(JSScope *scope)
{
    uint32 size;
    JSScopeProperty **spp, **start, **end, *ancestorLine, *sprop, *aprop;
    uint32 entryCount, ancestorCount;

    ancestorLine = scope->lastProperty();
    if (ancestorLine)
        JS_ASSERT(scope->hasProperty(ancestorLine));

    entryCount = 0;
    size = SCOPE_CAPACITY(scope);
    start = scope->table;
    for (spp = start, end = start + size; spp < end; spp++) {
        sprop = SPROP_FETCH(spp);
        if (sprop) {
            ++entryCount;
            for (aprop = ancestorLine; aprop; aprop = aprop->parent) {
                if (aprop == sprop)
                    break;
            }
            JS_ASSERT(aprop);
        }
    }
    JS_ASSERT(entryCount == scope->entryCount);

    ancestorCount = 0;
    for (sprop = ancestorLine; sprop; sprop = sprop->parent)
        ancestorCount++;
    JS_ASSERT(ancestorCount == scope->entryCount);
}
#else
#define CHECK_ANCESTOR_LINE(scope, sparse)
#endif

void
JSScope::reportReadOnlyScope(JSContext *cx)
{
    JSString *str;
    const char *bytes;

    str = js_ValueToString(cx, ObjectOrNullValue(object));
    if (!str)
        return;
    bytes = js_GetStringBytes(cx, str);
    if (!bytes)
        return;
    JS_ReportErrorNumber(cx, js_GetErrorMessage, NULL, JSMSG_READ_ONLY, bytes);
}

void
JSScope::generateOwnShape(JSContext *cx)
{
#ifdef JS_TRACER
    if (object) {
         LeaveTraceIfGlobalObject(cx, object);

        




        JS_ASSERT_IF(JS_ON_TRACE(cx), cx->bailExit);

        




        TraceMonitor *tm = &JS_TRACE_MONITOR(cx);
        if (TraceRecorder *tr = tm->recorder)
            tr->forgetGuardedShapesForObject(object);
    }
#endif

    shape = js_GenerateShape(cx, false);
    setOwnShape();
}

JSScopeProperty *
JSScope::newDictionaryProperty(JSContext *cx, const JSScopeProperty &child,
                               JSScopeProperty **childp)
{
    JSScopeProperty *dprop = JS_PROPERTY_TREE(cx).newScopeProperty(cx);
    if (!dprop)
        return NULL;

    new (dprop) JSScopeProperty(child.id, child.rawGetter, child.rawSetter, child.slot,
                                child.attrs, child.flags | JSScopeProperty::IN_DICTIONARY,
                                child.shortid);
    dprop->shape = js_GenerateShape(cx, false);

    dprop->childp = NULL;
    insertDictionaryProperty(dprop, childp);
    updateFlags(dprop);
    return dprop;
}

bool
JSScope::toDictionaryMode(JSContext *cx, JSScopeProperty *&aprop)
{
    JS_ASSERT(!inDictionaryMode());

    JSScopeProperty **oldTable = table;
    uint32 saveRemovedCount = removedCount;
    if (oldTable) {
        int sizeLog2 = JS_DHASH_BITS - hashShift;
        JSScopeProperty **newTable = (JSScopeProperty **)
            js_calloc(JS_BIT(sizeLog2) * sizeof(JSScopeProperty *));

        if (!newTable) {
            JS_ReportOutOfMemory(cx);
            METER(toDictFails);
            return false;
        }
        table = newTable;
        removedCount = 0;
    }

    



    JSScopeProperty *oldLastProp = lastProp;
    lastProp = NULL;

    



    uint32 saveEntryCount = entryCount;
    entryCount = 0;

    for (JSScopeProperty *sprop = oldLastProp, **childp = &lastProp; sprop; sprop = sprop->parent) {
        JSScopeProperty *dprop = newDictionaryProperty(cx, *sprop, childp);
        if (!dprop) {
            entryCount = saveEntryCount;
            removedCount = saveRemovedCount;
            if (table)
                js_free(table);
            table = oldTable;
            lastProp = oldLastProp;
            METER(toDictFails);
            return false;
        }

        if (table) {
            JSScopeProperty **spp = search(dprop->id, true);
            JS_ASSERT(!SPROP_FETCH(spp));
            SPROP_STORE_PRESERVING_COLLISION(spp, dprop);
        }

        if (aprop == sprop)
            aprop = dprop;
        childp = &dprop->parent;
    }

    if (oldTable)
        js_free(oldTable);
    setDictionaryMode();
    clearOwnShape();

    if (lastProp) {
        




        shape = lastProp->shape;
    }
    return true;
}

JSScopeProperty *
JSScope::addProperty(JSContext *cx, jsid id,
                     PropertyOp getter, PropertyOp setter,
                     uint32 slot, uintN attrs,
                     uintN flags, intN shortid)
{
    JS_ASSERT(JS_IS_SCOPE_LOCKED(cx, this));
    CHECK_ANCESTOR_LINE(this, true);

    JS_ASSERT(!JSID_IS_VOID(id));
    JS_ASSERT_IF(!cx->runtime->gcRegenShapes,
                 hasRegenFlag(cx->runtime->gcRegenShapesScopeFlag));

    





    if (sealed()) {
        reportReadOnlyScope(cx);
        return NULL;
    }

    
    JSScopeProperty **spp = search(id, true);
    JS_ASSERT(!SPROP_FETCH(spp));
    return addPropertyHelper(cx, id, getter, setter, slot, attrs, flags, shortid, spp);
}





static inline bool
NormalizeGetterAndSetter(JSContext *cx, JSScope *scope,
                         jsid id, uintN attrs, uintN flags,
                         PropertyOp &getter,
                         PropertyOp &setter)
{
    if (setter == PropertyStub) {
        JS_ASSERT(!(attrs & JSPROP_SETTER));
        setter = NULL;
    }
    if (flags & JSScopeProperty::METHOD) {
        
        JS_ASSERT(getter);
        JS_ASSERT(!setter || setter == js_watch_set);
        JS_ASSERT(!(attrs & (JSPROP_GETTER | JSPROP_SETTER)));
    } else {
        if (getter == PropertyStub) {
            JS_ASSERT(!(attrs & JSPROP_GETTER));
            getter = NULL;
        }
    }

    




    if (!JS_CLIST_IS_EMPTY(&cx->runtime->watchPointList) &&
        js_FindWatchPoint(cx->runtime, scope, id)) {
        setter = js_WrapWatchedSetter(cx, id, attrs, setter);
        if (!setter) {
            METER(wrapWatchFails);
            return false;
        }
    }
    return true;
}

JSScopeProperty *
JSScope::addPropertyHelper(JSContext *cx, jsid id,
                           PropertyOp getter, PropertyOp setter,
                           uint32 slot, uintN attrs,
                           uintN flags, intN shortid,
                           JSScopeProperty **spp)
{
    NormalizeGetterAndSetter(cx, this, id, attrs, flags, getter, setter);

    
    uint32 size = SCOPE_CAPACITY(this);
    if (entryCount + removedCount >= size - (size >> 2)) {
        int change = removedCount < size >> 2;
        if (!change)
            METER(compresses);
        else
            METER(grows);
        if (!changeTable(cx, change) && entryCount + removedCount == size - 1)
            return NULL;
        spp = search(id, true);
        JS_ASSERT(!SPROP_FETCH(spp));
    }

    
    JSScopeProperty *sprop;
    {
        JSScopeProperty child(id, getter, setter, slot, attrs, flags, shortid);
        sprop = getChildProperty(cx, lastProp, child);
    }

    if (sprop) {
        
        if (table)
            SPROP_STORE_PRESERVING_COLLISION(spp, sprop);
        CHECK_ANCESTOR_LINE(this, false);
#ifdef DEBUG
        LIVE_SCOPE_METER(cx, ++cx->runtime->liveScopeProps);
        JS_RUNTIME_METER(cx->runtime, totalScopeProps);
#endif

        







        if (!table && entryCount >= SCOPE_HASH_THRESHOLD)
            (void) createTable(cx, false);

        METER(adds);
        return sprop;
    }

    METER(addFails);
    return NULL;
}

JSScopeProperty *
JSScope::putProperty(JSContext *cx, jsid id,
                     PropertyOp getter, PropertyOp setter,
                     uint32 slot, uintN attrs,
                     uintN flags, intN shortid)
{
    JSScopeProperty **spp, *sprop, *overwriting;

    JS_ASSERT(JS_IS_SCOPE_LOCKED(cx, this));
    CHECK_ANCESTOR_LINE(this, true);

    JS_ASSERT(!JSID_IS_VOID(id));

    JS_ASSERT_IF(!cx->runtime->gcRegenShapes,
                 hasRegenFlag(cx->runtime->gcRegenShapesScopeFlag));

    if (sealed()) {
        reportReadOnlyScope(cx);
        return NULL;
    }

    
    spp = search(id, true);
    sprop = SPROP_FETCH(spp);
    if (!sprop)
        return addPropertyHelper(cx, id, getter, setter, slot, attrs, flags, shortid, spp);

    
    JS_ASSERT(!SPROP_IS_REMOVED(*spp));
    overwriting = sprop;

    NormalizeGetterAndSetter(cx, this, id, attrs, flags, getter, setter);

    





    if (!(attrs & JSPROP_SHARED) &&
        slot == SPROP_INVALID_SLOT &&
        SPROP_HAS_VALID_SLOT(sprop, this)) {
        slot = sprop->slot;
    }
    if (sprop->matchesParamsAfterId(getter, setter, slot, attrs, flags, shortid)) {
        METER(redundantPuts);
        return sprop;
    }

    









    if (sprop == lastProp && !inDictionaryMode()) {
        removeLastProperty();
    } else {
        if (!inDictionaryMode()) {
            if (!toDictionaryMode(cx, sprop))
                return NULL;
            spp = search(id, false);
        }
        removeDictionaryProperty(sprop);
    }

    





    if (table)
        SPROP_STORE_PRESERVING_COLLISION(spp, NULL);
    CHECK_ANCESTOR_LINE(this, true);

    {
        
        JSScopeProperty child(id, getter, setter, slot, attrs, flags, shortid);
        sprop = getChildProperty(cx, lastProp, child);
    }

    if (sprop) {
        CHECK_ANCESTOR_LINE(this, false);

        if (table) {
            
            SPROP_STORE_PRESERVING_COLLISION(spp, sprop);
        } else if (entryCount >= SCOPE_HASH_THRESHOLD) {
            
            (void) createTable(cx, false);
        }

        METER(puts);
        return sprop;
    }

    if (table)
        SPROP_STORE_PRESERVING_COLLISION(spp, overwriting);
    ++entryCount;
    CHECK_ANCESTOR_LINE(this, true);
    METER(putFails);
    return NULL;
}

JSScopeProperty *
JSScope::changeProperty(JSContext *cx, JSScopeProperty *sprop,
                        uintN attrs, uintN mask,
                        PropertyOp getter, PropertyOp setter)
{
    JSScopeProperty *newsprop;

    JS_ASSERT(JS_IS_SCOPE_LOCKED(cx, this));
    CHECK_ANCESTOR_LINE(this, true);

    JS_ASSERT(!JSID_IS_VOID(sprop->id));
    JS_ASSERT(hasProperty(sprop));

    attrs |= sprop->attrs & mask;

    
    JS_ASSERT(!((attrs ^ sprop->attrs) & JSPROP_SHARED) ||
              !(attrs & JSPROP_SHARED));

    
    JS_ASSERT_IF(getter != sprop->rawGetter, !sprop->isMethod());

    if (getter == PropertyStub)
        getter = NULL;
    if (setter == PropertyStub)
        setter = NULL;
    if (sprop->attrs == attrs && sprop->getter() == getter && sprop->setter() == setter)
        return sprop;

    JSScopeProperty child(sprop->id, getter, setter, sprop->slot, attrs, sprop->flags,
                          sprop->shortid);
    if (inDictionaryMode()) {
        removeDictionaryProperty(sprop);
        newsprop = newDictionaryProperty(cx, child, &lastProp);
        if (newsprop) {
            if (table) {
                JSScopeProperty **spp = search(sprop->id, false);
                SPROP_STORE_PRESERVING_COLLISION(spp, newsprop);
            }
            updateShape(cx);
        }
    } else if (sprop == lastProp) {
        newsprop = getChildProperty(cx, sprop->parent, child);
        if (newsprop) {
            if (table) {
                JSScopeProperty **spp = search(sprop->id, false);
                JS_ASSERT(SPROP_FETCH(spp) == sprop);
                SPROP_STORE_PRESERVING_COLLISION(spp, newsprop);
            }
            CHECK_ANCESTOR_LINE(this, true);
        }
    } else {
        





        newsprop = putProperty(cx, child.id, child.rawGetter, child.rawSetter, child.slot,
                               child.attrs, child.flags, child.shortid);
    }

#ifdef DEBUG
    if (newsprop)
        METER(changes);
    else
        METER(changeFails);
#endif
    return newsprop;
}

bool
JSScope::removeProperty(JSContext *cx, jsid id)
{
    JSScopeProperty **spp, *sprop;
    uint32 size;

    JS_ASSERT(JS_IS_SCOPE_LOCKED(cx, this));
    CHECK_ANCESTOR_LINE(this, true);
    if (sealed()) {
        reportReadOnlyScope(cx);
        return false;
    }

    spp = search(id, false);
    sprop = SPROP_CLEAR_COLLISION(*spp);
    if (!sprop) {
        METER(uselessRemoves);
        return true;
    }

    
    if (sprop != lastProp) {
        if (!inDictionaryMode()) {
            if (!toDictionaryMode(cx, sprop))
                return false;
            spp = search(id, false);
        }
        JS_ASSERT(SPROP_FETCH(spp) == sprop);
    }

    
    if (SPROP_HAS_VALID_SLOT(sprop, this)) {
        js_FreeSlot(cx, object, sprop->slot);
        JS_ATOMIC_INCREMENT(&cx->runtime->propertyRemovals);
    }

    
    if (SPROP_HAD_COLLISION(*spp)) {
        JS_ASSERT(table);
        *spp = SPROP_REMOVED;
        ++removedCount;
    } else {
        METER(removeFrees);
        if (table) {
            *spp = NULL;
#ifdef DEBUG
            




            JSScopeProperty *aprop = lastProp;
            for (unsigned n = 50; aprop && n != 0; aprop = aprop->parent, --n)
                JS_ASSERT_IF(aprop != sprop, hasProperty(aprop));
#endif
        }
    }
    LIVE_SCOPE_METER(cx, --cx->runtime->liveScopeProps);

    if (inDictionaryMode()) {
        




        if (sprop != lastProp)
            setOwnShape();
        removeDictionaryProperty(sprop);
    } else {
        JS_ASSERT(sprop == lastProp);
        removeLastProperty();
    }
    updateShape(cx);
    CHECK_ANCESTOR_LINE(this, true);

    
    size = SCOPE_CAPACITY(this);
    if (size > MIN_SCOPE_SIZE && entryCount <= size >> 2) {
        METER(shrinks);
        (void) changeTable(cx, -1);
    }

    METER(removes);
    return true;
}

void
JSScope::clear(JSContext *cx)
{
    CHECK_ANCESTOR_LINE(this, true);
    LIVE_SCOPE_METER(cx, cx->runtime->liveScopeProps -= entryCount);

    if (table)
        js_free(table);
    clearDictionaryMode();
    clearOwnShape();
    LeaveTraceIfGlobalObject(cx, object);

    Class *clasp = object->getClass();
    JSObject *proto = object->getProto();
    JSEmptyScope *emptyScope;
    uint32 newShape;
    if (proto &&
        proto->isNative() &&
        (emptyScope = proto->scope()->emptyScope) &&
        emptyScope->clasp == clasp) {
        newShape = emptyScope->shape;
    } else {
        newShape = js_GenerateShape(cx, false);
    }
    initMinimal(cx, newShape);

    JS_ATOMIC_INCREMENT(&cx->runtime->propertyRemovals);
}

void
JSScope::deletingShapeChange(JSContext *cx, JSScopeProperty *sprop)
{
    JS_ASSERT(!JSID_IS_VOID(sprop->id));
    generateOwnShape(cx);
}

bool
JSScope::methodShapeChange(JSContext *cx, JSScopeProperty *sprop)
{
    JS_ASSERT(!JSID_IS_VOID(sprop->id));
    if (sprop->isMethod()) {
#ifdef DEBUG
        const Value &prev = object->lockedGetSlot(sprop->slot);
        JS_ASSERT(&sprop->methodObject() == &prev.toObject());
        JS_ASSERT(hasMethodBarrier());
        JS_ASSERT(object->getClass() == &js_ObjectClass);
        JS_ASSERT(!sprop->rawSetter || sprop->rawSetter == js_watch_set);
#endif

        





        sprop = putProperty(cx, sprop->id, NULL, sprop->rawSetter, sprop->slot,
                            sprop->attrs,
                            sprop->getFlags() & ~JSScopeProperty::METHOD,
                            sprop->shortid);
        if (!sprop)
            return false;
    }

    generateOwnShape(cx);
    return true;
}

bool
JSScope::methodShapeChange(JSContext *cx, uint32 slot)
{
    if (!hasMethodBarrier()) {
        generateOwnShape(cx);
    } else {
        for (JSScopeProperty *sprop = lastProp; sprop; sprop = sprop->parent) {
            JS_ASSERT(!JSID_IS_VOID(sprop->id));
            if (sprop->slot == slot)
                return methodShapeChange(cx, sprop);
        }
    }
    return true;
}

void
JSScope::protoShapeChange(JSContext *cx)
{
    generateOwnShape(cx);
}

void
JSScope::shadowingShapeChange(JSContext *cx, JSScopeProperty *sprop)
{
    JS_ASSERT(!JSID_IS_VOID(sprop->id));
    generateOwnShape(cx);
}

bool
JSScope::globalObjectOwnShapeChange(JSContext *cx)
{
    generateOwnShape(cx);
    return !js_IsPropertyCacheDisabled(cx);
}

#ifdef DEBUG
static void
PrintPropertyGetterOrSetter(JSTracer *trc, char *buf, size_t bufsize)
{
    JSScopeProperty *sprop;
    jsid id;
    size_t n;
    const char *name;

    JS_ASSERT(trc->debugPrinter == PrintPropertyGetterOrSetter);
    sprop = (JSScopeProperty *)trc->debugPrintArg;
    id = sprop->id;
    JS_ASSERT(!JSID_IS_VOID(id));
    name = trc->debugPrintIndex ? js_setter_str : js_getter_str;

    if (JSID_IS_ATOM(id)) {
        n = js_PutEscapedString(buf, bufsize - 1,
                                JSID_TO_STRING(id), 0);
        if (n < bufsize - 1)
            JS_snprintf(buf + n, bufsize - n, " %s", name);
    } else if (JSID_IS_INT(sprop->id)) {
        JS_snprintf(buf, bufsize, "%d %s", JSID_TO_INT(id), name);
    } else {
        JS_snprintf(buf, bufsize, "<object> %s", name);
    }
}

static void
PrintPropertyMethod(JSTracer *trc, char *buf, size_t bufsize)
{
    JSScopeProperty *sprop;
    jsid id;
    size_t n;

    JS_ASSERT(trc->debugPrinter == PrintPropertyMethod);
    sprop = (JSScopeProperty *)trc->debugPrintArg;
    id = sprop->id;
    JS_ASSERT(!JSID_IS_VOID(id));

    JS_ASSERT(JSID_IS_ATOM(id));
    n = js_PutEscapedString(buf, bufsize - 1, JSID_TO_STRING(id), 0);
    if (n < bufsize - 1)
        JS_snprintf(buf + n, bufsize - n, " method");
}
#endif

void
JSScopeProperty::trace(JSTracer *trc)
{
    if (IS_GC_MARKING_TRACER(trc))
        mark();
    MarkId(trc, id, "id");

    if (attrs & (JSPROP_GETTER | JSPROP_SETTER)) {
        if ((attrs & JSPROP_GETTER) && rawGetter) {
            JS_SET_TRACING_DETAILS(trc, PrintPropertyGetterOrSetter, this, 0);
            Mark(trc, getterObject(), JSTRACE_OBJECT);
        }
        if ((attrs & JSPROP_SETTER) && rawSetter) {
            JS_SET_TRACING_DETAILS(trc, PrintPropertyGetterOrSetter, this, 1);
            Mark(trc, setterObject(), JSTRACE_OBJECT);
        }
    }

    if (isMethod()) {
        JS_SET_TRACING_DETAILS(trc, PrintPropertyMethod, this, 0);
        Mark(trc, &methodObject(), JSTRACE_OBJECT);
    }
}
