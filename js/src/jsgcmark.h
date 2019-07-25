






































#ifndef jsgcmark_h___
#define jsgcmark_h___

#include "jsgc.h"
#include "jscntxt.h"
#include "jscompartment.h"
#include "jslock.h"


#include "js/TemplateLib.h"

namespace js {
namespace gc {

void
MarkString(JSTracer *trc, JSString *str);

void
MarkString(JSTracer *trc, JSString *str, const char *name);

void
MarkObject(JSTracer *trc, JSObject &obj, const char *name);





void
MarkCrossCompartmentObject(JSTracer *trc, JSObject &obj, const char *name);

void
MarkObjectWithPrinter(JSTracer *trc, JSObject &obj, JSTraceNamePrinter printer,
		      const void *arg, size_t index);

void
MarkScript(JSTracer *trc, JSScript *script, const char *name);

void
MarkShape(JSTracer *trc, const Shape *shape, const char *name);

void
MarkTypeObject(JSTracer *trc, types::TypeObject *type, const char *name);

void
MarkXML(JSTracer *trc, JSXML *xml, const char *name);

void
MarkObjectRange(JSTracer *trc, size_t len, JSObject **vec, const char *name);

void
MarkXMLRange(JSTracer *trc, size_t len, JSXML **vec, const char *name);

void
MarkId(JSTracer *trc, jsid id);

void
MarkId(JSTracer *trc, jsid id, const char *name);

void
MarkIdRange(JSTracer *trc, jsid *beg, jsid *end, const char *name);

void
MarkIdRange(JSTracer *trc, size_t len, jsid *vec, const char *name);

void
MarkKind(JSTracer *trc, void *thing, JSGCTraceKind kind);

void
MarkValueRaw(JSTracer *trc, const js::Value &v);

void
MarkValue(JSTracer *trc, const js::Value &v, const char *name);





void
MarkCrossCompartmentValue(JSTracer *trc, const js::Value &v, const char *name);

void
MarkValueRange(JSTracer *trc, const Value *beg, const Value *end, const char *name);

void
MarkValueRange(JSTracer *trc, size_t len, const Value *vec, const char *name);

void
MarkShapeRange(JSTracer *trc, const Shape **beg, const Shape **end, const char *name);

void
MarkShapeRange(JSTracer *trc, size_t len, const Shape **vec, const char *name);


void
MarkGCThing(JSTracer *trc, void *thing, uint32 kind);

void
MarkGCThing(JSTracer *trc, void *thing);

void
MarkGCThing(JSTracer *trc, void *thing, const char *name);

void
MarkGCThing(JSTracer *trc, void *thing, const char *name, size_t index);

void
Mark(JSTracer *trc, void *thing, uint32 kind, const char *name);

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
Mark(JSTracer *trc, const js::Value &v, const char *name)
{
    MarkValue(trc, v, name);
}

inline void
Mark(JSTracer *trc, JSObject *o, const char *name)
{
    MarkObject(trc, *o, name);
}

inline bool
IsMarked(JSContext *cx, const js::Value &v)
{
    if (v.isMarkable())
        return !IsAboutToBeFinalized(cx, v.toGCThing());
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
}

#endif
