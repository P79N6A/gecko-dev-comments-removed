





#include "jit/ParallelSafetyAnalysis.h"

#include "jit/Ion.h"
#include "jit/IonAnalysis.h"
#include "jit/IonSpewer.h"
#include "jit/MIR.h"
#include "jit/MIRGenerator.h"
#include "jit/MIRGraph.h"
#include "jit/UnreachableCodeElimination.h"

#include "jsinferinlines.h"

using namespace js;
using namespace jit;

using parallel::Spew;
using parallel::SpewMIR;
using parallel::SpewCompile;

#define SAFE_OP(op)                             \
    virtual bool visit##op(M##op *prop) { return true; }

#define CUSTOM_OP(op)                        \
    virtual bool visit##op(M##op *prop);

#define DROP_OP(op)                             \
    virtual bool visit##op(M##op *ins) {        \
        MBasicBlock *block = ins->block();      \
        block->discard(ins);                    \
        return true;                            \
    }

#define PERMIT(T) (1 << T)

#define PERMIT_INT32 (PERMIT(MIRType_Int32))
#define PERMIT_NUMERIC (PERMIT(MIRType_Int32) | PERMIT(MIRType_Double))

#define SPECIALIZED_OP(op, flags)                                               \
    virtual bool visit##op(M##op *ins) {                                        \
        return visitSpecializedInstruction(ins, ins->specialization(), flags);  \
    }

#define UNSAFE_OP(op)                                                         \
    virtual bool visit##op(M##op *ins) {                                      \
        SpewMIR(ins, "Unsafe");                                               \
        return markUnsafe();                                                  \
    }

#define WRITE_GUARDED_OP(op, obj)                                             \
    virtual bool visit##op(M##op *prop) {                                     \
        return insertWriteGuard(prop, prop->obj());                           \
    }

#define MAYBE_WRITE_GUARDED_OP(op, obj)                                       \
    virtual bool visit##op(M##op *prop) {                                     \
        if (prop->racy())                                                     \
            return true;                                                      \
        return insertWriteGuard(prop, prop->obj());                           \
    }

class ParallelSafetyVisitor : public MInstructionVisitor
{
    MIRGraph &graph_;
    bool unsafe_;
    MDefinition *slice_;

    bool insertWriteGuard(MInstruction *writeInstruction, MDefinition *valueBeingWritten);

    bool replaceWithNewPar(MInstruction *newInstruction, JSObject *templateObject);
    bool replace(MInstruction *oldInstruction, MInstruction *replacementInstruction);

    bool visitSpecializedInstruction(MInstruction *ins, MIRType spec, uint32_t flags);

    
    
    
    bool markUnsafe() {
        JS_ASSERT(!unsafe_);
        unsafe_ = true;
        return true;
    }

  public:
    ParallelSafetyVisitor(MIRGraph &graph)
      : graph_(graph),
        unsafe_(false),
        slice_(nullptr)
    { }

    void clearUnsafe() { unsafe_ = false; }
    bool unsafe() { return unsafe_; }
    MDefinition *forkJoinSlice() {
        if (!slice_)
            slice_ = graph_.forkJoinSlice();
        return slice_;
    }

    bool convertToBailout(MBasicBlock *block, MInstruction *ins);

    
    

