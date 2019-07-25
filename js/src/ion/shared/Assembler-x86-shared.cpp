








































#include "ion/IonMacroAssembler.h"
#include "jsgcmark.h"

using namespace js;
using namespace js::ion;

void
AssemblerX86Shared::copyRelocationTable(uint8 *dest)
{
    if (relocations_.length())
        memcpy(dest, relocations_.buffer(), relocations_.length());
}

void
AssemblerX86Shared::trace(JSTracer *trc)
{
    for (size_t i = 0; i < jumps_.length(); i++) {
        RelativePatch &rp = jumps_[i];
        if (rp.kind == Relocation::CODE)
            MarkIonCode(trc, IonCode::FromExecutable((uint8 *)rp.target), "masmrel32");
    }
}

void
AssemblerX86Shared::executableCopy(void *buffer)
{
    masm.executableCopy(buffer);
}

void
AssemblerX86Shared::processDeferredData(uint8 *code, uint8 *data)
{
    for (size_t i = 0; i < data_.length(); i++) {
        DeferredData *deferred = data_[i];
        bind(deferred->label(), code, data + deferred->offset());
        deferred->copy(code, data + deferred->offset());
    }
}

