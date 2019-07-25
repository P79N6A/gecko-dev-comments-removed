






































#if !defined jsjaeger_framestate_h__ && defined JS_METHODJIT
#define jsjaeger_framestate_h__

#include "jsanalyze.h"
#include "jsapi.h"
#include "methodjit/MachineRegs.h"
#include "methodjit/FrameEntry.h"
#include "CodeGenIncludes.h"
#include "ImmutableSync.h"
#include "jscompartment.h"

namespace js {
namespace mjit {

struct Uses {
    explicit Uses(uint32 nuses)
      : nuses(nuses)
    { }
    uint32 nuses;
};

struct Changes {
    explicit Changes(uint32 nchanges)
      : nchanges(nchanges)
    { }
    uint32 nchanges;
};

class StubCompiler;






































class FrameState
{
    friend class ImmutableSync;

    typedef JSC::MacroAssembler::RegisterID RegisterID;
    typedef JSC::MacroAssembler::FPRegisterID FPRegisterID;
    typedef JSC::MacroAssembler::Address Address;
    typedef JSC::MacroAssembler::Jump Jump;
    typedef JSC::MacroAssembler::Imm32 Imm32;

    static const uint32 InvalidIndex = 0xFFFFFFFF;

    struct Tracker {
        Tracker()
          : entries(NULL), nentries(0)
        { }

        void add(FrameEntry *fe) {
            entries[nentries++] = fe;
        }

        void reset() {
            nentries = 0;
        }

        FrameEntry * operator [](uint32 n) const {
            JS_ASSERT(n < nentries);
            return entries[n];
        }

        FrameEntry **entries;
        uint32 nentries;
    };

    










    struct RegisterState {
        RegisterState() : fe_(NULL), save_(NULL)
        { }

        RegisterState(FrameEntry *fe, RematInfo::RematType type)
          : fe_(fe), save_(NULL), type_(type)
        {
            JS_ASSERT(!save_);
        }

        bool isPinned() const {
            assertConsistency();
            return !!save_;
        }

        void assertConsistency() const {
            JS_ASSERT_IF(fe_, !save_);
            JS_ASSERT_IF(save_, !fe_);
        }

        FrameEntry *fe() const {
            assertConsistency();
            return fe_;
        }

        RematInfo::RematType type() const {
            assertConsistency();
            return type_;
        }

        FrameEntry *usedBy() const {
            if (fe_)
                return fe_;
            return save_;
        }

        void associate(FrameEntry *fe, RematInfo::RematType type) {
            JS_ASSERT(!fe_);
            JS_ASSERT(!save_);

            fe_ = fe;
            type_ = type;
            JS_ASSERT(!save_);
        }

        
        void reassociate(FrameEntry *fe) {
            assertConsistency();
            JS_ASSERT(fe);

            fe_ = fe;
        }

        
        void forget() {
            JS_ASSERT(fe_);
            fe_ = NULL;
            JS_ASSERT(!save_);
        }

        void pin() {
            JS_ASSERT(fe_ != NULL);
            assertConsistency();
            save_ = fe_;
            fe_ = NULL;
        }

        void unpin() {
            JS_ASSERT(save_ != NULL);
            assertConsistency();
            fe_ = save_;
            save_ = NULL;
        }

        void unpinUnsafe() {
            assertConsistency();
            save_ = NULL;
        }

      private:
        
        FrameEntry *fe_;

        
        FrameEntry *save_;
        
        
        RematInfo::RematType type_;
    };

    FrameState *thisFromCtor() { return this; }
  public:
    FrameState(JSContext *cx, JSScript *script, JSFunction *fun,
               Compiler &cc, Assembler &masm, StubCompiler &stubcc,
               analyze::LifetimeScript &liveness);
    ~FrameState();
    bool init();

    


    inline void pushSynced(JSValueType knownType, types::TypeSet *typeSet = NULL);

    


    inline void pushSynced(JSValueType knownType, RegisterID reg, types::TypeSet *typeSet = NULL);

    


    inline void push(const Value &v, types::TypeSet *typeSet = NULL);

    


    inline void push(Address address, JSValueType knownType, types::TypeSet *typeSet = NULL);

    


    inline void pushTypedPayload(JSValueType type, RegisterID payload, types::TypeSet *typeSet = NULL);

    




    inline FPRegisterID pushRegs(RegisterID type, RegisterID data, JSValueType knownType, types::TypeSet *typeSet);

    
    void pushDouble(FPRegisterID fpreg);
    void pushDouble(Address address);

    
    void ensureDouble(FrameEntry *fe);

    
    void forgetKnownDouble(FrameEntry *fe);

    








