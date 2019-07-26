






#include <stdio.h>

#include "Ion.h"
#include "MIR.h"
#include "MIRGraph.h"
#include "ParallelArrayAnalysis.h"
#include "IonSpewer.h"
#include "UnreachableCodeElimination.h"
#include "IonAnalysis.h"

#include "vm/ParallelDo.h"

#include "vm/Stack.h"

namespace js {
namespace ion {

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

#define PERMIT_NUMERIC (PERMIT(MIRType_Int32) | PERMIT(MIRType_Double))

#define SPECIALIZED_OP(op, flags)                                               \
    virtual bool visit##op(M##op *ins) {                                        \
        return visitSpecializedInstruction(ins, ins->specialization(), flags);  \
    }

#define UNSAFE_OP(op)                                               \
    virtual bool visit##op(M##op *ins) {                            \
        SpewMIR(ins, "Unsafe");                                     \
        return markUnsafe();                                        \
    }

#define WRITE_GUARDED_OP(op, obj)                   \
    virtual bool visit##op(M##op *prop) {           \
        return insertWriteGuard(prop, prop->obj()); \
    }

#define MAYBE_WRITE_GUARDED_OP(op, obj)                                       \
    virtual bool visit##op(M##op *prop) {                                     \
        if (prop->racy())                                                     \
            return true;                                                      \
        return insertWriteGuard(prop, prop->obj());                           \
    }

class ParallelArrayVisitor : public MInstructionVisitor
{
    JSContext *cx_;
    MIRGraph &graph_;
    bool unsafe_;
    MDefinition *parSlice_;

    bool insertWriteGuard(MInstruction *writeInstruction,
                          MDefinition *valueBeingWritten);

    bool replaceWithParNew(MInstruction *newInstruction,
                           JSObject *templateObject);

    bool replace(MInstruction *oldInstruction,
                 MInstruction *replacementInstruction);

    bool visitSpecializedInstruction(MInstruction *ins, MIRType spec, uint32_t flags);

    
    
    
    bool markUnsafe() {
        JS_ASSERT(!unsafe_);
        unsafe_ = true;
        return true;
    }

  public:
    AutoObjectVector callTargets;

    ParallelArrayVisitor(JSContext *cx,
                         MIRGraph &graph)
      : cx_(cx),
        graph_(graph),
        unsafe_(false),
        parSlice_(NULL),
        callTargets(cx)
    { }

    void clearUnsafe() { unsafe_ = false; }
    bool unsafe() { return unsafe_; }
    MDefinition *parSlice() {
        if (!parSlice_)
            parSlice_ = graph_.parSlice();
        return parSlice_;
    }

    bool convertToBailout(MBasicBlock *block, MInstruction *ins);

    
    

