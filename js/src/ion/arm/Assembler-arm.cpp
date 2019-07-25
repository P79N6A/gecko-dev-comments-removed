








































#include "Assembler-arm.h"
#include "jsgcmark.h"

using namespace js;
using namespace js::ion;

void
Assembler::executableCopy(uint8 *buffer)
{
#if 0
    AssemblerARMShared::executableCopy(buffer);

    for (size_t i = 0; i < jumps_.length(); i++) {
        RelativePatch &rp = jumps_[i];
        JSC::ARMAssembler::setRel32(buffer + rp.offset, rp.target);
    }
#endif
}

class RelocationIterator
{
    CompactBufferReader reader_;
    uint32 offset_;

  public:
    RelocationIterator(CompactBufferReader &reader)
      : reader_(reader)
    { }

    bool read() {
        if (!reader_.more())
            return false;
        offset_ = reader_.readUnsigned();
        return true;
    }

    uint32 offset() const {
        return offset_;
    }
};

static inline IonCode *
CodeFromJump(uint8 *jump)
{
#if 0
    uint8 *target = (uint8 *)JSC::ARMAssembler::getRel32Target(jump);
    return IonCode::FromExecutable(target);
#endif
    return NULL;
}

void
Assembler::TraceRelocations(JSTracer *trc, IonCode *code, CompactBufferReader &reader)
{
#if 0
    RelocationIterator iter(reader);
    while (iter.read()) {
        IonCode *child = CodeFromJump(code->raw() + iter.offset());
        MarkIonCode(trc, child, "rel32");
    };
#endif
}

void
Assembler::copyRelocationTable(uint8 *dest)
{
#if 0
    if (relocations_.length())
        memcpy(dest, relocations_.buffer(), relocations_.length());
#endif
}

void
Assembler::trace(JSTracer *trc)
{
#if 0
    for (size_t i = 0; i < jumps_.length(); i++) {
        RelativePatch &rp = jumps_[i];
        if (rp.kind == Relocation::CODE)
            MarkIonCode(trc, IonCode::FromExecutable((uint8 *)rp.target), "masmrel32");
    }
#endif
}

void
Assembler::executableCopy(void *buffer)
{
    masm.executableCopy(buffer);
}

void
Assembler::processDeferredData(IonCode *code, uint8 *data)
{
    for (size_t i = 0; i < data_.length(); i++) {
        DeferredData *deferred = data_[i];
        Bind(code, deferred->label(), data + deferred->offset());
        deferred->copy(code, data + deferred->offset());
    }
}

void
Assembler::processCodeLabels(IonCode *code)
{
    for (size_t i = 0; i < codeLabels_.length(); i++) {
        CodeLabel *label = codeLabels_[i];
        Bind(code, label->dest(), code->raw() + label->src()->offset());
    }
}

Assembler::Condition
Assembler::inverseCondition(Condition cond)
{
#if 0
    switch (cond) {
      case LessThan:
        return GreaterThanOrEqual;
      case LessThanOrEqual:
        return GreaterThan;
      case GreaterThan:
        return LessThanOrEqual;
      case GreaterThanOrEqual:
        return LessThan;
      default:
        JS_NOT_REACHED("Comparisons other than LT, LE, GT, GE not yet supported");
        return Equal;
    }
#endif
    JS_NOT_REACHED("inverse Not Implemented");
    return Equal;

}