    SAFE_OP(Constant)
    SAFE_OP(Parameter)
    SAFE_OP(Callee)
    SAFE_OP(TableSwitch)
    SAFE_OP(Goto)
    SAFE_OP(Test)
    SAFE_OP(Compare)
    SAFE_OP(Phi)
    SAFE_OP(Beta)
    UNSAFE_OP(OsrValue)
    UNSAFE_OP(OsrScopeChain)
    UNSAFE_OP(OsrArgumentsObject)
    UNSAFE_OP(ReturnFromCtor)
    CUSTOM_OP(CheckOverRecursed)
    UNSAFE_OP(DefVar)
    UNSAFE_OP(DefFun)
    UNSAFE_OP(CreateThis)
    CUSTOM_OP(CreateThisWithTemplate)
    UNSAFE_OP(CreateThisWithProto)
    UNSAFE_OP(CreateArgumentsObject)
    UNSAFE_OP(GetArgumentsObjectArg)
    UNSAFE_OP(SetArgumentsObjectArg)
    UNSAFE_OP(ComputeThis)
    SAFE_OP(PrepareCall)
    SAFE_OP(PassArg)
    CUSTOM_OP(Call)
    UNSAFE_OP(ApplyArgs)
    UNSAFE_OP(Bail)
    UNSAFE_OP(AssertFloat32)
    UNSAFE_OP(GetDynamicName)
    UNSAFE_OP(FilterArguments)
    UNSAFE_OP(CallDirectEval)
    SAFE_OP(BitNot)
    UNSAFE_OP(TypeOf)
    SAFE_OP(ToId)
    SAFE_OP(BitAnd)
    SAFE_OP(BitOr)
    SAFE_OP(BitXor)
    SAFE_OP(Lsh)
    SAFE_OP(Rsh)
    SAFE_OP(Ursh)
    SPECIALIZED_OP(MinMax, PERMIT_NUMERIC)
    SAFE_OP(Abs)
    SAFE_OP(Sqrt)
    UNSAFE_OP(Atan2)
    CUSTOM_OP(MathFunction)
    SPECIALIZED_OP(Add, PERMIT_NUMERIC)
    SPECIALIZED_OP(Sub, PERMIT_NUMERIC)
    SPECIALIZED_OP(Mul, PERMIT_NUMERIC)
    SPECIALIZED_OP(Div, PERMIT_NUMERIC)
    SPECIALIZED_OP(Mod, PERMIT_NUMERIC)
    CUSTOM_OP(Concat)
    SAFE_OP(ConcatPar)
    UNSAFE_OP(CharCodeAt)
    UNSAFE_OP(FromCharCode)
    SAFE_OP(Return)
    CUSTOM_OP(Throw)
    SAFE_OP(Box)     
    SAFE_OP(Unbox)
    SAFE_OP(GuardObject)
    SAFE_OP(ToDouble)
    SAFE_OP(ToFloat32)
    SAFE_OP(ToInt32)
    SAFE_OP(TruncateToInt32)
    SAFE_OP(MaybeToDoubleElement)
    CUSTOM_OP(ToString)
    SAFE_OP(NewSlots)
    CUSTOM_OP(NewArray)
    CUSTOM_OP(NewObject)
    CUSTOM_OP(NewCallObject)
    CUSTOM_OP(NewParallelArray)
    UNSAFE_OP(NewDerivedTypedObject)
    UNSAFE_OP(InitElem)
    UNSAFE_OP(InitElemGetterSetter)
    UNSAFE_OP(InitProp)
    UNSAFE_OP(InitPropGetterSetter)
    SAFE_OP(Start)
    UNSAFE_OP(OsrEntry)
    SAFE_OP(Nop)
    UNSAFE_OP(RegExp)
    CUSTOM_OP(Lambda)
    UNSAFE_OP(ImplicitThis)
    SAFE_OP(Slots)
    SAFE_OP(Elements)
    SAFE_OP(ConstantElements)
    SAFE_OP(LoadSlot)
    WRITE_GUARDED_OP(StoreSlot, slots)
    SAFE_OP(FunctionEnvironment) 
    SAFE_OP(TypeBarrier) 
    SAFE_OP(MonitorTypes) 
    UNSAFE_OP(PostWriteBarrier)
    SAFE_OP(GetPropertyCache)
    SAFE_OP(GetPropertyPolymorphic)
    UNSAFE_OP(SetPropertyPolymorphic)
    SAFE_OP(GetElementCache)
    UNSAFE_OP(SetElementCache)
    UNSAFE_OP(BindNameCache)
    SAFE_OP(GuardShape)
    SAFE_OP(GuardObjectType)
    SAFE_OP(GuardClass)
    SAFE_OP(AssertRange)
    SAFE_OP(ArrayLength)
    SAFE_OP(TypedArrayLength)
    SAFE_OP(TypedArrayElements)
    SAFE_OP(TypedObjectElements)
    SAFE_OP(InitializedLength)
    WRITE_GUARDED_OP(SetInitializedLength, elements)
    SAFE_OP(Not)
    SAFE_OP(BoundsCheck)
    SAFE_OP(BoundsCheckLower)
    SAFE_OP(LoadElement)
    SAFE_OP(LoadElementHole)
    MAYBE_WRITE_GUARDED_OP(StoreElement, elements)
    WRITE_GUARDED_OP(StoreElementHole, elements)
    UNSAFE_OP(ArrayPopShift)
    UNSAFE_OP(ArrayPush)
    SAFE_OP(LoadTypedArrayElement)
    SAFE_OP(LoadTypedArrayElementHole)
    SAFE_OP(LoadTypedArrayElementStatic)
    MAYBE_WRITE_GUARDED_OP(StoreTypedArrayElement, elements)
    WRITE_GUARDED_OP(StoreTypedArrayElementHole, elements)
    UNSAFE_OP(StoreTypedArrayElementStatic)
    UNSAFE_OP(ClampToUint8)
    SAFE_OP(LoadFixedSlot)
    WRITE_GUARDED_OP(StoreFixedSlot, object)
    UNSAFE_OP(CallGetProperty)
    UNSAFE_OP(GetNameCache)
    UNSAFE_OP(CallGetIntrinsicValue)
    UNSAFE_OP(CallsiteCloneCache)
    UNSAFE_OP(CallGetElement)
    UNSAFE_OP(CallSetElement)
    UNSAFE_OP(CallInitElementArray)
    UNSAFE_OP(CallSetProperty)
    UNSAFE_OP(DeleteProperty)
    UNSAFE_OP(DeleteElement)
    UNSAFE_OP(SetPropertyCache)
    UNSAFE_OP(IteratorStart)
    UNSAFE_OP(IteratorNext)
    UNSAFE_OP(IteratorMore)
    UNSAFE_OP(IteratorEnd)
    SAFE_OP(StringLength)
    SAFE_OP(ArgumentsLength)
    SAFE_OP(GetFrameArgument)
    UNSAFE_OP(SetFrameArgument)
    UNSAFE_OP(RunOncePrologue)
    CUSTOM_OP(Rest)
    SAFE_OP(RestPar)
    SAFE_OP(Floor)
    SAFE_OP(Round)
    UNSAFE_OP(InstanceOf)
    CUSTOM_OP(InterruptCheck)
    SAFE_OP(ForkJoinSlice)
    SAFE_OP(NewPar)
    SAFE_OP(NewDenseArrayPar)
    SAFE_OP(NewCallObjectPar)
    SAFE_OP(LambdaPar)
    SAFE_OP(AbortPar)
    UNSAFE_OP(ArrayConcat)
    UNSAFE_OP(GetDOMProperty)
    UNSAFE_OP(SetDOMProperty)
    UNSAFE_OP(NewStringObject)
    UNSAFE_OP(Random)
    UNSAFE_OP(Pow)
    UNSAFE_OP(PowHalf)
    UNSAFE_OP(RegExpTest)
    UNSAFE_OP(CallInstanceOf)
    UNSAFE_OP(FunctionBoundary)
    UNSAFE_OP(GuardString)
    UNSAFE_OP(NewDeclEnvObject)
    UNSAFE_OP(In)
    UNSAFE_OP(InArray)
    SAFE_OP(GuardThreadLocalObject)
    SAFE_OP(CheckInterruptPar)
    SAFE_OP(CheckOverRecursedPar)
    SAFE_OP(FunctionDispatch)
    SAFE_OP(TypeObjectDispatch)
    SAFE_OP(IsCallable)
    SAFE_OP(HaveSameClass)
    UNSAFE_OP(EffectiveAddress)
    UNSAFE_OP(AsmJSUnsignedToDouble)
    UNSAFE_OP(AsmJSNeg)
    UNSAFE_OP(AsmJSUDiv)
    UNSAFE_OP(AsmJSUMod)
    UNSAFE_OP(AsmJSLoadHeap)
    UNSAFE_OP(AsmJSStoreHeap)
    UNSAFE_OP(AsmJSLoadGlobalVar)
    UNSAFE_OP(AsmJSStoreGlobalVar)
    UNSAFE_OP(AsmJSLoadFuncPtr)
    UNSAFE_OP(AsmJSLoadFFIFunc)
    UNSAFE_OP(AsmJSReturn)
    UNSAFE_OP(AsmJSVoidReturn)
    UNSAFE_OP(AsmJSPassStackArg)
    UNSAFE_OP(AsmJSParameter)
    UNSAFE_OP(AsmJSCall)
    UNSAFE_OP(AsmJSCheckOverRecursed)

    
    UNSAFE_OP(ConvertElementsToDoubles)
};