    SAFE_OP(Constant)
    SAFE_OP(Parameter)
    SAFE_OP(Callee)
    SAFE_OP(TableSwitch)
    SAFE_OP(Goto)
    CUSTOM_OP(Test)
    CUSTOM_OP(Compare)
    SAFE_OP(Phi)
    SAFE_OP(Beta)
    UNSAFE_OP(OsrValue)
    UNSAFE_OP(OsrScopeChain)
    UNSAFE_OP(ReturnFromCtor)
    CUSTOM_OP(CheckOverRecursed)
    DROP_OP(RecompileCheck)
    UNSAFE_OP(DefVar)
    UNSAFE_OP(DefFun)
    UNSAFE_OP(CreateThis)
    UNSAFE_OP(CreateThisWithTemplate)
    UNSAFE_OP(CreateThisWithProto)
    SAFE_OP(PrepareCall)
    SAFE_OP(PassArg)
    CUSTOM_OP(Call)
    UNSAFE_OP(ApplyArgs)
    UNSAFE_OP(CallDirectEval)
    SAFE_OP(BitNot)
    UNSAFE_OP(TypeOf)
    SAFE_OP(ToId)
    SAFE_OP(BitAnd)
    SAFE_OP(BitOr)
    SAFE_OP(BitXor)
    SAFE_OP(Lsh)
    SAFE_OP(Rsh)
    SPECIALIZED_OP(Ursh, PERMIT_NUMERIC)
    SPECIALIZED_OP(MinMax, PERMIT_NUMERIC)
    SAFE_OP(Abs)
    SAFE_OP(Sqrt)
    SAFE_OP(MathFunction)
    SPECIALIZED_OP(Add, PERMIT_NUMERIC)
    SPECIALIZED_OP(Sub, PERMIT_NUMERIC)
    SPECIALIZED_OP(Mul, PERMIT_NUMERIC)
    SPECIALIZED_OP(Div, PERMIT_NUMERIC)
    SPECIALIZED_OP(Mod, PERMIT_NUMERIC)
    UNSAFE_OP(Concat)
    UNSAFE_OP(CharCodeAt)
    UNSAFE_OP(FromCharCode)
    SAFE_OP(Return)
    CUSTOM_OP(Throw)
    SAFE_OP(Box)     
    SAFE_OP(Unbox)
    SAFE_OP(GuardObject)
    SAFE_OP(ToDouble)
    SAFE_OP(ToInt32)
    SAFE_OP(TruncateToInt32)
    UNSAFE_OP(ToString)
    SAFE_OP(NewSlots)
    CUSTOM_OP(NewArray)
    CUSTOM_OP(NewObject)
    CUSTOM_OP(NewCallObject)
    CUSTOM_OP(NewParallelArray)
    UNSAFE_OP(InitProp)
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
    UNSAFE_OP(GetPropertyCache)
    UNSAFE_OP(GetElementCache)
    UNSAFE_OP(BindNameCache)
    SAFE_OP(GuardShape)
    SAFE_OP(GuardClass)
    SAFE_OP(ArrayLength)
    SAFE_OP(TypedArrayLength)
    SAFE_OP(TypedArrayElements)
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
    MAYBE_WRITE_GUARDED_OP(StoreTypedArrayElement, elements)
    UNSAFE_OP(ClampToUint8)
    SAFE_OP(LoadFixedSlot)
    WRITE_GUARDED_OP(StoreFixedSlot, object)
    UNSAFE_OP(CallGetProperty)
    UNSAFE_OP(GetNameCache)
    SAFE_OP(CallGetIntrinsicValue) 
    UNSAFE_OP(CallsiteCloneCache)
    UNSAFE_OP(CallGetElement)
    UNSAFE_OP(CallSetElement)
    UNSAFE_OP(CallSetProperty)
    UNSAFE_OP(DeleteProperty)
    UNSAFE_OP(SetPropertyCache)
    UNSAFE_OP(IteratorStart)
    UNSAFE_OP(IteratorNext)
    UNSAFE_OP(IteratorMore)
    UNSAFE_OP(IteratorEnd)
    SAFE_OP(StringLength)
    UNSAFE_OP(ArgumentsLength)
    UNSAFE_OP(GetArgument)
    SAFE_OP(Floor)
    SAFE_OP(Round)
    UNSAFE_OP(InstanceOf)
    CUSTOM_OP(InterruptCheck)
    SAFE_OP(ParSlice)
    SAFE_OP(ParNew)
    SAFE_OP(ParNewDenseArray)
    SAFE_OP(ParNewCallObject)
    SAFE_OP(ParLambda)
    SAFE_OP(ParDump)
    SAFE_OP(ParBailout)
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
    SAFE_OP(ParWriteGuard)
    SAFE_OP(ParCheckInterrupt)
    SAFE_OP(ParCheckOverRecursed)
    SAFE_OP(PolyInlineDispatch)

    
    UNSAFE_OP(ConvertElementsToDoubles)
};

