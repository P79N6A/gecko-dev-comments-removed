





#ifndef gc_Marking_h
#define gc_Marking_h

#include "gc/Barrier.h"

class JSAtom;
class JSLinearString;

namespace js {

class ArgumentsObject;
class ArrayBufferObject;
class ArrayBufferViewObject;
class SharedArrayBufferObject;
class BaseShape;
class DebugScopeObject;
class GCMarker;
class GlobalObject;
class LazyScript;
class NestedScopeObject;
class SavedFrame;
class ScopeObject;
class Shape;
class UnownedBaseShape;

template<class> class HeapPtr;

namespace jit {
class JitCode;
struct IonScript;
struct VMFunction;
}

namespace types {
class Type;
}

namespace gc {















































#define DeclMarker(base, type)                                                                    \
void Mark##base(JSTracer *trc, BarrieredBase<type*> *thing, const char *name);                    \
void Mark##base##Root(JSTracer *trc, type **thingp, const char *name);                            \
void Mark##base##Unbarriered(JSTracer *trc, type **thingp, const char *name);                     \
void Mark##base##Range(JSTracer *trc, size_t len, HeapPtr<type*> *thing, const char *name);       \
void Mark##base##RootRange(JSTracer *trc, size_t len, type **thing, const char *name);            \
bool Is##base##Marked(type **thingp);                                                             \
bool Is##base##Marked(BarrieredBase<type*> *thingp);                                              \
bool Is##base##AboutToBeFinalized(type **thingp);                                                 \
bool Is##base##AboutToBeFinalized(BarrieredBase<type*> *thingp);                                  \
type *Update##base##IfRelocated(JSRuntime *rt, BarrieredBase<type*> *thingp);                     \
type *Update##base##IfRelocated(JSRuntime *rt, type **thingp);

DeclMarker(BaseShape, BaseShape)
DeclMarker(BaseShape, UnownedBaseShape)
DeclMarker(JitCode, jit::JitCode)
DeclMarker(Object, ArgumentsObject)
DeclMarker(Object, ArrayBufferObject)
DeclMarker(Object, ArrayBufferObjectMaybeShared)
DeclMarker(Object, ArrayBufferViewObject)
DeclMarker(Object, DebugScopeObject)
DeclMarker(Object, GlobalObject)
DeclMarker(Object, JSObject)
DeclMarker(Object, JSFunction)
DeclMarker(Object, NestedScopeObject)
DeclMarker(Object, SavedFrame)
DeclMarker(Object, ScopeObject)
DeclMarker(Object, SharedArrayBufferObject)
DeclMarker(Object, SharedTypedArrayObject)
DeclMarker(Script, JSScript)
DeclMarker(LazyScript, LazyScript)
DeclMarker(Shape, Shape)
DeclMarker(String, JSAtom)
DeclMarker(String, JSString)
DeclMarker(String, JSFlatString)
DeclMarker(String, JSLinearString)
DeclMarker(String, PropertyName)
DeclMarker(Symbol, JS::Symbol)
DeclMarker(TypeObject, types::TypeObject)

#undef DeclMarker

void
MarkPermanentAtom(JSTracer *trc, JSAtom *atom, const char *name);

void
MarkWellKnownSymbol(JSTracer *trc, JS::Symbol *sym);




MOZ_ALWAYS_INLINE bool
IsNullTaggedPointer(void *p)
{
    return uintptr_t(p) < 32;
}








void
MarkKind(JSTracer *trc, void **thingp, JSGCTraceKind kind);

void
MarkGCThingRoot(JSTracer *trc, void **thingp, const char *name);

void
MarkGCThingUnbarriered(JSTracer *trc, void **thingp, const char *name);



void
MarkId(JSTracer *trc, BarrieredBase<jsid> *id, const char *name);

void
MarkIdRoot(JSTracer *trc, jsid *id, const char *name);

void
MarkIdUnbarriered(JSTracer *trc, jsid *id, const char *name);

void
MarkIdRange(JSTracer *trc, size_t len, HeapId *vec, const char *name);

void
MarkIdRootRange(JSTracer *trc, size_t len, jsid *vec, const char *name);



void
MarkValue(JSTracer *trc, BarrieredBase<Value> *v, const char *name);

void
MarkValueRange(JSTracer *trc, size_t len, BarrieredBase<Value> *vec, const char *name);

inline void
MarkValueRange(JSTracer *trc, HeapValue *begin, HeapValue *end, const char *name)
{
    return MarkValueRange(trc, end - begin, begin, name);
}

void
MarkValueRoot(JSTracer *trc, Value *v, const char *name);

void
MarkThingOrValueUnbarriered(JSTracer *trc, uintptr_t *word, const char *name);

void
MarkValueRootRange(JSTracer *trc, size_t len, Value *vec, const char *name);

inline void
MarkValueRootRange(JSTracer *trc, Value *begin, Value *end, const char *name)
{
    MarkValueRootRange(trc, end - begin, begin, name);
}

void
MarkTypeRoot(JSTracer *trc, types::Type *v, const char *name);

bool
IsValueMarked(Value *v);

bool
IsValueAboutToBeFinalized(Value *v);



bool
IsSlotMarked(HeapSlot *s);

void
MarkSlot(JSTracer *trc, HeapSlot *s, const char *name);

void
MarkArraySlots(JSTracer *trc, size_t len, HeapSlot *vec, const char *name);

void
MarkObjectSlots(JSTracer *trc, JSObject *obj, uint32_t start, uint32_t nslots);

void
MarkCrossCompartmentObjectUnbarriered(JSTracer *trc, JSObject *src, JSObject **dst_obj,
                                      const char *name);

void
MarkCrossCompartmentScriptUnbarriered(JSTracer *trc, JSObject *src, JSScript **dst_script,
                                      const char *name);





void
MarkCrossCompartmentSlot(JSTracer *trc, JSObject *src, HeapSlot *dst_slot, const char *name);








void
MarkChildren(JSTracer *trc, JSObject *obj);






void
MarkCycleCollectorChildren(JSTracer *trc, Shape *shape);

void
PushArena(GCMarker *gcmarker, ArenaHeader *aheader);








inline void
Mark(JSTracer *trc, BarrieredBase<Value> *v, const char *name)
{
    MarkValue(trc, v, name);
}

inline void
Mark(JSTracer *trc, BarrieredBase<JSObject*> *o, const char *name)
{
    MarkObject(trc, o, name);
}

inline void
Mark(JSTracer *trc, BarrieredBase<JSScript*> *o, const char *name)
{
    MarkScript(trc, o, name);
}

inline void
Mark(JSTracer *trc, HeapPtrJitCode *code, const char *name)
{
    MarkJitCode(trc, code, name);
}


inline void
Mark(JSTracer *trc, JSObject **objp, const char *name)
{
    MarkObjectUnbarriered(trc, objp, name);
}


inline void
Mark(JSTracer *trc, ScopeObject **obj, const char *name)
{
    MarkObjectUnbarriered(trc, obj, name);
}

bool
IsCellMarked(Cell **thingp);

bool
IsCellAboutToBeFinalized(Cell **thing);

inline bool
IsMarked(BarrieredBase<Value> *v)
{
    if (!v->isMarkable())
        return true;
    return IsValueMarked(v->unsafeGet());
}

inline bool
IsMarked(BarrieredBase<JSObject*> *objp)
{
    return IsObjectMarked(objp);
}

inline bool
IsMarked(BarrieredBase<JSScript*> *scriptp)
{
    return IsScriptMarked(scriptp);
}

inline bool
IsAboutToBeFinalized(BarrieredBase<Value> *v)
{
    if (!v->isMarkable())
        return false;
    return IsValueAboutToBeFinalized(v->unsafeGet());
}

inline bool
IsAboutToBeFinalized(BarrieredBase<JSObject*> *objp)
{
    return IsObjectAboutToBeFinalized(objp);
}

inline bool
IsAboutToBeFinalized(BarrieredBase<JSScript*> *scriptp)
{
    return IsScriptAboutToBeFinalized(scriptp);
}

inline bool
IsAboutToBeFinalized(const js::jit::VMFunction **vmfunc)
{
    



    return false;
}

inline bool
IsAboutToBeFinalized(ReadBarrieredJitCode code)
{
    return IsJitCodeAboutToBeFinalized(code.unsafeGet());
}

inline Cell *
ToMarkable(const Value &v)
{
    if (v.isMarkable())
        return (Cell *)v.toGCThing();
    return nullptr;
}

inline Cell *
ToMarkable(Cell *cell)
{
    return cell;
}

inline JSGCTraceKind
TraceKind(const Value &v)
{
    JS_ASSERT(v.isMarkable());
    if (v.isObject())
        return JSTRACE_OBJECT;
    if (v.isString())
        return JSTRACE_STRING;
    JS_ASSERT(v.isSymbol());
    return JSTRACE_SYMBOL;
}

inline JSGCTraceKind
TraceKind(JSObject *obj)
{
    return JSTRACE_OBJECT;
}

inline JSGCTraceKind
TraceKind(JSScript *script)
{
    return JSTRACE_SCRIPT;
}

inline JSGCTraceKind
TraceKind(LazyScript *lazy)
{
    return JSTRACE_LAZY_SCRIPT;
}

} 

void
TraceChildren(JSTracer *trc, void *thing, JSGCTraceKind kind);

} 

#endif 
