






































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

void
MarkAtom(JSTracer *trc, JSAtom *str);

void
MarkAtom(JSTracer *trc, JSAtom *str, const char *name);

void
MarkObjectUnbarriered(JSTracer *trc, JSObject *obj, const char *name);

void
MarkObject(JSTracer *trc, const MarkablePtr<JSObject> &obj, const char *name);

void
MarkStringUnbarriered(JSTracer *trc, JSString *str, const char *name);

void
MarkString(JSTracer *trc, const MarkablePtr<JSString> &str, const char *name);

void
MarkScriptUnbarriered(JSTracer *trc, JSScript *script, const char *name);

void
MarkScript(JSTracer *trc, const MarkablePtr<JSScript> &script, const char *name);

void
MarkShapeUnbarriered(JSTracer *trc, const Shape *shape, const char *name);

void
MarkShape(JSTracer *trc, const MarkablePtr<const Shape> &shape, const char *name);

void
MarkBaseShapeUnbarriered(JSTracer *trc, BaseShape *shape, const char *name);

void
MarkTypeObjectUnbarriered(JSTracer *trc, types::TypeObject *type, const char *name);

void
MarkTypeObject(JSTracer *trc, const MarkablePtr<types::TypeObject> &type, const char *name);

void
MarkXMLUnbarriered(JSTracer *trc, JSXML *xml, const char *name);

void
MarkXML(JSTracer *trc, const MarkablePtr<JSXML> &xml, const char *name);

void
MarkObjectRange(JSTracer *trc, size_t len, HeapPtr<JSObject> *vec, const char *name);

void
MarkXMLRange(JSTracer *trc, size_t len, HeapPtr<JSXML> *vec, const char *name);

void
MarkId(JSTracer *trc, const HeapId &id, const char *name);

void
MarkIdRange(JSTracer *trc, js::HeapId *beg, js::HeapId *end, const char *name);

void
MarkIdRangeUnbarriered(JSTracer *trc, size_t len, jsid *vec, const char *name);

void
MarkIdRangeUnbarriered(JSTracer *trc, jsid *beg, jsid *end, const char *name);

void
MarkKind(JSTracer *trc, void *thing, JSGCTraceKind kind);

void
MarkValueUnbarriered(JSTracer *trc, const js::Value &v, const char *name);

void
MarkValue(JSTracer *trc, const js::HeapValue &v, const char *name);





void
MarkCrossCompartmentValue(JSTracer *trc, const js::HeapValue &v, const char *name);

void
MarkValueRange(JSTracer *trc, const HeapValue *beg, const HeapValue *end, const char *name);

void
MarkValueRange(JSTracer *trc, size_t len, const HeapValue *vec, const char *name);

void
MarkRoot(JSTracer *trc, JSObject *thing, const char *name);

void
MarkRoot(JSTracer *trc, JSString *thing, const char *name);

void
MarkRoot(JSTracer *trc, JSScript *thing, const char *name);

void
MarkRoot(JSTracer *trc, const Shape *thing, const char *name);

void
MarkRoot(JSTracer *trc, types::TypeObject *thing, const char *name);

void
MarkRoot(JSTracer *trc, JSXML *thing, const char *name);

void
MarkRoot(JSTracer *trc, const Value &v, const char *name);

void
MarkRoot(JSTracer *trc, jsid id, const char *name);

void
MarkRootGCThing(JSTracer *trc, void *thing, const char *name);

void
MarkRootRange(JSTracer *trc, size_t len, const Shape **vec, const char *name);

void
MarkRootRange(JSTracer *trc, size_t len, JSObject **vec, const char *name);

void
MarkRootRange(JSTracer *trc, const Value *beg, const Value *end, const char *name);

void
MarkRootRange(JSTracer *trc, size_t len, const Value *vec, const char *name);

void
MarkRootRange(JSTracer *trc, jsid *beg, jsid *end, const char *name);

void
MarkRootRange(JSTracer *trc, size_t len, jsid *vec, const char *name);

void
MarkChildren(JSTracer *trc, JSObject *obj);

void
MarkChildren(JSTracer *trc, JSString *str);

void
MarkChildren(JSTracer *trc, const Shape *shape);

void
MarkChildren(JSTracer *trc, JSScript *script);

void
MarkChildren(JSTracer *trc, JSXML *xml);






inline void
Mark(JSTracer *trc, const js::HeapValue &v, const char *name)
{
    MarkValue(trc, v, name);
}

inline void
Mark(JSTracer *trc, const MarkablePtr<JSObject> &o, const char *name)
{
    MarkObject(trc, o, name);
}

inline bool
IsMarked(JSContext *cx, const js::Value &v)
{
    if (v.isMarkable())
        return !IsAboutToBeFinalized(cx, v);
    return true;
}

inline bool
IsMarked(JSContext *cx, JSObject *o)
{
    return !IsAboutToBeFinalized(cx, o);
}

inline bool
IsMarked(JSContext *cx, Cell *cell)
{
    return !IsAboutToBeFinalized(cx, cell);
}

} 

void
TraceChildren(JSTracer *trc, void *thing, JSGCTraceKind kind);

void
CallTracer(JSTracer *trc, void *thing, JSGCTraceKind kind);

} 

#endif
