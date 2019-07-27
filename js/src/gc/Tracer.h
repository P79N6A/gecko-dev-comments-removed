





#ifndef js_Tracer_h
#define js_Tracer_h

#include "jsfriendapi.h"

#include "gc/Barrier.h"

namespace js {







































template <typename T>
void
TraceEdge(JSTracer* trc, BarrieredBase<T>* thingp, const char* name);




template <typename T>
void
TraceRoot(JSTracer* trc, T* thingp, const char* name);




template <typename T>
void
TraceManuallyBarrieredEdge(JSTracer* trc, T* thingp, const char* name);


template <typename T>
void
TraceRange(JSTracer* trc, size_t len, BarrieredBase<T>* vec, const char* name);


template <typename T>
void
TraceRootRange(JSTracer* trc, size_t len, T* vec, const char* name);



template <typename T>
void
TraceCrossCompartmentEdge(JSTracer* trc, JSObject* src, BarrieredBase<T>* dst,
                          const char* name);


template <typename T>
void
TraceManuallyBarrieredCrossCompartmentEdge(JSTracer* trc, JSObject* src, T* dst,
                                           const char* name);




template <typename T>
void
TraceProcessGlobalRoot(JSTracer* trc, T* thing, const char* name);



void
TraceGenericPointerRoot(JSTracer* trc, gc::Cell** thingp, const char* name);



void
TraceManuallyBarrieredGenericPointerEdge(JSTracer* trc, gc::Cell** thingp, const char* name);


void
TraceChildren(JSTracer* trc, void* thing, JS::TraceKind kind);

namespace gc {



void
TraceCycleCollectorChildren(JS::CallbackTracer* trc, Shape* shape);
void
TraceCycleCollectorChildren(JS::CallbackTracer* trc, ObjectGroup* group);

} 

} 

#endif 
