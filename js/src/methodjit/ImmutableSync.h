






































#if !defined jsjaeger_imm_sync_h__ && defined JS_METHODJIT && defined JS_NUNBOX32
#define jsjaeger_imm_sync_h__

#include "methodjit/MachineRegs.h"
#include "methodjit/FrameEntry.h"
#include "CodeGenIncludes.h"

namespace js {
namespace mjit {

class FrameState;










class ImmutableSync
{
    typedef JSC::MacroAssembler::RegisterID RegisterID;
    typedef JSC::MacroAssembler::Address Address;

    struct SyncEntry {
        





        uint32 generation;
        bool dataClobbered;
        bool typeClobbered;
        bool hasDataReg;
        bool hasTypeReg;
        bool learnedType;
        RegisterID dataReg;
        RegisterID typeReg;
        JSValueType type;

        void reset(uint32 gen) {
            dataClobbered = false;
            typeClobbered = false;
            hasDataReg = false;
            hasTypeReg = false;
            learnedType = false;
            generation = gen;
        }
    };

  public:
    ImmutableSync(JSContext *cx, const FrameState &frame);
    ~ImmutableSync();
    bool init(uint32 nentries);

    void reset(Assembler *masm, Registers avail, FrameEntry *top, FrameEntry *bottom);
    void sync(FrameEntry *fe);

  private:
    void syncCopy(FrameEntry *fe);
    void syncNormal(FrameEntry *fe);
    RegisterID ensureDataReg(FrameEntry *fe, SyncEntry &e);
    RegisterID ensureTypeReg(FrameEntry *fe, SyncEntry &e);
    RegisterID allocReg();

    inline SyncEntry &entryFor(FrameEntry *fe);

    bool shouldSyncType(FrameEntry *fe, SyncEntry &e);
    bool shouldSyncData(FrameEntry *fe, SyncEntry &e);

  private:
    JSContext *cx;
    SyncEntry *entries;
    const FrameState &frame;
    uint32 nentries;
    Registers avail;
    Assembler *masm;
    SyncEntry *regs[Assembler::TotalRegisters];
    FrameEntry *top;
    FrameEntry *bottom;
    uint32 generation;
};

} 
} 

#endif 

