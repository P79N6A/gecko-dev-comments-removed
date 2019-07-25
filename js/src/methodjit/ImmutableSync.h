






































#if !defined jsjaeger_imm_sync_h__ && defined JS_METHODJIT
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
        





        bool dataSynced;
        bool typeSynced;
        bool dataClobbered;
        bool typeClobbered;
        RegisterID dataReg;
        RegisterID typeReg;
        bool hasDataReg;
        bool hasTypeReg;
        bool learnedType;
        JSValueTag typeTag;
    };

  public:
    ImmutableSync(JSContext *cx, const FrameState &frame);
    ~ImmutableSync();
    bool init(uint32 nentries);

    void reset(Assembler *masm, Registers avail, uint32 n,
               FrameEntry *bottom);
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
    FrameEntry *bottom;
};

} 
} 

#endif 