bool
ParallelSafetyAnalysis::analyze()
{
    
    
    
    
    
    
    
    ParallelSafetyVisitor visitor(graph_);
    graph_.entryBlock()->mark();  
    uint32_t marked = 0;
    for (ReversePostorderIterator block(graph_.rpoBegin()); block != graph_.rpoEnd(); block++) {
        if (mir_->shouldCancel("ParallelSafetyAnalysis"))
            return false;

        if (block->isMarked()) {
            
            
            
            
            MInstruction *instr = nullptr;
            for (MInstructionIterator ins(block->begin());
                 ins != block->end() && !visitor.unsafe();)
            {
                if (mir_->shouldCancel("ParallelSafetyAnalysis"))
                    return false;

                
                
                
                
                instr = *ins++;

                if (!instr->accept(&visitor)) {
                    SpewMIR(instr, "Unaccepted");
                    return false;
                }
            }

            if (!visitor.unsafe()) {
                
                marked++;

                
                for (uint32_t i = 0; i < block->numSuccessors(); i++)
                    block->getSuccessor(i)->mark();
            } else {
                
                

                
                
                
                if (*block == graph_.entryBlock()) {
                    Spew(SpewCompile, "Entry block contains unsafe MIR");
                    return false;
                }

                
                if (!visitor.convertToBailout(*block, instr))
                    return false;

                JS_ASSERT(!block->isMarked());
            }
        }
    }

    Spew(SpewCompile, "Safe");
    IonSpewPass("ParallelSafetyAnalysis");

    UnreachableCodeElimination uce(mir_, graph_);
    if (!uce.removeUnmarkedBlocks(marked))
        return false;
    IonSpewPass("UCEAfterParallelSafetyAnalysis");
    AssertExtendedGraphCoherency(graph_);

    if (!removeResumePointOperands())
        return false;
    IonSpewPass("RemoveResumePointOperands");
    AssertExtendedGraphCoherency(graph_);

    return true;
}

