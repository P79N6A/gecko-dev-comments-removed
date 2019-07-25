







































#include "CodeGenerator-shared.h"
#include "ion/MIRGenerator.h"
#include "ion/IonFrames.h"
#include "ion/MIR.h"
#include "CodeGenerator-shared-inl.h"
#include "ion/IonSpewer.h"

using namespace js;
using namespace js::ion;

CodeGeneratorShared::CodeGeneratorShared(MIRGenerator *gen, LIRGraph &graph)
  : gen(gen),
    graph(graph),
    deoptTable_(NULL),
    frameDepth_(graph.localSlotCount() * sizeof(STACK_SLOT_SIZE) +
                graph.argumentSlotCount() * sizeof(Value))
{
    frameClass_ = FrameSizeClass::FromDepth(frameDepth_);
}

bool
CodeGeneratorShared::generateOutOfLineCode()
{
    for (size_t i = 0; i < outOfLineCode_.length(); i++) {
        if (!outOfLineCode_[i]->generate(this))
            return false;
    }

    return true;
}

bool
CodeGeneratorShared::addOutOfLineCode(OutOfLineCode *code)
{
    return outOfLineCode_.append(code);
}

bool
CodeGeneratorShared::encodeSlots(LSnapshot *snapshot, MResumePoint *resumePoint,
                                 uint32 *startIndex)
{
    IonSpew(IonSpew_Codegen, "Encoding %u of resume point %p's operands starting from %u",
            resumePoint->numOperands(), (void *) resumePoint, *startIndex);
    for (uint32 slotno = 0; slotno < resumePoint->numOperands(); slotno++) {
        uint32 i = slotno + *startIndex;
        MDefinition *mir = resumePoint->getOperand(slotno);

        MIRType type = mir->isUnused()
                       ? MIRType_Undefined
                       : mir->type();

        switch (type) {
          case MIRType_Undefined:
            snapshots_.addUndefinedSlot();
            break;
          case MIRType_Null:
            snapshots_.addNullSlot();
            break;
          case MIRType_Int32:
          case MIRType_String:
          case MIRType_Object:
          case MIRType_Boolean:
          case MIRType_Double:
          {
            LAllocation *payload = snapshot->payloadOfSlot(i);
            JSValueType type = ValueTypeFromMIRType(mir->type());
            if (payload->isMemory()) {
                snapshots_.addSlot(type, ToStackOffset(payload));
            } else if (payload->isGeneralReg()) {
                snapshots_.addSlot(type, ToRegister(payload));
            } else if (payload->isFloatReg()) {
                snapshots_.addSlot(ToFloatRegister(payload));
            } else {
                MConstant *constant = mir->toConstant();
                const Value &v = constant->value();

                
                if (v.isInt32() && v.toInt32() >= -32 && v.toInt32() <= 32) {
                    snapshots_.addInt32Slot(v.toInt32());
                } else {
                    uint32 index;
                    if (!graph.addConstantToPool(constant, &index))
                        return false;
                    snapshots_.addConstantPoolSlot(index);
                }
            }
            break;
          }
          default:
          {
            JS_ASSERT(mir->type() == MIRType_Value);
            LAllocation *payload = snapshot->payloadOfSlot(i);
#ifdef JS_NUNBOX32
            LAllocation *type = snapshot->typeOfSlot(i);
            if (type->isRegister()) {
                if (payload->isRegister())
                    snapshots_.addSlot(ToRegister(type), ToRegister(payload));
                else
                    snapshots_.addSlot(ToRegister(type), ToStackOffset(payload));
            } else {
                if (payload->isRegister())
                    snapshots_.addSlot(ToStackOffset(type), ToRegister(payload));
                else
                    snapshots_.addSlot(ToStackOffset(type), ToStackOffset(payload));
            }
#elif JS_PUNBOX64
            if (payload->isRegister())
                snapshots_.addSlot(ToRegister(payload));
            else
                snapshots_.addSlot(ToStackOffset(payload));
#endif
            break;
          }
      }
    }

    *startIndex += resumePoint->numOperands();
    return true;
}

bool
CodeGeneratorShared::encode(LSnapshot *snapshot)
{
    if (snapshot->snapshotOffset() != INVALID_SNAPSHOT_OFFSET)
        return true;

    uint32 frameCount = snapshot->mir()->frameCount();

    IonSpew(IonSpew_Snapshots, "Encoding LSnapshot %p (frameCount %u)",
            (void *)snapshot, frameCount);

    SnapshotOffset offset = snapshots_.startSnapshot(frameCount, snapshot->bailoutKind());

    FlattenedMResumePointIter mirOperandIter(snapshot->mir());
    if (!mirOperandIter.init())
        return false;
    
    uint32 startIndex = 0;
    for (MResumePoint **it = mirOperandIter.begin(), **end = mirOperandIter.end();
         it != end;
         ++it)
    {
        MResumePoint *mir = *it;
        MBasicBlock *block = mir->block();
        JSFunction *fun = block->info().fun();
        JSScript *script = block->info().script();
        jsbytecode *pc = mir->pc();
        uint32 exprStack = mir->stackDepth() - block->info().ninvoke();
        snapshots_.startFrame(fun, script, pc, exprStack);
        encodeSlots(snapshot, mir, &startIndex);
        snapshots_.endFrame();
    }

    snapshots_.endSnapshot();

    snapshot->setSnapshotOffset(offset);

    return !snapshots_.oom();
}

bool
CodeGeneratorShared::assignBailoutId(LSnapshot *snapshot)
{
    JS_ASSERT(snapshot->snapshotOffset() != INVALID_SNAPSHOT_OFFSET);

    
    if (!deoptTable_)
        return false;

    JS_ASSERT(frameClass_ != FrameSizeClass::None());

    if (snapshot->bailoutId() != INVALID_BAILOUT_ID)
        return true;

    
    if (bailouts_.length() >= BAILOUT_TABLE_SIZE)
        return false;

    uintN bailoutId = bailouts_.length();
    snapshot->setBailoutId(bailoutId);
    IonSpew(IonSpew_Snapshots, "Assigned snapshot bailout id %u", bailoutId);
    return bailouts_.append(snapshot->snapshotOffset());
}