bool
ParallelCompileContext::appendToWorklist(HandleFunction fun)
{
    JS_ASSERT(fun);

    if (!fun->isInterpreted())
        return true;

    RootedScript script(cx_, fun->nonLazyScript());

    
    if (!script->canParallelIonCompile()) {
        Spew(SpewCompile, "Skipping %p:%s:%u, canParallelIonCompile() is false",
             fun.get(), script->filename, script->lineno);
        return true;
    }

    
    if (script->parallelIon == ION_COMPILING_SCRIPT) {
        Spew(SpewCompile, "Skipping %p:%s:%u, off-main-thread compilation in progress",
             fun.get(), script->filename, script->lineno);
        return true;
    }

    
    if (script->parallelIon && script->parallelIon->bailoutExpected()) {
        Spew(SpewCompile, "Skipping %p:%s:%u, bailout expected",
             fun.get(), script->filename, script->lineno);
        return true;
    }

    
    
    
    if (script->getUseCount() < js_IonOptions.usesBeforeCompileParallel) {
        Spew(SpewCompile, "Skipping %p:%s:%u, use count %u < %u",
             fun.get(), script->filename, script->lineno,
             script->getUseCount(), js_IonOptions.usesBeforeCompileParallel);
        return true;
    }

    for (uint32_t i = 0; i < worklist_.length(); i++) {
        if (worklist_[i]->toFunction() == fun)
            return true;
    }

    
    
    
    
    return worklist_.append(fun);
}

bool
ParallelCompileContext::analyzeAndGrowWorklist(MIRGenerator *mir, MIRGraph &graph)
{
    
    
    
    
    
    
    
    ParallelArrayVisitor visitor(cx_, graph);
    graph.entryBlock()->mark();  
    uint32_t marked = 0;
    for (ReversePostorderIterator block(graph.rpoBegin()); block != graph.rpoEnd(); block++) {
        if (mir->shouldCancel("ParallelArrayAnalysis"))
            return false;

        if (block->isMarked()) {
            
            
            
            
            MInstruction *instr = NULL;
            for (MInstructionIterator ins(block->begin());
                 ins != block->end() && !visitor.unsafe();)
            {
                if (mir->shouldCancel("ParallelArrayAnalysis"))
                    return false;

                
                
                
                
                instr = *ins++;

                if (!instr->accept(&visitor))
                    return false;
            }

            if (!visitor.unsafe()) {
                
                marked++;

                
                for (uint32_t i = 0; i < block->numSuccessors(); i++)
                    block->getSuccessor(i)->mark();
            } else {
                
                

                
                
                
                if (*block == graph.entryBlock()) {
                    Spew(SpewCompile, "Entry block contains unsafe MIR");
                    return false;
                }

                
                if (!visitor.convertToBailout(*block, instr))
                    return false;

                JS_ASSERT(!block->isMarked());
            }
        }
    }

    
    RootedFunction target(cx_);
    for (uint32_t i = 0; i < visitor.callTargets.length(); i++) {
        target = visitor.callTargets[i]->toFunction();
        appendToWorklist(target);
    }

    Spew(SpewCompile, "Safe");
    IonSpewPass("ParallelArrayAnalysis");

    UnreachableCodeElimination uce(mir, graph);
    if (!uce.removeUnmarkedBlocks(marked))
        return false;
    IonSpewPass("UCEAfterParallelArrayAnalysis");
    AssertExtendedGraphCoherency(graph);

    if (!removeResumePointOperands(mir, graph))
        return false;
    IonSpewPass("RemoveResumePointOperands");
    AssertExtendedGraphCoherency(graph);

    if (!EliminateDeadCode(mir, graph))
        return false;
    IonSpewPass("DCEAfterParallelArrayAnalysis");
    AssertExtendedGraphCoherency(graph);

    return true;
}