bool
ParallelSafetyAnalysis::removeResumePointOperands()
{
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    

    MConstant *udef = nullptr;
    for (ReversePostorderIterator block(graph_.rpoBegin()); block != graph_.rpoEnd(); block++) {
        if (udef)
            replaceOperandsOnResumePoint(block->entryResumePoint(), udef);

        for (MInstructionIterator ins(block->begin()); ins != block->end(); ins++) {
            if (ins->isStart()) {
                JS_ASSERT(udef == nullptr);
                udef = MConstant::New(UndefinedValue());
                block->insertAfter(*ins, udef);
            } else if (udef) {
                if (MResumePoint *resumePoint = ins->resumePoint())
                    replaceOperandsOnResumePoint(resumePoint, udef);
            }
        }
    }
    return true;
}

void
ParallelSafetyAnalysis::replaceOperandsOnResumePoint(MResumePoint *resumePoint,
                                                     MDefinition *withDef)
{
    for (size_t i = 0, e = resumePoint->numOperands(); i < e; i++)
        resumePoint->replaceOperand(i, withDef);
}

bool
ParallelSafetyVisitor::convertToBailout(MBasicBlock *block, MInstruction *ins)
{
    JS_ASSERT(unsafe()); 
    JS_ASSERT(block->isMarked()); 

    
    clearUnsafe();

    
    block->unmark();

    
    
    
    
    
    
    
    for (size_t i = 0; i < block->numPredecessors(); i++) {
        MBasicBlock *pred = block->getPredecessor(i);

        
        if (!pred->isMarked())
            continue;

        
        MBasicBlock *bailBlock = MBasicBlock::NewAbortPar(graph_, block->info(), pred,
                                                               block->pc(),
                                                               block->entryResumePoint());
        if (!bailBlock)
            return false;

        
        if (pred->successorWithPhis() == block)
            pred->setSuccessorWithPhis(nullptr, 0);

        
        uint32_t succIdx = pred->getSuccessorIndex(block);
        pred->replaceSuccessor(succIdx, bailBlock);

        
        
        
        
        
        graph_.insertBlockAfter(block, bailBlock);
        bailBlock->mark();
    }

    return true;
}









bool
ParallelSafetyVisitor::visitCreateThisWithTemplate(MCreateThisWithTemplate *ins)
{
    return replaceWithNewPar(ins, ins->templateObject());
}

bool
ParallelSafetyVisitor::visitNewParallelArray(MNewParallelArray *ins)
{
    return replaceWithNewPar(ins, ins->templateObject());
}

bool
ParallelSafetyVisitor::visitNewCallObject(MNewCallObject *ins)
{
    replace(ins, MNewCallObjectPar::New(forkJoinSlice(), ins));
    return true;
}

