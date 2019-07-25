





#ifndef jsgcmark_h___
#define jsgcmark_h___

#include "jsgc.h"
#include "jscntxt.h"
#include "jscompartment.h"
#include "jslock.h"

#include "gc/Barrier.h"
#include "js/TemplateLib.h"

namespace js {
namespace gc {



























#define DeclMarker(base, type)                                                                    \
void Mark##base(JSTracer *trc, const HeapPtr<type> &thing, const char *name);                     \
void Mark##base##Root(JSTracer *trc, type *thing, const char *name);                              \
void Mark##base##Unbarriered(JSTracer *trc, type *thing, const char *name);                       \
void Mark##base##Range(JSTracer *trc, size_t len, HeapPtr<type> *thing, const char *name);        \
void Mark##base##RootRange(JSTracer *trc, size_t len, type **thing, const char *name);

DeclMarker(BaseShape, BaseShape)
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
#if JS_HAS_XML_SUPPORT
DeclMarker(XML, JSXML)
#endif








void
MarkKind(JSTracer *trc, void *thing, JSGCTraceKind kind);

void
MarkGCThingRoot(JSTracer *trc, void *thing, const char *name);



void
MarkId(JSTracer *trc, const HeapId &id, const char *name);

void
MarkIdRoot(JSTracer *trc, const jsid &id, const char *name);

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
MarkValueRootRange(JSTracer *trc, size_t len, Value *vec, const char *name);

inline void
MarkValueRootRange(JSTracer *trc, Value *begin, Value *end, const char *name)
{
    MarkValueRootRange(trc, end - begin, begin, name);
}




void
MarkShape(JSTracer *trc, const HeapPtr<const Shape> &thing, const char *name);


void
MarkValueUnbarriered(JSTracer *trc, Value *v, const char *name);





void
MarkCrossCompartmentValue(JSTracer *trc, HeapValue *v, const char *name);





void
MarkChildren(JSTracer *trc, JSObject *obj);






void
MarkCycleCollectorChildren(JSTracer *trc, const Shape *shape);







inline void
Mark(JSTracer *trc, HeapValue *v, const char *name)
{
    MarkValue(trc, v, name);
}

inline void
Mark(JSTracer *trc, const HeapPtr<JSObject> &o, const char *name)
{
    MarkObject(trc, o, name);
}

inline void
Mark(JSTracer *trc, const HeapPtr<JSXML> &xml, const char *name)
{
    MarkXML(trc, xml, name);
}

inline bool
IsMarked(const Value &v)
{
    if (v.isMarkable())
        return !IsAboutToBeFinalized(v);
    return true;
}

inline bool
IsMarked(Cell *cell)
{
    return !IsAboutToBeFinalized(cell);
}

} 

void
TraceChildren(JSTracer *trc, void *thing, JSGCTraceKind kind);

void
CallTracer(JSTracer *trc, void *thing, JSGCTraceKind kind);

} 

#endif
