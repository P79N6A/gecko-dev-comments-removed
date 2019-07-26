





#ifndef jsgcmark_h___
#define jsgcmark_h___

#include "jsgc.h"
#include "jscntxt.h"
#include "jscompartment.h"
#include "jslock.h"

#include "gc/Barrier.h"
#include "js/TemplateLib.h"

#include "ion/IonCode.h"

namespace js {
namespace gc {



























#define DeclMarker(base, type)                                                                    \
void Mark##base(JSTracer *trc, HeapPtr<type> *thing, const char *name);                           \
void Mark##base##Root(JSTracer *trc, type **thingp, const char *name);                            \
void Mark##base##Unbarriered(JSTracer *trc, type **thingp, const char *name);                     \
void Mark##base##Range(JSTracer *trc, size_t len, HeapPtr<type> *thing, const char *name);        \
void Mark##base##RootRange(JSTracer *trc, size_t len, type **thing, const char *name);

DeclMarker(BaseShape, BaseShape)
DeclMarker(BaseShape, UnownedBaseShape)
DeclMarker(Object, ArgumentsObject)
DeclMarker(Object, GlobalObject)
DeclMarker(Object, JSObject)
DeclMarker(Object, JSFunction)
DeclMarker(Script, JSScript)
DeclMarker(Shape, Shape)
DeclMarker(String, JSAtom)
DeclMarker(String, JSString)
DeclMarker(String, JSFlatString)
DeclMarker(String, JSLinearString)
DeclMarker(TypeObject, types::TypeObject)
DeclMarker(IonCode, ion::IonCode)
#if JS_HAS_XML_SUPPORT
DeclMarker(XML, JSXML)
#endif








void
MarkKind(JSTracer *trc, void **thingp, JSGCTraceKind kind);

void
MarkGCThingRoot(JSTracer *trc, void **thingp, const char *name);



void
MarkId(JSTracer *trc, HeapId *id, const char *name);

void
MarkIdRoot(JSTracer *trc, jsid *id, const char *name);

void
MarkIdRange(JSTracer *trc, size_t len, js::HeapId *vec, const char *name);

void
MarkIdRootRange(JSTracer *trc, size_t len, jsid *vec, const char *name);



void
MarkValue(JSTracer *trc, HeapValue *v, const char *name);

void
MarkValueRange(JSTracer *trc, size_t len, HeapValue *vec, const char *name);

void
MarkValueRoot(JSTracer *trc, Value *v, const char *name);

void
MarkThingOrValueUnbarriered(JSTracer *trc, uintptr_t *word, const char *name);

void
MarkThingOrValueRoot(JSTracer *trc, uintptr_t *word, const char *name);

void
MarkValueRootRange(JSTracer *trc, size_t len, Value *vec, const char *name);

inline void
MarkValueRootRange(JSTracer *trc, Value *begin, Value *end, const char *name)
{
    MarkValueRootRange(trc, end - begin, begin, name);
}



void
MarkSlot(JSTracer *trc, HeapSlot *s, const char *name);

void
MarkArraySlots(JSTracer *trc, size_t len, HeapSlot *vec, const char *name);

void
MarkObjectSlots(JSTracer *trc, JSObject *obj, uint32_t start, uint32_t nslots);

void
MarkCrossCompartmentObjectUnbarriered(JSTracer *trc, JSObject **obj, const char *name);

void
MarkCrossCompartmentScriptUnbarriered(JSTracer *trc, JSScript **script, const char *name);





void
MarkCrossCompartmentSlot(JSTracer *trc, HeapSlot *s, const char *name);








void
MarkObject(JSTracer *trc, HeapPtr<GlobalObject, JSScript *> *thingp, const char *name);


void
MarkValueUnbarriered(JSTracer *trc, Value *v, const char *name);





void
MarkChildren(JSTracer *trc, JSObject *obj);






void
MarkCycleCollectorChildren(JSTracer *trc, Shape *shape);

void
PushArena(GCMarker *gcmarker, ArenaHeader *aheader);








inline void
Mark(JSTracer *trc, HeapValue *v, const char *name)
{
    MarkValue(trc, v, name);
}

inline void
Mark(JSTracer *trc, HeapPtr<JSObject> *o, const char *name)
{
    MarkObject(trc, o, name);
}

inline void
Mark(JSTracer *trc, HeapPtr<JSScript> *o, const char *name)
{
    MarkScript(trc, o, name);
}

inline void
Mark(JSTracer *trc, HeapPtr<JSXML> *xml, const char *name)
{
    MarkXML(trc, xml, name);
}

inline void
Mark(JSTracer *trc, HeapPtr<ion::IonCode> *code, const char *name)
{
    MarkIonCode(trc, code, name);
}

inline bool
IsMarked(const Value &v)
{
    if (v.isMarkable())
        return !IsAboutToBeFinalized(v);
    return true;
}

inline bool
IsMarked(JSContext *cx, ion::IonCode *code)
{
    return !IsAboutToBeFinalized(code);
}

inline bool
IsMarked(Cell *cell)
{
    return !IsAboutToBeFinalized(cell);
}

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