    inline void pushUntypedPayload(JSValueType type, RegisterID payload);

    




    inline void pushUntypedValue(const Value &value);

    










    inline void pushNumber(RegisterID payload, bool asInt32 = false);

    





    inline void pushInt32(RegisterID payload);

    



    inline void pushInitializerObject(RegisterID payload, bool array, JSObject *baseobj);

    


    inline void pop();

    



    inline void popn(uint32 n);

    


    inline bool haveSameBacking(FrameEntry *lhs, FrameEntry *rhs);

    


    inline void enterBlock(uint32 n);
    inline void leaveBlock(uint32 n);

    
    
    void pushLocal(uint32 n, JSValueType knownType, types::TypeSet *typeSet);
    void pushArg(uint32 n, JSValueType knownType, types::TypeSet *typeSet);
    void pushCallee();
    void pushThis();
    inline void learnThisIsObject();

    inline FrameEntry *getLocal(uint32 slot);
    inline FrameEntry *getArg(uint32 slot);

    




    inline RegisterID tempRegForType(FrameEntry *fe);

    






    inline RegisterID tempRegForType(FrameEntry *fe, RegisterID fallback);

    




    inline RegisterID tempRegForData(FrameEntry *fe);
    inline FPRegisterID tempFPRegForData(FrameEntry *fe);

    


    inline RegisterID tempRegInMaskForData(FrameEntry *fe, uint32 mask);

    



    inline RegisterID tempRegForData(FrameEntry *fe, RegisterID reg, Assembler &masm) const;

    



    inline void convertInt32ToDouble(Assembler &masm, FrameEntry *fe,
                                     FPRegisterID fpreg) const;

    


    inline bool peekTypeInRegister(FrameEntry *fe) const;

    








    RegisterID ownRegForData(FrameEntry *fe);

    








    RegisterID ownRegForType(FrameEntry *fe);

    



    RegisterID copyDataIntoReg(FrameEntry *fe);
    void copyDataIntoReg(FrameEntry *fe, RegisterID exact);
    RegisterID copyDataIntoReg(Assembler &masm, FrameEntry *fe);

    



    RegisterID copyTypeIntoReg(FrameEntry *fe);

    





    RegisterID copyInt32ConstantIntoReg(FrameEntry *fe);
    RegisterID copyInt32ConstantIntoReg(Assembler &masm, FrameEntry *fe);

    



    void pinEntry(FrameEntry *fe, ValueRemat &vr);

    
    void unpinEntry(const ValueRemat &vr);

    
    void ensureValueSynced(Assembler &masm, FrameEntry *fe, const ValueRemat &vr);

    struct BinaryAlloc {
        MaybeRegisterID lhsType;
        MaybeRegisterID lhsData;
        MaybeRegisterID rhsType;
        MaybeRegisterID rhsData;
        MaybeRegisterID extraFree;
        RegisterID result;  
        FPRegisterID lhsFP; 
        FPRegisterID rhsFP; 
        bool resultHasRhs;  
        bool lhsNeedsRemat; 
        bool rhsNeedsRemat; 
        bool undoResult;    
    };

    







    void allocForBinary(FrameEntry *lhs, FrameEntry *rhs, JSOp op, BinaryAlloc &alloc,
                        bool resultNeeded = true);

    



    void rematBinary(FrameEntry *lhs, FrameEntry *rhs, const BinaryAlloc &alloc, Assembler &masm);

    
    void ensureFullRegs(FrameEntry *fe, MaybeRegisterID *typeReg, MaybeRegisterID *dataReg);

    







    void allocForSameBinary(FrameEntry *fe, JSOp op, BinaryAlloc &alloc);

    
    inline void loadDouble(FrameEntry *fe, FPRegisterID fpReg, Assembler &masm) const;

    



    inline void loadDouble(RegisterID type, RegisterID data, FrameEntry *fe, FPRegisterID fpReg,
                           Assembler &masm) const;

    




    inline bool shouldAvoidTypeRemat(FrameEntry *fe);

    




    inline bool shouldAvoidDataRemat(FrameEntry *fe);

    



    inline void freeReg(AnyRegisterID reg);

    




    inline RegisterID allocReg();
    inline FPRegisterID allocFPReg();

    


    inline AnyRegisterID allocReg(uint32 mask);

    


    void takeReg(AnyRegisterID reg);

    


    inline FrameEntry *peek(int32 depth);

    



