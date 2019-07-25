



































#ifndef js_MemoryMetrics_h
#define js_MemoryMetrics_h






#include "jspubtd.h"

#include "js/Utility.h"

namespace JS {


struct TypeInferenceMemoryStats
{
    int64_t scripts;
    int64_t objects;
    int64_t tables;
    int64_t temporary;
};

extern JS_PUBLIC_API(void)
SizeOfCompartmentTypeInferenceData(JSContext *cx, JSCompartment *compartment,
                                   TypeInferenceMemoryStats *stats,
                                   JSMallocSizeOfFun mallocSizeOf);

extern JS_PUBLIC_API(void)
SizeOfObjectTypeInferenceData( void *object,
                              TypeInferenceMemoryStats *stats,
                              JSMallocSizeOfFun mallocSizeOf);

extern JS_PUBLIC_API(size_t)
SizeOfObjectDynamicSlots(JSObject *obj, JSMallocSizeOfFun mallocSizeOf);

extern JS_PUBLIC_API(size_t)
SizeOfCompartmentShapeTable(JSCompartment *c, JSMallocSizeOfFun mallocSizeOf);

extern JS_PUBLIC_API(size_t)
SizeOfCompartmentMjitCode(const JSCompartment *c);

extern JS_PUBLIC_API(bool)
IsShapeInDictionary(const void *shape);

extern JS_PUBLIC_API(size_t)
SizeOfShapePropertyTable(const void *shape, JSMallocSizeOfFun mallocSizeOf);

extern JS_PUBLIC_API(size_t)
SizeOfShapeKids(const void *shape, JSMallocSizeOfFun mallocSizeOf);

extern JS_PUBLIC_API(size_t)
SizeOfScriptData(JSScript *script, JSMallocSizeOfFun mallocSizeOf);

#ifdef JS_METHODJIT
extern JS_PUBLIC_API(size_t)
SizeOfScriptJitData(JSScript *script, JSMallocSizeOfFun mallocSizeOf);
#endif

extern JS_PUBLIC_API(size_t)
SystemCompartmentCount(const JSRuntime *rt);

extern JS_PUBLIC_API(size_t)
UserCompartmentCount(const JSRuntime *rt);

} 

#endif 
