





#ifndef jsion_parallel_functions_h__
#define jsion_parallel_functions_h__

#include "vm/ThreadPool.h"
#include "vm/ForkJoin.h"
#include "gc/Heap.h"

namespace js {
namespace ion {

ForkJoinSlice *ParForkJoinSlice();
JSObject *ParNewGCThing(gc::AllocKind allocKind);
bool ParWriteGuard(ForkJoinSlice *context, JSObject *object);
void ParBailout(uint32_t id);
bool ParCheckOverRecursed(ForkJoinSlice *slice);
bool ParCheckInterrupt(ForkJoinSlice *context);

void ParDumpValue(Value *v);




struct ParPushArgs {
    JSObject *object;
    Value value;
};




JSObject* ParPush(ParPushArgs *args);




JSObject *ParExtendArray(ForkJoinSlice *slice, JSObject *array, uint32_t length);



ParallelResult ParStrictlyEqual(ForkJoinSlice *slice, MutableHandleValue v1, MutableHandleValue v2, JSBool *);
ParallelResult ParStrictlyUnequal(ForkJoinSlice *slice, MutableHandleValue v1, MutableHandleValue v2, JSBool *);
ParallelResult ParLooselyEqual(ForkJoinSlice *slice, MutableHandleValue v1, MutableHandleValue v2, JSBool *);
ParallelResult ParLooselyUnequal(ForkJoinSlice *slice, MutableHandleValue v1, MutableHandleValue v2, JSBool *);
ParallelResult ParLessThan(ForkJoinSlice *slice, MutableHandleValue v1, MutableHandleValue v2, JSBool *);
ParallelResult ParLessThanOrEqual(ForkJoinSlice *slice, MutableHandleValue v1, MutableHandleValue v2, JSBool *);
ParallelResult ParGreaterThan(ForkJoinSlice *slice, MutableHandleValue v1, MutableHandleValue v2, JSBool *);
ParallelResult ParGreaterThanOrEqual(ForkJoinSlice *slice, MutableHandleValue v1, MutableHandleValue v2, JSBool *);

ParallelResult ParStringsEqual(ForkJoinSlice *slice, HandleString v1, HandleString v2, JSBool *);
ParallelResult ParStringsUnequal(ForkJoinSlice *slice, HandleString v1, HandleString v2, JSBool *);

void ParallelAbort(JSScript *script);

void TraceLIR(uint32_t bblock, uint32_t lir, uint32_t execModeInt,
              const char *lirOpName, const char *mirOpName,
              JSScript *script, jsbytecode *pc);

void ParCallToUncompiledScript(JSFunction *func);

} 
} 

#endif 
