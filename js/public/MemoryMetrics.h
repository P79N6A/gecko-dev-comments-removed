



































#ifndef js_MemoryMetrics_h
#define js_MemoryMetrics_h






#include <string.h>

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

typedef void (* DestroyNameCallback)(void *string);

struct CompartmentStats
{
    CompartmentStats()
    {
        memset(this, 0, sizeof(*this));
    }

    void init(void *name_, DestroyNameCallback destroyName)
    {
        name = name_;
        destroyNameCb = destroyName;
    }

    ~CompartmentStats()
    {
        destroyNameCb(name);
    }

    
    void *name;
    DestroyNameCallback destroyNameCb;

    int64_t gcHeapArenaHeaders;
    int64_t gcHeapArenaPadding;
    int64_t gcHeapArenaUnused;

    int64_t gcHeapObjectsNonFunction;
    int64_t gcHeapObjectsFunction;
    int64_t gcHeapStrings;
    int64_t gcHeapShapesTree;
    int64_t gcHeapShapesDict;
    int64_t gcHeapShapesBase;
    int64_t gcHeapScripts;
    int64_t gcHeapTypeObjects;
    int64_t gcHeapXML;

    int64_t objectSlots;
    int64_t stringChars;
    int64_t shapesExtraTreeTables;
    int64_t shapesExtraDictTables;
    int64_t shapesExtraTreeShapeKids;
    int64_t shapesCompartmentTables;
    int64_t scriptData;

#ifdef JS_METHODJIT
    int64_t mjitCode;
    int64_t mjitData;
#endif
    TypeInferenceMemoryStats typeInferenceMemory;
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