    void storeTo(FrameEntry *fe, Address address, bool popHint = false);

    



    void loadForReturn(FrameEntry *fe, RegisterID typeReg, RegisterID dataReg, RegisterID tempReg);
    void loadThisForReturn(RegisterID typeReg, RegisterID dataReg, RegisterID tempReg);

    



    void storeLocal(uint32 n, JSValueType type, types::TypeSet *typeSet,
                    bool popGuaranteed = false, bool fixedType = false);
    void storeArg(uint32 n, JSValueType type, types::TypeSet *typeSet,
                  bool popGuaranteed = false);
    void storeTop(FrameEntry *target, JSValueType type, types::TypeSet *typeSet,
                  bool popGuaranteed);

    


    void merge(Assembler &masm, Changes changes) const;

    


    void sync(Assembler &masm, Uses uses) const;

    



    void syncAndKill(Registers kill, Uses uses, Uses ignored);
    void syncAndKill(Registers kill, Uses uses) { syncAndKill(kill, uses, Uses(0)); }
    void syncAndKill(Uses uses) { syncAndKill(Registers(Registers::AvailAnyRegs), uses, Uses(0)); }

    
    void syncAndKillEverything() {
        syncAndKill(Registers(Registers::AvailAnyRegs), Uses(frameSlots()));
    }

    



    void forgetEverything();

    void syncAndForgetEverything()
    {
        syncAndKillEverything();
        forgetEverything();
    }

    


    void discardFrame();

    





    bool syncForBranch(jsbytecode *target, Uses uses);

    
    bool discardForJoin(jsbytecode *target, uint32 stackDepth);

    
    bool consistentRegisters(jsbytecode *target);

    



    void prepareForJump(jsbytecode *target, Assembler &masm, bool synced);

    



    inline void learnType(FrameEntry *fe, JSValueType type, bool unsync = true);
    inline void learnType(FrameEntry *fe, JSValueType type, RegisterID payload);

    


    inline void forgetType(FrameEntry *fe);

    


    void discardFe(FrameEntry *fe);

    



    inline Jump testNull(Assembler::Condition cond, FrameEntry *fe);

    



    inline Jump testUndefined(Assembler::Condition cond, FrameEntry *fe);

    



    inline Jump testInt32(Assembler::Condition cond, FrameEntry *fe);

    



    inline Jump testDouble(Assembler::Condition cond, FrameEntry *fe);

    



    inline Jump testBoolean(Assembler::Condition cond, FrameEntry *fe);

    



    inline Jump testString(Assembler::Condition cond, FrameEntry *fe);

    



    inline Jump testObject(Assembler::Condition cond, FrameEntry *fe);

    



    inline Jump testPrimitive(Assembler::Condition cond, FrameEntry *fe);

    





    inline void pinReg(AnyRegisterID reg) { regstate(reg).pin(); }

    


    inline void unpinReg(AnyRegisterID reg) { regstate(reg).unpin(); }

    


    inline void unpinKilledReg(RegisterID reg);

    
    MaybeRegisterID maybePinData(FrameEntry *fe);
    MaybeRegisterID maybePinType(FrameEntry *fe);
    void maybeUnpinReg(MaybeRegisterID reg);

    


    inline void dup();

    


    inline void dup2();

    


    inline void dupAt(int32 n);

    


    inline void syncAt(int32 n);

    



    inline void giveOwnRegs(FrameEntry *fe);

    uint32 stackDepth() const { return sp - spBase; }

    
    
    
    
    
    uint32 frameSlots() const { return uint32(sp - entries); }

    
    uint32 localSlots() const { return uint32(sp - locals); }

#ifdef DEBUG
    void assertValidRegisterState() const;
#endif

    
    
    
    
    Address addressOf(const FrameEntry *fe) const;

    
    
    
    
    Address addressForDataRemat(const FrameEntry *fe) const;

    inline StateRemat dataRematInfo(const FrameEntry *fe) const;

    





    inline void eviscerate(FrameEntry *fe);

    



    void shimmy(uint32 n);

    




    void shift(int32 n);

    
    inline void setClosedVar(uint32 slot);
    inline void setClosedArg(uint32 slot);

    inline void setInTryBlock(bool inTryBlock) {
        this->inTryBlock = inTryBlock;
    }

    void setAnalysis(analyze::Script *analysis) { this->analysis = analysis; }

    inline uint32 regsInUse() const { return Registers::AvailRegs & ~freeRegs.freeMask; }