bool
ParallelSafetyVisitor::visitLambda(MLambda *ins)
{
    if (ins->info().singletonType || ins->info().useNewTypeForClone) {
        
        return markUnsafe();
    }

    
    replace(ins, MLambdaPar::New(forkJoinSlice(), ins));
    return true;
}

bool
ParallelSafetyVisitor::visitNewObject(MNewObject *newInstruction)
{
    if (newInstruction->shouldUseVM()) {
        SpewMIR(newInstruction, "should use VM");
        return markUnsafe();
    }

    return replaceWithNewPar(newInstruction, newInstruction->templateObject());
}

bool
ParallelSafetyVisitor::visitNewArray(MNewArray *newInstruction)
{
    if (newInstruction->shouldUseVM()) {
        SpewMIR(newInstruction, "should use VM");
        return markUnsafe();
    }

    return replaceWithNewPar(newInstruction, newInstruction->templateObject());
}

bool
ParallelSafetyVisitor::visitRest(MRest *ins)
{
    return replace(ins, MRestPar::New(forkJoinSlice(), ins));
}

bool
ParallelSafetyVisitor::visitMathFunction(MMathFunction *ins)
{
    return replace(ins, MMathFunction::New(ins->input(), ins->function(), nullptr));
}

bool
ParallelSafetyVisitor::visitConcat(MConcat *ins)
{
    return replace(ins, MConcatPar::New(forkJoinSlice(), ins));
}

bool
ParallelSafetyVisitor::visitToString(MToString *ins)
{
    MIRType inputType = ins->input()->type();
    if (inputType != MIRType_Int32 && inputType != MIRType_Double)
        return markUnsafe();
    return true;
}

bool
ParallelSafetyVisitor::replaceWithNewPar(MInstruction *newInstruction,
                                         JSObject *templateObject)
{
    replace(newInstruction, new MNewPar(forkJoinSlice(), templateObject));
    return true;
}

bool
ParallelSafetyVisitor::replace(MInstruction *oldInstruction,
                               MInstruction *replacementInstruction)
{
    MBasicBlock *block = oldInstruction->block();
    block->insertBefore(oldInstruction, replacementInstruction);
    oldInstruction->replaceAllUsesWith(replacementInstruction);
    block->discard(oldInstruction);
    return true;
}











bool
ParallelSafetyVisitor::insertWriteGuard(MInstruction *writeInstruction,
                                        MDefinition *valueBeingWritten)
{
    
    
    
    MDefinition *object;
    switch (valueBeingWritten->type()) {
      case MIRType_Object:
        object = valueBeingWritten;
        break;

      case MIRType_Slots:
        switch (valueBeingWritten->op()) {
          case MDefinition::Op_Slots:
            object = valueBeingWritten->toSlots()->object();
            break;

          case MDefinition::Op_NewSlots:
            
            
            return true;

          default:
            SpewMIR(writeInstruction, "cannot insert write guard for %s",
                    valueBeingWritten->opName());
            return markUnsafe();
        }
        break;

      case MIRType_Elements:
        switch (valueBeingWritten->op()) {
          case MDefinition::Op_Elements:
            object = valueBeingWritten->toElements()->object();
            break;

          case MDefinition::Op_TypedArrayElements:
            object = valueBeingWritten->toTypedArrayElements()->object();
            break;

          default:
            SpewMIR(writeInstruction, "cannot insert write guard for %s",
                    valueBeingWritten->opName());
            return markUnsafe();
        }
        break;

      default:
        SpewMIR(writeInstruction, "cannot insert write guard for MIR Type %d",
                valueBeingWritten->type());
        return markUnsafe();
    }

    if (object->isUnbox())
        object = object->toUnbox()->input();

    switch (object->op()) {
      case MDefinition::Op_NewPar:
        
        SpewMIR(writeInstruction, "write to NewPar prop does not require guard");
        return true;
      default:
        break;
    }

    MBasicBlock *block = writeInstruction->block();
    MGuardThreadLocalObject *writeGuard = MGuardThreadLocalObject::New(forkJoinSlice(), object);
    block->insertBefore(writeInstruction, writeGuard);
    writeGuard->adjustInputs(writeGuard);
    return true;
}







bool
ParallelSafetyVisitor::visitCall(MCall *ins)
{
    
    if (ins->isDOMFunction()) {
        SpewMIR(ins, "call to dom function");
        return markUnsafe();
    }

    JSFunction *target = ins->getSingleTarget();
    if (target) {
        
        if (target->isNative() && !target->hasParallelNative()) {
            SpewMIR(ins, "call to non-parallel native function");
            return markUnsafe();
        }
        return true;
    }

    if (ins->isConstructing()) {
        SpewMIR(ins, "call to unknown constructor");
        return markUnsafe();
    }

    return true;
}









