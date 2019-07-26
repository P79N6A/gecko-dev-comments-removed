






#ifndef gc_marking_h___
#define gc_marking_h___

#include "jsgc.h"
#include "jscntxt.h"
#include "jslock.h"

#include "gc/Barrier.h"
#include "js/TemplateLib.h"
#include "ion/IonCode.h"

extern "C" {
struct JSContext;
class JSFunction;
class JSObject;
class JSScript;
}

class JSAtom;
class JSLinearString;

namespace js {

class ArgumentsObject;
class BaseShape;
class GlobalObject;
class UnownedBaseShape;
class Shape;

template<class, typename> class HeapPtr;

namespace gc {









































#define DeclMarker(base, type)                                                                    \
void Mark##base(JSTracer *trc, EncapsulatedPtr<type> *thing, const char *name);                   \
void Mark##base##Root(JSTracer *trc, type **thingp, const char *name);                            \
void Mark##base##Unbarriered(JSTracer *trc, type **thingp, const char *name);                     \
void Mark##base##Range(JSTracer *trc, size_t len, HeapPtr<type> *thing, const char *name);        \
void Mark##base##RootRange(JSTracer *trc, size_t len, type **thing, const char *name);            \
bool Is##base##Marked(type **thingp);                                                             \
bool Is##base##Marked(EncapsulatedPtr<type> *thingp);                                             \
bool Is##base##AboutToBeFinalized(type **thingp);                                                 \
bool Is##base##AboutToBeFinalized(EncapsulatedPtr<type> *thingp);

DeclMarker(BaseShape, BaseShape)
DeclMarker(BaseShape, UnownedBaseShape)
DeclMarker(IonCode, ion::IonCode)
DeclMarker(Object, ArgumentsObject)
DeclMarker(Object, DebugScopeObject)
DeclMarker(Object, GlobalObject)
DeclMarker(Object, JSObject)
DeclMarker(Object, JSFunction)
DeclMarker(Object, ScopeObject)
DeclMarker(Script, JSScript)
DeclMarker(Shape, Shape)
DeclMarker(String, JSAtom)
DeclMarker(String, JSString)
DeclMarker(String, JSFlatString)
DeclMarker(String, JSLinearString)
DeclMarker(String, PropertyName)
DeclMarker(TypeObject, types::TypeObject)

#undef DeclMarker


JS_ALWAYS_INLINE bool
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
MarkId(JSTracer *trc, EncapsulatedId *id, const char *name);

void
MarkIdRoot(JSTracer *trc, jsid *id, const char *name);

void
MarkIdUnbarriered(JSTracer *trc, jsid *id, const char *name);

void
MarkIdRange(JSTracer *trc, size_t len, HeapId *vec, const char *name);

void
MarkIdRootRange(JSTracer *trc, size_t len, jsid *vec, const char *name);



void
MarkValue(JSTracer *trc, EncapsulatedValue *v, const char *name);

void
MarkValueRange(JSTracer *trc, size_t len, EncapsulatedValue *vec, const char *name);

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
MarkValueRootRangeMaybeNullPayload(JSTracer *trc, size_t len, Value *vec, const char *name);

void
MarkTypeRoot(JSTracer *trc, types::Type *v, const char *name);

bool
IsValueMarked(Value *v);

bool
IsValueAboutToBeFinalized(Value *v);



void
MarkSlot(JSTracer *trc, HeapSlot *s, const char *name);

void
MarkArraySlots(JSTracer *trc, size_t len, HeapSlot *vec, const char *name);

void
MarkObjectSlots(JSTracer *trc, JSObject *obj, uint32_t start, uint32_t nslots);

void
MarkCrossCompartmentObjectUnbarriered(JSTracer *trc, RawObject src, JSObject **dst_obj,
                                      const char *name);

void
MarkCrossCompartmentScriptUnbarriered(JSTracer *trc, RawObject src, JSScript **dst_script,
                                      const char *name);





void
MarkCrossCompartmentSlot(JSTracer *trc, RawObject src, HeapSlot *dst_slot, const char *name);








void
MarkObject(JSTracer *trc, HeapPtr<GlobalObject, JSScript *> *thingp, const char *name);


void
MarkValueUnbarriered(JSTracer *trc, Value *v, const char *name);





void
MarkChildren(JSTracer *trc, JSObject *obj);






void
MarkCycleCollectorChildren(JSTracer *trc, UnrootedShape shape);

void
PushArena(GCMarker *gcmarker, ArenaHeader *aheader);








inline void
Mark(JSTracer *trc, EncapsulatedValue *v, const char *name)
{
    MarkValue(trc, v, name);
}

inline void
Mark(JSTracer *trc, EncapsulatedPtrObject *o, const char *name)
{
    MarkObject(trc, o, name);
}

inline void
Mark(JSTracer *trc, EncapsulatedPtrScript *o, const char *name)
{
    MarkScript(trc, o, name);
}

inline void
Mark(JSTracer *trc, HeapPtr<ion::IonCode> *code, const char *name)
{
    MarkIonCode(trc, code, name);
}

bool
IsCellMarked(Cell **thingp);

bool
IsCellAboutToBeFinalized(Cell **thing);

inline bool
IsMarked(EncapsulatedValue *v)
{
    if (!v->isMarkable())
        return true;
    return IsValueMarked(v->unsafeGet());
}

inline bool
IsMarked(EncapsulatedPtrObject *objp)
{
    return IsObjectMarked(objp);
}

inline bool
IsMarked(EncapsulatedPtrScript *scriptp)
{
    return IsScriptMarked(scriptp);
}

inline bool
IsAboutToBeFinalized(EncapsulatedValue *v)
{
    if (!v->isMarkable())
        return false;
    return IsValueAboutToBeFinalized(v->unsafeGet());
}

inline bool
IsAboutToBeFinalized(EncapsulatedPtrObject *objp)
{
    return IsObjectAboutToBeFinalized(objp);
}

inline bool
IsAboutToBeFinalized(EncapsulatedPtrScript *scriptp)
{
    return IsScriptAboutToBeFinalized(scriptp);
}

#ifdef JS_ION


inline bool
IsAboutToBeFinalized(const js::ion::VMFunction **vmfunc)
{
    



    return true;
}

inline bool
IsAboutToBeFinalized(ReadBarriered<js::ion::IonCode> code)
{
    return IsIonCodeAboutToBeFinalized(code.unsafeGet());
}
#endif

inline Cell *
ToMarkable(const Value &v)
{
    if (v.isMarkable())
        return (Cell *)v.toGCThing();
    return NULL;
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
    return JSTRACE_STRING;
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

} 

void
TraceChildren(JSTracer *trc, void *thing, JSGCTraceKind kind);

void
CallTracer(JSTracer *trc, void *thing, JSGCTraceKind kind);

} 

#endif 