    bool pushLoop(jsbytecode *head, Jump entry, jsbytecode *entryTarget);
    void popLoop(jsbytecode *head, Jump *pentry, jsbytecode **pentryTarget);

    void setPC(jsbytecode *PC) { this->PC = PC; }

    struct StubJoin {
        unsigned index;
        bool script;
    };

    void addJoin(unsigned index, bool script) {
        if (activeLoop) {
            StubJoin r;
            r.index = index;
            r.script = script;
            loopJoins.append(r);
        }
    }

  private:
    inline AnyRegisterID allocAndLoadReg(FrameEntry *fe, bool fp, RematInfo::RematType type);
    inline void forgetReg(AnyRegisterID reg);
    AnyRegisterID evictSomeReg(uint32 mask);
    void evictReg(AnyRegisterID reg);
    inline FrameEntry *rawPush();
    inline void addToTracker(FrameEntry *fe);

    
    inline void ensureFeSynced(const FrameEntry *fe, Assembler &masm) const;
    inline void ensureTypeSynced(const FrameEntry *fe, Assembler &masm) const;
    inline void ensureDataSynced(const FrameEntry *fe, Assembler &masm) const;

    
    inline void syncFe(FrameEntry *fe);
    inline void syncType(FrameEntry *fe);
    inline void syncData(FrameEntry *fe);
    inline void syncAndForgetFe(FrameEntry *fe);

    inline FrameEntry *getOrTrack(uint32 index);
    inline FrameEntry *getCallee();
    inline FrameEntry *getThis();
    inline void forgetAllRegs(FrameEntry *fe);
    inline void swapInTracker(FrameEntry *lhs, FrameEntry *rhs);
    void pushCopyOf(uint32 index);
#if defined JS_NUNBOX32
    void syncFancy(Assembler &masm, Registers avail, FrameEntry *resumeAt,
                   FrameEntry *bottom) const;
#endif
    inline bool tryFastDoubleLoad(FrameEntry *fe, FPRegisterID fpReg, Assembler &masm) const;
    void resetInternalState();

    







    FrameEntry *uncopy(FrameEntry *original);
    FrameEntry *walkTrackerForUncopy(FrameEntry *original);
    FrameEntry *walkFrameForUncopy(FrameEntry *original);

    
    bool hasOnlyCopy(FrameEntry *backing, FrameEntry *fe);

    
    bool isEntryCopied(FrameEntry *fe) const;

    



    void forgetEntry(FrameEntry *fe);

    FrameEntry *entryFor(uint32 index) const {
        JS_ASSERT(entries[index].isTracked());
        return &entries[index];
    }

    uint32 indexOf(int32 depth) const {
        JS_ASSERT(uint32((sp + depth) - entries) < feLimit());
        return uint32((sp + depth) - entries);
    }
    uint32 indexOfFe(FrameEntry *fe) const {
        JS_ASSERT(uint32(fe - entries) < feLimit());
        return uint32(fe - entries);
    }
    uint32 feLimit() const { return script->nslots + nargs + 2; }

    inline bool isClosedVar(uint32 slot) const;
    inline bool isClosedArg(uint32 slot) const;

    RegisterState & regstate(AnyRegisterID reg) {
        JS_ASSERT(reg.reg_ < Registers::TotalAnyRegisters);
        return regstate_[reg.reg_];
    }

    const RegisterState & regstate(AnyRegisterID reg) const {
        JS_ASSERT(reg.reg_ < Registers::TotalAnyRegisters);
        return regstate_[reg.reg_];
    }

    AnyRegisterID bestEvictReg(uint32 mask, bool includePinned) const;

    inline analyze::Lifetime * variableLive(FrameEntry *fe, jsbytecode *pc) const;
    inline bool binaryEntryLive(FrameEntry *fe) const;
    RegisterAllocation * computeAllocation(jsbytecode *target);
    void relocateReg(AnyRegisterID reg, RegisterAllocation *alloc, Uses uses);

    bool isArg(FrameEntry *fe) const { return fun && fe >= args && fe - args < fun->nargs; }
    bool isLocal(FrameEntry *fe) const { return fe >= locals && fe - locals < script->nfixed; }

    inline void clearLoopReg(AnyRegisterID reg);
    void setLoopReg(AnyRegisterID reg, FrameEntry *fe);
    void flushLoopJoins();

#ifdef DEBUG
    const char * entryName(FrameEntry *fe) const;
    void dumpAllocation(RegisterAllocation *alloc);
#else
    const char * entryName(FrameEntry *fe) const { return NULL; }
#endif