bool
ParallelSafetyVisitor::visitCheckOverRecursed(MCheckOverRecursed *ins)
{
    return replace(ins, new MCheckOverRecursedPar(forkJoinSlice()));
}

bool
ParallelSafetyVisitor::visitInterruptCheck(MInterruptCheck *ins)
{
    return replace(ins, new MCheckInterruptPar(forkJoinSlice()));
}










bool
ParallelSafetyVisitor::visitSpecializedInstruction(MInstruction *ins, MIRType spec,
                                                   uint32_t flags)
{
    uint32_t flag = 1 << spec;
    if (flags & flag)
        return true;

    SpewMIR(ins, "specialized to unacceptable type %d", spec);
    return markUnsafe();
}




bool
ParallelSafetyVisitor::visitThrow(MThrow *thr)
{
    MBasicBlock *block = thr->block();
    JS_ASSERT(block->lastIns() == thr);
    block->discardLastIns();
    MAbortPar *bailout = new MAbortPar();
    if (!bailout)
        return false;
    block->end(bailout);
    return true;
}






static bool
GetPossibleCallees(JSContext *cx, HandleScript script, jsbytecode *pc,
                   types::TemporaryTypeSet *calleeTypes, CallTargetVector &targets);

static bool
AddCallTarget(HandleScript script, CallTargetVector &targets);

bool
jit::AddPossibleCallees(JSContext *cx, MIRGraph &graph, CallTargetVector &targets)
{
    for (ReversePostorderIterator block(graph.rpoBegin()); block != graph.rpoEnd(); block++) {
        for (MInstructionIterator ins(block->begin()); ins != block->end(); ins++)
        {
            if (!ins->isCall())
                continue;

            MCall *callIns = ins->toCall();

            RootedFunction target(cx, callIns->getSingleTarget());
            if (target) {
                JS_ASSERT_IF(!target->isInterpreted(), target->hasParallelNative());

                if (target->isInterpreted()) {
                    RootedScript script(cx, target->getOrCreateScript(cx));
                    if (!script || !AddCallTarget(script, targets))
                        return false;
                }

                continue;
            }

            types::TemporaryTypeSet *calleeTypes = callIns->getFunction()->resultTypeSet();
            RootedScript script(cx, callIns->block()->info().script());
            if (!GetPossibleCallees(cx,
                                    script,
                                    callIns->resumePoint()->pc(),
                                    calleeTypes,
                                    targets))
                return false;
        }
    }

    return true;
}

static bool
GetPossibleCallees(JSContext *cx,
                   HandleScript script,
                   jsbytecode *pc,
                   types::TemporaryTypeSet *calleeTypes,
                   CallTargetVector &targets)
{
    if (!calleeTypes || calleeTypes->baseFlags() != 0)
        return true;

    unsigned objCount = calleeTypes->getObjectCount();

    if (objCount == 0)
        return true;

    RootedFunction rootedFun(cx);
    RootedScript rootedScript(cx);
    for (unsigned i = 0; i < objCount; i++) {
        JSObject *obj = calleeTypes->getSingleObject(i);
        if (obj && obj->is<JSFunction>()) {
            rootedFun = &obj->as<JSFunction>();
        } else {
            types::TypeObject *typeObj = calleeTypes->getTypeObject(i);
            if (!typeObj)
                continue;
            rootedFun = typeObj->interpretedFunction;
            if (!rootedFun)
                continue;
        }

        if (!rootedFun->isInterpreted())
            continue;

        rootedScript = rootedFun->getOrCreateScript(cx);
        if (!rootedScript)
            return false;

        if (rootedScript->shouldCloneAtCallsite) {
            rootedFun = CloneFunctionAtCallsite(cx, rootedFun, script, pc);
            if (!rootedFun)
                return false;
            rootedScript = rootedFun->nonLazyScript();
        }

        
        if (!AddCallTarget(rootedScript, targets))
            return false;
    }

    return true;
}

static bool
AddCallTarget(HandleScript script, CallTargetVector &targets)
{
    for (size_t i = 0; i < targets.length(); i++) {
        if (targets[i] == script)
            return true;
    }

    if (!targets.append(script))
        return false;

    return true;
}
