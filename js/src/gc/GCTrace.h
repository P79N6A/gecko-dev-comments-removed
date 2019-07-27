





#ifndef gc_GCTrace_h
#define gc_GCTrace_h

#include "gc/Heap.h"

namespace js {

namespace types { struct TypeObject; }

namespace gc {

#ifdef JS_GC_TRACE

extern bool InitTrace(GCRuntime &gc);
extern void FinishTrace();
extern bool TraceEnabled();
extern void TraceNurseryAlloc(Cell *thing, size_t size);
extern void TraceTenuredAlloc(Cell *thing, AllocKind kind);
extern void TraceCreateObject(JSObject* object);
extern void TraceMinorGCStart();
extern void TracePromoteToTenured(Cell *src, Cell *dst);
extern void TraceMinorGCEnd();
extern void TraceMajorGCStart();
extern void TraceTenuredFinalize(Cell *thing);
extern void TraceMajorGCEnd();

#else

inline bool InitTrace(GCRuntime &gc) { return true; }
inline void FinishTrace() {}
inline bool TraceEnabled() { return false; }
inline void TraceNurseryAlloc(Cell *thing, size_t size) {}
inline void TraceTenuredAlloc(Cell *thing, AllocKind kind) {}
inline void TraceCreateObject(JSObject* object) {}
inline void TraceMinorGCStart() {}
inline void TracePromoteToTenured(Cell *src, Cell *dst) {}
inline void TraceMinorGCEnd() {}
inline void TraceMajorGCStart() {}
inline void TraceTenuredFinalize(Cell *thing) {}
inline void TraceMajorGCEnd() {}

#endif

} 
} 

#endif