  private:
    JSContext *cx;
    JSScript *script;
    JSFunction *fun;
    uint32 nargs;
    Assembler &masm;
    StubCompiler &stubcc;

    
    Registers freeRegs;

    
    FrameEntry *entries;

    FrameEntry *callee_;
    FrameEntry *this_;

    
    FrameEntry *args;

    
    FrameEntry *locals;

    
    FrameEntry *spBase;

    
    FrameEntry *sp;

    
    Tracker tracker;

    



    RegisterState regstate_[Registers::TotalAnyRegisters];

    struct LoopState
    {
        LoopState *outer;
        jsbytecode *head;
        RegisterAllocation *alloc;

        




        Jump entry;
        jsbytecode *entryTarget;
    };

    
    LoopState *activeLoop;

    
    Registers loopRegs;

    
    Vector<StubJoin,16,CompilerAllocPolicy> loopJoins;

    struct StubJoinPatch {
        StubJoin join;
        Address address;
        AnyRegisterID reg;
    };

    
    Vector<StubJoinPatch,16,CompilerAllocPolicy> loopPatches;

    analyze::Script *analysis;

    



    analyze::LifetimeScript &liveness;
    jsbytecode *PC;

#if defined JS_NUNBOX32
    mutable ImmutableSync reifier;
#endif

    JSPackedBool *closedVars;
    JSPackedBool *closedArgs;
    bool eval;
    bool usesArguments;
    bool inTryBlock;
};



















struct RegisterAllocation {
  private:
    typedef JSC::MacroAssembler::RegisterID RegisterID;
    typedef JSC::MacroAssembler::FPRegisterID FPRegisterID;

    
    static const uint32 UNASSIGNED_REGISTER = 0xffffffff;

    






    static const uint32 LOOP_REGISTER = 0xfffffffe;

    



    uint32 regstate_[Registers::TotalAnyRegisters];

    
    static const uint32 SYNCED = 0x80000000;

    uint32 & regstate(AnyRegisterID reg) {
        JS_ASSERT(reg.reg_ < Registers::TotalAnyRegisters);
        return regstate_[reg.reg_];
    }

  public:
    RegisterAllocation(bool forLoop) {
        uint32 entry = forLoop ? (uint32) LOOP_REGISTER : (uint32) UNASSIGNED_REGISTER;
        for (unsigned i = 0; i < Registers::TotalAnyRegisters; i++) {
            AnyRegisterID reg = AnyRegisterID::fromRaw(i);
            bool avail = Registers::maskReg(reg) & Registers::AvailAnyRegs;
            regstate_[i] = avail ? entry : UNASSIGNED_REGISTER;
        }
    }

    bool assigned(AnyRegisterID reg) {
        return regstate(reg) != UNASSIGNED_REGISTER && regstate(reg) != LOOP_REGISTER;
    }

    bool loop(AnyRegisterID reg) {
        return regstate(reg) == LOOP_REGISTER;
    }

    bool synced(AnyRegisterID reg) {
        JS_ASSERT(assigned(reg));
        return regstate(reg) & SYNCED;
    }

    uint32 slot(AnyRegisterID reg) {
        JS_ASSERT(assigned(reg));
        return regstate(reg) & ~SYNCED;
    }

    void set(AnyRegisterID reg, uint32 slot, bool synced) {
        JS_ASSERT(slot != LOOP_REGISTER && slot != UNASSIGNED_REGISTER);
        regstate(reg) = slot | (synced ? SYNCED : 0);
    }

    void setUnassigned(AnyRegisterID reg) {
        regstate(reg) = UNASSIGNED_REGISTER;
    }

    bool synced() {
        for (unsigned i = 0; i < Registers::TotalAnyRegisters; i++) {
            if (assigned(AnyRegisterID::fromRaw(i)))
                return false;
        }
        return true;
    }

    void clearLoops() {
        for (unsigned i = 0; i < Registers::TotalAnyRegisters; i++) {
            AnyRegisterID reg = AnyRegisterID::fromRaw(i);
            if (loop(reg))
                setUnassigned(reg);
        }
    }

    bool hasAnyReg(uint32 n) {
        for (unsigned i = 0; i < Registers::TotalAnyRegisters; i++) {
            AnyRegisterID reg = AnyRegisterID::fromRaw(i);
            if (assigned(reg) && slot(reg) == n)
                return true;
        }
        return false;
    }
};

class AutoPreserveAcrossSyncAndKill;

} 
} 

#endif 

