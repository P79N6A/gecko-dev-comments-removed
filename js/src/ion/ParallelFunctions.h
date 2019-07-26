





#ifndef ion_ParallelFunctions_h
#define ion_ParallelFunctions_h

#include "vm/ThreadPool.h"
#include "vm/ForkJoin.h"
#include "gc/Heap.h"

namespace js {
namespace ion {

ForkJoinSlice *ForkJoinSlicePar();
JSObject *NewGCThingPar(gc::AllocKind allocKind);
bool IsThreadLocalObject(ForkJoinSlice *context, JSObject *object);
bool CheckOverRecursedPar(ForkJoinSlice *slice);
bool CheckInterruptPar(ForkJoinSlice *context);

void DumpValuePar(Value *v);




struct PushParArgs {
    JSObject *object;
    Value value;
};




JSObject *PushPar(PushParArgs *args);




JSObject *ExtendArrayPar(ForkJoinSlice *slice, JSObject *array, uint32_t length);



ParallelResult ConcatStringsPar(ForkJoinSlice *slice, HandleString left, HandleString right,
                                MutableHandleString out);
ParallelResult IntToStringPar(ForkJoinSlice *slice, int i, MutableHandleString out);
ParallelResult DoubleToStringPar(ForkJoinSlice *slice, double d, MutableHandleString out);



ParallelResult StrictlyEqualPar(ForkJoinSlice *slice, MutableHandleValue v1, MutableHandleValue v2, JSBool *);
ParallelResult StrictlyUnequalPar(ForkJoinSlice *slice, MutableHandleValue v1, MutableHandleValue v2, JSBool *);
ParallelResult LooselyEqualPar(ForkJoinSlice *slice, MutableHandleValue v1, MutableHandleValue v2, JSBool *);
ParallelResult LooselyUnequalPar(ForkJoinSlice *slice, MutableHandleValue v1, MutableHandleValue v2, JSBool *);
ParallelResult LessThanPar(ForkJoinSlice *slice, MutableHandleValue v1, MutableHandleValue v2, JSBool *);
ParallelResult LessThanOrEqualPar(ForkJoinSlice *slice, MutableHandleValue v1, MutableHandleValue v2, JSBool *);
ParallelResult GreaterThanPar(ForkJoinSlice *slice, MutableHandleValue v1, MutableHandleValue v2, JSBool *);
ParallelResult GreaterThanOrEqualPar(ForkJoinSlice *slice, MutableHandleValue v1, MutableHandleValue v2, JSBool *);

ParallelResult StringsEqualPar(ForkJoinSlice *slice, HandleString v1, HandleString v2, JSBool *);
ParallelResult StringsUnequalPar(ForkJoinSlice *slice, HandleString v1, HandleString v2, JSBool *);

ParallelResult InitRestParameterPar(ForkJoinSlice *slice, uint32_t length, Value *rest,
                                    HandleObject templateObj, HandleObject res,
                                    MutableHandleObject out);

void AbortPar(ParallelBailoutCause cause, JSScript *outermostScript, JSScript *currentScript,
              jsbytecode *bytecode);
void PropagateAbortPar(JSScript *outermostScript, JSScript *currentScript);

void TraceLIR(uint32_t bblock, uint32_t lir, uint32_t execModeInt,
              const char *lirOpName, const char *mirOpName,
              JSScript *script, jsbytecode *pc);

void CallToUncompiledScriptPar(JSFunction *func);

} 
} 

#endif 
