





#ifndef jit_ParallelFunctions_h
#define jit_ParallelFunctions_h

#include "gc/Heap.h"
#include "vm/ForkJoin.h"

namespace js {
namespace jit {

ForkJoinSlice *ForkJoinSlicePar();
JSObject *NewGCThingPar(ForkJoinSlice *slice, gc::AllocKind allocKind);
bool IsThreadLocalObject(ForkJoinSlice *slice, JSObject *object);
bool CheckOverRecursedPar(ForkJoinSlice *slice);
bool CheckInterruptPar(ForkJoinSlice *slice);




JSObject *ExtendArrayPar(ForkJoinSlice *slice, JSObject *array, uint32_t length);


bool SetPropertyPar(ForkJoinSlice *slice, HandleObject obj, HandlePropertyName name,
                    HandleValue value, bool strict, jsbytecode *pc);
bool SetElementPar(ForkJoinSlice *slice, HandleObject obj, HandleValue index,
                   HandleValue value, bool strict);



JSString *ConcatStringsPar(ForkJoinSlice *slice, HandleString left, HandleString right);
JSFlatString *IntToStringPar(ForkJoinSlice *slice, int i);
JSString *DoubleToStringPar(ForkJoinSlice *slice, double d);
bool StringToNumberPar(ForkJoinSlice *slice, JSString *str, double *out);



bool StrictlyEqualPar(ForkJoinSlice *slice, MutableHandleValue v1, MutableHandleValue v2, bool *);
bool StrictlyUnequalPar(ForkJoinSlice *slice, MutableHandleValue v1, MutableHandleValue v2, bool *);
bool LooselyEqualPar(ForkJoinSlice *slice, MutableHandleValue v1, MutableHandleValue v2, bool *);
bool LooselyUnequalPar(ForkJoinSlice *slice, MutableHandleValue v1, MutableHandleValue v2, bool *);
bool LessThanPar(ForkJoinSlice *slice, MutableHandleValue v1, MutableHandleValue v2, bool *);
bool LessThanOrEqualPar(ForkJoinSlice *slice, MutableHandleValue v1, MutableHandleValue v2, bool *);
bool GreaterThanPar(ForkJoinSlice *slice, MutableHandleValue v1, MutableHandleValue v2, bool *);
bool GreaterThanOrEqualPar(ForkJoinSlice *slice, MutableHandleValue v1, MutableHandleValue v2, bool *);

bool StringsEqualPar(ForkJoinSlice *slice, HandleString v1, HandleString v2, bool *);
bool StringsUnequalPar(ForkJoinSlice *slice, HandleString v1, HandleString v2, bool *);

bool BitNotPar(ForkJoinSlice *slice, HandleValue in, int32_t *out);
bool BitXorPar(ForkJoinSlice *slice, HandleValue lhs, HandleValue rhs, int32_t *out);
bool BitOrPar(ForkJoinSlice *slice, HandleValue lhs, HandleValue rhs, int32_t *out);
bool BitAndPar(ForkJoinSlice *slice, HandleValue lhs, HandleValue rhs, int32_t *out);
bool BitLshPar(ForkJoinSlice *slice, HandleValue lhs, HandleValue rhs, int32_t *out);
bool BitRshPar(ForkJoinSlice *slice, HandleValue lhs, HandleValue rhs, int32_t *out);

bool UrshValuesPar(ForkJoinSlice *slice, HandleValue lhs, HandleValue rhs,
                             Value *out);


JSObject *InitRestParameterPar(ForkJoinSlice *slice, uint32_t length, Value *rest,
                               HandleObject templateObj, HandleObject res);


void AbortPar(ParallelBailoutCause cause, JSScript *outermostScript, JSScript *currentScript,
              jsbytecode *bytecode);
void PropagateAbortPar(JSScript *outermostScript, JSScript *currentScript);

void TraceLIR(IonLIRTraceData *current);

void CallToUncompiledScriptPar(JSObject *obj);

} 
} 

#endif 