bool
ParallelCompileContext::removeResumePointOperands(MIRGenerator *mir, MIRGraph &graph)
{
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    

    MConstant *udef = NULL;
    for (ReversePostorderIterator block(graph.rpoBegin()); block != graph.rpoEnd(); block++) {
        if (udef)
            replaceOperandsOnResumePoint(block->entryResumePoint(), udef);

        for (MInstructionIterator ins(block->begin()); ins != block->end(); ins++) {
            if (ins->isStart()) {
                JS_ASSERT(udef == NULL);
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
ParallelCompileContext::replaceOperandsOnResumePoint(MResumePoint *resumePoint,
                                                     MDefinition *withDef)
{
    for (size_t i = 0; i < resumePoint->numOperands(); i++)
        resumePoint->replaceOperand(i, withDef);
}

bool
ParallelArrayVisitor::visitTest(MTest *)
{
    return true;
}

bool
ParallelArrayVisitor::visitCompare(MCompare *compare)
{
    MCompare::CompareType type = compare->compareType();
    return type == MCompare::Compare_Int32 ||
           type == MCompare::Compare_Double ||
           type == MCompare::Compare_String;
}

bool
ParallelArrayVisitor::convertToBailout(MBasicBlock *block, MInstruction *ins)
{
    JS_ASSERT(unsafe()); 
    JS_ASSERT(block->isMarked()); 

    
    clearUnsafe();

    
    block->unmark();

    
    jsbytecode *pc = block->pc();
    if (!pc)
        pc = block->pc();

    
    
    
    
    
    
    
    for (size_t i = 0; i < block->numPredecessors(); i++) {
        MBasicBlock *pred = block->getPredecessor(i);

        
        if (!pred->isMarked())
            continue;

        
        MBasicBlock *bailBlock = MBasicBlock::NewParBailout(graph_, block->info(), pred, pc);
        if (!bailBlock)
            return false;

        
        if (pred->successorWithPhis() == block)
            pred->setSuccessorWithPhis(NULL, 0);

        
        uint32_t succIdx = pred->getSuccessorIndex(block);
        pred->replaceSuccessor(succIdx, bailBlock);

        
        
        
        
        
        graph_.insertBlockAfter(block, bailBlock);
        bailBlock->mark();
    }

    return true;
}









bool
ParallelArrayVisitor::visitNewParallelArray(MNewParallelArray *ins)
{
    MParNew *parNew = new MParNew(parSlice(), ins->templateObject());
    replace(ins, parNew);
    return true;
}

bool
ParallelArrayVisitor::visitNewCallObject(MNewCallObject *ins)
{
    
    MParNewCallObject *parNewCallObjectInstruction =
        MParNewCallObject::New(parSlice(), ins);
    replace(ins, parNewCallObjectInstruction);
    return true;
}

bool
ParallelArrayVisitor::visitLambda(MLambda *ins)
{
    if (ins->fun()->hasSingletonType() ||
        types::UseNewTypeForClone(ins->fun()))
    {
        
        return markUnsafe();
    }

    
    MParLambda *parLambdaInstruction = MParLambda::New(parSlice(), ins);
    replace(ins, parLambdaInstruction);
    return true;
}

bool
ParallelArrayVisitor::visitNewObject(MNewObject *newInstruction)
{
    if (newInstruction->shouldUseVM()) {
        SpewMIR(newInstruction, "should use VM");
        return markUnsafe();
    }

    return replaceWithParNew(newInstruction,
                             newInstruction->templateObject());
}

bool
ParallelArrayVisitor::visitNewArray(MNewArray *newInstruction)
{
    if (newInstruction->shouldUseVM()) {
        SpewMIR(newInstruction, "should use VM");
        return markUnsafe();
    }

    return replaceWithParNew(newInstruction,
                             newInstruction->templateObject());
}

bool
ParallelArrayVisitor::replaceWithParNew(MInstruction *newInstruction,
                                        JSObject *templateObject)
{
    MParNew *parNewInstruction = new MParNew(parSlice(), templateObject);
    replace(newInstruction, parNewInstruction);
    return true;
}

bool
ParallelArrayVisitor::replace(MInstruction *oldInstruction,
                              MInstruction *replacementInstruction)
{
    MBasicBlock *block = oldInstruction->block();
    block->insertBefore(oldInstruction, replacementInstruction);
    oldInstruction->replaceAllUsesWith(replacementInstruction);
    block->discard(oldInstruction);
    return true;
}











bool
ParallelArrayVisitor::insertWriteGuard(MInstruction *writeInstruction,
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
      case MDefinition::Op_ParNew:
        
        SpewMIR(writeInstruction, "write to ParNew prop does not require guard");
        return true;
      default:
        break;
    }

    MBasicBlock *block = writeInstruction->block();
    MParWriteGuard *writeGuard = MParWriteGuard::New(parSlice(), object);
    block->insertBefore(writeInstruction, writeGuard);
    writeGuard->adjustInputs(writeGuard);
    return true;
}








static bool
GetPossibleCallees(JSContext *cx, HandleScript script, jsbytecode *pc,
                   types::StackTypeSet *calleeTypes, AutoObjectVector &targets)
{
    JS_ASSERT(calleeTypes);

    if (calleeTypes->baseFlags() != 0)
        return true;

    unsigned objCount = calleeTypes->getObjectCount();

    if (objCount == 0)
        return true;

    RootedFunction fun(cx);
    for (unsigned i = 0; i < objCount; i++) {
        RawObject obj = calleeTypes->getSingleObject(i);
        if (obj && obj->isFunction()) {
            fun = obj->toFunction();
        } else {
            types::TypeObject *typeObj = calleeTypes->getTypeObject(i);
            if (!typeObj)
                continue;
            fun = typeObj->interpretedFunction;
            if (!fun)
                continue;
        }

        if (fun->isCloneAtCallsite()) {
            fun = CloneFunctionAtCallsite(cx, fun, script, pc);
            if (!fun)
                return false;
        }

        if (!targets.append(fun))
            return false;
    }

    return true;
}

bool
ParallelArrayVisitor::visitCall(MCall *ins)
{
    JS_ASSERT(ins->getSingleTarget() || ins->calleeTypes());

    
    if (ins->isDOMFunction()) {
        SpewMIR(ins, "call to dom function");
        return markUnsafe();
    }

    RootedFunction target(cx_, ins->getSingleTarget());
    if (target) {
        
        if (target->isNative()) {
            SpewMIR(ins, "call to native function");
            return markUnsafe();
        }
        return callTargets.append(target);
    }

    if (ins->isConstructing()) {
        SpewMIR(ins, "call to unknown constructor");
        return markUnsafe();
    }

    RootedScript script(cx_, ins->block()->info().script());
    return GetPossibleCallees(cx_, script, ins->resumePoint()->pc(),
                              ins->calleeTypes(), callTargets);
}









bool
ParallelArrayVisitor::visitCheckOverRecursed(MCheckOverRecursed *ins)
{
    MParCheckOverRecursed *replacement = new MParCheckOverRecursed(parSlice());
    return replace(ins, replacement);
}

bool
ParallelArrayVisitor::visitInterruptCheck(MInterruptCheck *ins)
{
    MParCheckInterrupt *replacement = new MParCheckInterrupt(parSlice());
    return replace(ins, replacement);
}










bool
ParallelArrayVisitor::visitSpecializedInstruction(MInstruction *ins, MIRType spec,
                                                  uint32_t flags)
{
    uint32_t flag = 1 << spec;
    if (flags & flag)
        return true;

    SpewMIR(ins, "specialized to unacceptable type %d", spec);
    return markUnsafe();
}




bool
ParallelArrayVisitor::visitThrow(MThrow *thr)
{
    MBasicBlock *block = thr->block();
    JS_ASSERT(block->lastIns() == thr);
    block->discardLastIns();
    MParBailout *bailout = new MParBailout();
    if (!bailout)
        return false;
    block->end(bailout);
    return true;
}

}
}

