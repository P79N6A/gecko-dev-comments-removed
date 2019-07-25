






































#ifndef jsgcmark_h___
#define jsgcmark_h___

#include "jsgc.h"
#include "jscntxt.h"
#include "jscompartment.h"

#include "jslock.h"
#include "jstl.h"

namespace js {
namespace gc {

template<typename T>
void Mark(JSTracer *trc, T *thing);

void
MarkString(JSTracer *trc, JSString *str);

void
MarkString(JSTracer *trc, JSString *str, const char *name);

void
MarkObject(JSTracer *trc, JSObject &obj, const char *name);

void
MarkObjectWithPrinter(JSTracer *trc, JSObject &obj, JSTraceNamePrinter printer,
		      const void *arg, size_t index);

void
MarkShape(JSTracer *trc, const Shape *shape, const char *name);

void
MarkXML(JSTracer *trc, JSXML *xml, const char *name);

void
MarkAtomRange(JSTracer *trc, size_t len, JSAtom **vec, const char *name);

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
MarkKind(JSTracer *trc, void *thing, uint32 kind);

void
MarkValueRaw(JSTracer *trc, const js::Value &v);

void
MarkValue(JSTracer *trc, const js::Value &v, const char *name);

void
MarkValueRange(JSTracer *trc, Value *beg, Value *end, const char *name);

void
MarkValueRange(JSTracer *trc, size_t len, Value *vec, const char *name);

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
MarkRoot(JSTracer *trc, const Shape *thing, const char *name);

void
MarkRoot(JSTracer *trc, JSXML *thing, const char *name);

void
MarkChildren(JSTracer *trc, JSObject *obj);

void
MarkChildren(JSTracer *trc, JSString *str);

void
MarkChildren(JSTracer *trc, const Shape *shape);

void
MarkChildren(JSTracer *trc, JSXML *xml);

}
}

#endif
