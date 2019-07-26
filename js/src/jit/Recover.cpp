





#include "jit/Recover.h"

#include "jscntxt.h"
#include "jsmath.h"

#include "builtin/TypedObject.h"

#include "jit/IonSpewer.h"
#include "jit/JitFrameIterator.h"
#include "jit/MIR.h"
#include "jit/MIRGraph.h"
#include "jit/VMFunctions.h"

#include "vm/Interpreter.h"
#include "vm/Interpreter-inl.h"

using namespace js;
using namespace js::jit;

bool
MNode::writeRecoverData(CompactBufferWriter &writer) const
{
    MOZ_ASSUME_UNREACHABLE("This instruction is not serializable");
    return false;
}

void
RInstruction::readRecoverData(CompactBufferReader &reader, RInstructionStorage *raw)
{
    uint32_t op = reader.readUnsigned();
    switch (Opcode(op)) {
#   define MATCH_OPCODES_(op)                                           \
      case Recover_##op:                                                \
        static_assert(sizeof(R##op) <= sizeof(RInstructionStorage),     \
                      "Storage space is too small to decode R" #op " instructions."); \
        new (raw->addr()) R##op(reader);                                \
        break;

        RECOVER_OPCODE_LIST(MATCH_OPCODES_)
#   undef DEFINE_OPCODES_

      case Recover_Invalid:
      default:
        MOZ_ASSUME_UNREACHABLE("Bad decoding of the previous instruction?");
        break;
    }
}

bool
MResumePoint::writeRecoverData(CompactBufferWriter &writer) const
{
    writer.writeUnsigned(uint32_t(RInstruction::Recover_ResumePoint));

    MBasicBlock *bb = block();
    JSFunction *fun = bb->info().funMaybeLazy();
    JSScript *script = bb->info().script();
    uint32_t exprStack = stackDepth() - bb->info().ninvoke();

#ifdef DEBUG
    
    
    if (GetIonContext()->cx) {
        uint32_t stackDepth;
        bool reachablePC;
        jsbytecode *bailPC = pc();

        if (mode() == MResumePoint::ResumeAfter)
            bailPC = GetNextPc(pc());

        if (!ReconstructStackDepth(GetIonContext()->cx, script,
                                   bailPC, &stackDepth, &reachablePC))
        {
            return false;
        }

        if (reachablePC) {
            if (JSOp(*bailPC) == JSOP_FUNCALL) {
                
                
                
                MOZ_ASSERT(stackDepth - exprStack <= 1);
            } else if (JSOp(*bailPC) != JSOP_FUNAPPLY &&
                       !IsGetPropPC(bailPC) && !IsSetPropPC(bailPC))
            {
                
                
                
                

                
                
                
                
                
                MOZ_ASSERT(exprStack == stackDepth);
            }
        }
    }
#endif

    
    
    
    
    MOZ_ASSERT(CountArgSlots(script, fun) < SNAPSHOT_MAX_NARGS + 4);

    uint32_t implicit = StartArgSlot(script);
    uint32_t formalArgs = CountArgSlots(script, fun);
    uint32_t nallocs = formalArgs + script->nfixed() + exprStack;

    IonSpew(IonSpew_Snapshots, "Starting frame; implicit %u, formals %u, fixed %u, exprs %u",
            implicit, formalArgs - implicit, script->nfixed(), exprStack);

    uint32_t pcoff = script->pcToOffset(pc());
    IonSpew(IonSpew_Snapshots, "Writing pc offset %u, nslots %u", pcoff, nallocs);
    writer.writeUnsigned(pcoff);
    writer.writeUnsigned(nallocs);
    return true;
}

RResumePoint::RResumePoint(CompactBufferReader &reader)
{
    pcOffset_ = reader.readUnsigned();
    numOperands_ = reader.readUnsigned();
    IonSpew(IonSpew_Snapshots, "Read RResumePoint (pc offset %u, nslots %u)",
            pcOffset_, numOperands_);
}

bool
RResumePoint::recover(JSContext *cx, SnapshotIterator &iter) const
{
    MOZ_ASSUME_UNREACHABLE("This instruction is not recoverable.");
}

bool
MAdd::writeRecoverData(CompactBufferWriter &writer) const
{
    MOZ_ASSERT(canRecoverOnBailout());
    writer.writeUnsigned(uint32_t(RInstruction::Recover_Add));
    writer.writeByte(specialization_ == MIRType_Float32);
    return true;
}

RAdd::RAdd(CompactBufferReader &reader)
{
    isFloatOperation_ = reader.readByte();
}

bool
RAdd::recover(JSContext *cx, SnapshotIterator &iter) const
{
    RootedValue lhs(cx, iter.read());
    RootedValue rhs(cx, iter.read());
    RootedValue result(cx);

    MOZ_ASSERT(!lhs.isObject() && !rhs.isObject());
    if (!js::AddValues(cx, &lhs, &rhs, &result))
        return false;

    
    
    if (isFloatOperation_ && !RoundFloat32(cx, result, &result))
        return false;

    iter.storeInstructionResult(result);
    return true;
}

bool
MBitNot::writeRecoverData(CompactBufferWriter &writer) const
{
    MOZ_ASSERT(canRecoverOnBailout());
    writer.writeUnsigned(uint32_t(RInstruction::Recover_BitNot));
    return true;
}

RBitNot::RBitNot(CompactBufferReader &reader)
{ }

bool
RBitNot::recover(JSContext *cx, SnapshotIterator &iter) const
{
    RootedValue operand(cx, iter.read());

    int32_t result;
    if (!js::BitNot(cx, operand, &result))
        return false;

    RootedValue rootedResult(cx, js::Int32Value(result));
    iter.storeInstructionResult(rootedResult);
    return true;
}

bool
MBitXor::writeRecoverData(CompactBufferWriter &writer) const
{
    MOZ_ASSERT(canRecoverOnBailout());
    writer.writeUnsigned(uint32_t(RInstruction::Recover_BitXor));
    return true;
}

RBitXor::RBitXor(CompactBufferReader &reader)
{ }

bool
RBitXor::recover(JSContext *cx, SnapshotIterator &iter) const
{
    RootedValue lhs(cx, iter.read());
    RootedValue rhs(cx, iter.read());

    int32_t result;
    if (!js::BitXor(cx, lhs, rhs, &result))
        return false;

    RootedValue rootedResult(cx, js::Int32Value(result));
    iter.storeInstructionResult(rootedResult);
    return true;
}

bool
MNewObject::writeRecoverData(CompactBufferWriter &writer) const
{
    MOZ_ASSERT(canRecoverOnBailout());
    writer.writeUnsigned(uint32_t(RInstruction::Recover_NewObject));
    writer.writeByte(templateObjectIsClassPrototype_);
    return true;
}

RNewObject::RNewObject(CompactBufferReader &reader)
{
    templateObjectIsClassPrototype_ = reader.readByte();
}

bool
RNewObject::recover(JSContext *cx, SnapshotIterator &iter) const
{
    RootedObject templateObject(cx, &iter.read().toObject());
    RootedValue result(cx);
    JSObject *resultObject = nullptr;

    
    if (templateObjectIsClassPrototype_)
        resultObject = NewInitObjectWithClassPrototype(cx, templateObject);
    else
        resultObject = NewInitObject(cx, templateObject);

    if (!resultObject)
        return false;

    result.setObject(*resultObject);
    iter.storeInstructionResult(result);
    return true;
}

bool
MNewDerivedTypedObject::writeRecoverData(CompactBufferWriter &writer) const
{
    MOZ_ASSERT(canRecoverOnBailout());
    writer.writeUnsigned(uint32_t(RInstruction::Recover_NewDerivedTypedObject));
    return true;
}

RNewDerivedTypedObject::RNewDerivedTypedObject(CompactBufferReader &reader)
{ }

bool
RNewDerivedTypedObject::recover(JSContext *cx, SnapshotIterator &iter) const
{
    Rooted<SizedTypeDescr *> descr(cx, &iter.read().toObject().as<SizedTypeDescr>());
    Rooted<TypedObject *> owner(cx, &iter.read().toObject().as<TypedObject>());
    int32_t offset = iter.read().toInt32();

    JSObject *obj = TypedObject::createDerived(cx, descr, owner, offset);
    if (!obj)
        return false;

    RootedValue result(cx, ObjectValue(*obj));
    iter.storeInstructionResult(result);
    return true;
}

bool
MBitOr::writeRecoverData(CompactBufferWriter &writer) const
{
    MOZ_ASSERT(canRecoverOnBailout());
    writer.writeUnsigned(uint32_t(RInstruction::Recover_BitOr));
    return true;
}

RBitOr::RBitOr(CompactBufferReader &reader)
{}

bool
RBitOr::recover(JSContext *cx, SnapshotIterator &iter) const
{
    RootedValue lhs(cx, iter.read());
    RootedValue rhs(cx, iter.read());
    int32_t result;
    MOZ_ASSERT(!lhs.isObject() && !rhs.isObject());

    if (!js::BitOr(cx, lhs, rhs, &result))
        return false;

    RootedValue asValue(cx, js::Int32Value(result));
    iter.storeInstructionResult(asValue);
    return true;
}

bool
MUrsh::writeRecoverData(CompactBufferWriter &writer) const
{
    MOZ_ASSERT(canRecoverOnBailout());
    writer.writeUnsigned(uint32_t(RInstruction::Recover_Ursh));
    return true;
}

RUrsh::RUrsh(CompactBufferReader &reader)
{ }

bool
RUrsh::recover(JSContext *cx, SnapshotIterator &iter) const
{
    RootedValue lhs(cx, iter.read());
    RootedValue rhs(cx, iter.read());
    MOZ_ASSERT(!lhs.isObject() && !rhs.isObject());

    RootedValue result(cx);
    if (!js::UrshOperation(cx, lhs, rhs, &result))
        return false;

    iter.storeInstructionResult(result);
    return true;
}
