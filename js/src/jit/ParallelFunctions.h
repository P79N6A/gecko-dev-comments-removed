





#ifndef jit_ParallelFunctions_h
#define jit_ParallelFunctions_h

#include "gc/Heap.h"
#include "vm/ForkJoin.h"

namespace js {

class TypedObject; 

namespace jit {

ForkJoinContext *ForkJoinContextPar();
JSObject *NewGCThingPar(ForkJoinContext *cx, gc::AllocKind allocKind);
bool ParallelWriteGuard(ForkJoinContext *cx, JSObject *object);
bool IsInTargetRegion(ForkJoinContext *cx, TypedObject *object);
bool CheckOverRecursedPar(ForkJoinContext *cx);
bool InterruptCheckPar(ForkJoinContext *cx);




JSObject *ExtendArrayPar(ForkJoinContext *cx, JSObject *array, uint32_t length);


bool SetPropertyPar(ForkJoinContext *cx, HandleObject obj, HandlePropertyName name,
                    HandleValue value, bool strict, jsbytecode *pc);
bool SetElementPar(ForkJoinContext *cx, HandleObject obj, HandleValue index,
                   HandleValue value, bool strict);
bool SetDenseElementPar(ForkJoinContext *cx, HandleObject obj, int32_t index,
                        HandleValue value, bool strict);



JSString *ConcatStringsPar(ForkJoinContext *cx, HandleString left, HandleString right);
JSFlatString *IntToStringPar(ForkJoinContext *cx, int i);
JSString *DoubleToStringPar(ForkJoinContext *cx, double d);
JSString *PrimitiveToStringPar(ForkJoinContext *cx, HandleValue input);
bool StringToNumberPar(ForkJoinContext *cx, JSString *str, double *out);



bool StrictlyEqualPar(ForkJoinContext *cx, MutableHandleValue v1, MutableHandleValue v2, bool *);
bool StrictlyUnequalPar(ForkJoinContext *cx, MutableHandleValue v1, MutableHandleValue v2, bool *);
bool LooselyEqualPar(ForkJoinContext *cx, MutableHandleValue v1, MutableHandleValue v2, bool *);
bool LooselyUnequalPar(ForkJoinContext *cx, MutableHandleValue v1, MutableHandleValue v2, bool *);
bool LessThanPar(ForkJoinContext *cx, MutableHandleValue v1, MutableHandleValue v2, bool *);
bool LessThanOrEqualPar(ForkJoinContext *cx, MutableHandleValue v1, MutableHandleValue v2, bool *);
bool GreaterThanPar(ForkJoinContext *cx, MutableHandleValue v1, MutableHandleValue v2, bool *);
bool GreaterThanOrEqualPar(ForkJoinContext *cx, MutableHandleValue v1, MutableHandleValue v2, bool *);

bool StringsEqualPar(ForkJoinContext *cx, HandleString v1, HandleString v2, bool *);
bool StringsUnequalPar(ForkJoinContext *cx, HandleString v1, HandleString v2, bool *);

bool BitNotPar(ForkJoinContext *cx, HandleValue in, int32_t *out);
bool BitXorPar(ForkJoinContext *cx, HandleValue lhs, HandleValue rhs, int32_t *out);
bool BitOrPar(ForkJoinContext *cx, HandleValue lhs, HandleValue rhs, int32_t *out);
bool BitAndPar(ForkJoinContext *cx, HandleValue lhs, HandleValue rhs, int32_t *out);
bool BitLshPar(ForkJoinContext *cx, HandleValue lhs, HandleValue rhs, int32_t *out);
bool BitRshPar(ForkJoinContext *cx, HandleValue lhs, HandleValue rhs, int32_t *out);

bool UrshValuesPar(ForkJoinContext *cx, HandleValue lhs, HandleValue rhs, MutableHandleValue out);


JSObject *InitRestParameterPar(ForkJoinContext *cx, uint32_t length, Value *rest,
                               HandleObject templateObj, HandleObject res);


void AbortPar(ParallelBailoutCause cause, JSScript *outermostScript, JSScript *currentScript,
              jsbytecode *bytecode);
void PropagateAbortPar(JSScript *outermostScript, JSScript *currentScript);

void TraceLIR(IonLIRTraceData *current);

void CallToUncompiledScriptPar(JSObject *obj);

} 
} 

#endif 
