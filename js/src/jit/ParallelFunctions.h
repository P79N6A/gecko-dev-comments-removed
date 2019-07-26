





#ifndef jit_ParallelFunctions_h
#define jit_ParallelFunctions_h

#include "gc/Heap.h"
#include "vm/ForkJoin.h"

namespace js {
namespace jit {

ForkJoinSlice *ForkJoinSlicePar();
JSObject *NewGCThingPar(gc::AllocKind allocKind);
bool IsThreadLocalObject(ForkJoinSlice *context, JSObject *object);
bool CheckOverRecursedPar(ForkJoinSlice *slice);
bool CheckInterruptPar(ForkJoinSlice *context);




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
ParallelResult StringToNumberPar(ForkJoinSlice *slice, JSString *str, double *out);



ParallelResult StrictlyEqualPar(ForkJoinSlice *slice, MutableHandleValue v1, MutableHandleValue v2, bool *);
ParallelResult StrictlyUnequalPar(ForkJoinSlice *slice, MutableHandleValue v1, MutableHandleValue v2, bool *);
ParallelResult LooselyEqualPar(ForkJoinSlice *slice, MutableHandleValue v1, MutableHandleValue v2, bool *);
ParallelResult LooselyUnequalPar(ForkJoinSlice *slice, MutableHandleValue v1, MutableHandleValue v2, bool *);
ParallelResult LessThanPar(ForkJoinSlice *slice, MutableHandleValue v1, MutableHandleValue v2, bool *);
ParallelResult LessThanOrEqualPar(ForkJoinSlice *slice, MutableHandleValue v1, MutableHandleValue v2, bool *);
ParallelResult GreaterThanPar(ForkJoinSlice *slice, MutableHandleValue v1, MutableHandleValue v2, bool *);
ParallelResult GreaterThanOrEqualPar(ForkJoinSlice *slice, MutableHandleValue v1, MutableHandleValue v2, bool *);

ParallelResult StringsEqualPar(ForkJoinSlice *slice, HandleString v1, HandleString v2, bool *);
ParallelResult StringsUnequalPar(ForkJoinSlice *slice, HandleString v1, HandleString v2, bool *);

ParallelResult BitNotPar(ForkJoinSlice *slice, HandleValue in, int32_t *out);
ParallelResult BitXorPar(ForkJoinSlice *slice, HandleValue lhs, HandleValue rhs, int32_t *out);
ParallelResult BitOrPar(ForkJoinSlice *slice, HandleValue lhs, HandleValue rhs, int32_t *out);
ParallelResult BitAndPar(ForkJoinSlice *slice, HandleValue lhs, HandleValue rhs, int32_t *out);
ParallelResult BitLshPar(ForkJoinSlice *slice, HandleValue lhs, HandleValue rhs, int32_t *out);
ParallelResult BitRshPar(ForkJoinSlice *slice, HandleValue lhs, HandleValue rhs, int32_t *out);

ParallelResult UrshValuesPar(ForkJoinSlice *slice, HandleValue lhs, HandleValue rhs,
                             Value *out);


ParallelResult InitRestParameterPar(ForkJoinSlice *slice, uint32_t length, Value *rest,
                                    HandleObject templateObj, HandleObject res,
                                    MutableHandleObject out);


void AbortPar(ParallelBailoutCause cause, JSScript *outermostScript, JSScript *currentScript,
              jsbytecode *bytecode);
void PropagateAbortPar(JSScript *outermostScript, JSScript *currentScript);

void TraceLIR(IonLIRTraceData *current);

void CallToUncompiledScriptPar(JSObject *obj);

} 
} 

#endif 
