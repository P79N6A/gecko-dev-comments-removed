






































#if !defined jsjaeger_framestate_h__ && defined JS_METHODJIT
#define jsjaeger_framestate_h__

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
    FrameState(JSContext *cx, JSScript *script, JSFunction *fun, Assembler &masm);
    ~FrameState();
    bool init();

    


    inline void pushSynced();

    


    inline void pushSyncedType(JSValueType type);

    


    inline void pushSynced(JSValueType type, RegisterID reg);

    


    inline void push(const Value &v);

    


    inline void push(Address address);

    


    inline void pushTypedPayload(JSValueType type, RegisterID payload);

    


    inline void pushRegs(RegisterID type, RegisterID data);

    








    inline void pushUntypedPayload(JSValueType type, RegisterID payload);

    




    inline void pushUntypedValue(const Value &value);

    










    inline void pushNumber(MaybeRegisterID payload, bool asInt32 = false);

    





    inline void pushInt32(RegisterID payload);

    



    inline void pushInitializerObject(RegisterID payload, bool array, JSObject *baseobj);

    


    inline void pop();

    



    inline void popn(uint32 n);

    


    inline bool haveSameBacking(FrameEntry *lhs, FrameEntry *rhs);

    


    inline void enterBlock(uint32 n);
    inline void leaveBlock(uint32 n);

    
    
    void pushLocal(uint32 n);
    void pushArg(uint32 n);
    void pushCallee();
    void pushThis();
    inline void learnThisIsObject();

    




    inline RegisterID tempRegForType(FrameEntry *fe);

    






    inline RegisterID tempRegForType(FrameEntry *fe, RegisterID fallback);

    




    inline RegisterID tempRegForData(FrameEntry *fe);

    


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

    



    FPRegisterID copyEntryIntoFPReg(FrameEntry *fe, FPRegisterID fpreg);
    FPRegisterID copyEntryIntoFPReg(Assembler &masm, FrameEntry *fe,
                                    FPRegisterID fpreg);

    



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
        bool resultHasRhs;  
        bool lhsNeedsRemat; 
        bool rhsNeedsRemat; 
    };

    







    void allocForBinary(FrameEntry *lhs, FrameEntry *rhs, JSOp op, BinaryAlloc &alloc,
                        bool resultNeeded = true);

    
    void ensureFullRegs(FrameEntry *fe, MaybeRegisterID *typeReg, MaybeRegisterID *dataReg);

    







    void allocForSameBinary(FrameEntry *fe, JSOp op, BinaryAlloc &alloc);

    
    inline void loadDouble(FrameEntry *fe, FPRegisterID fpReg, Assembler &masm) const;

    



    inline void loadDouble(RegisterID type, RegisterID data, FrameEntry *fe, FPRegisterID fpReg,
                           Assembler &masm) const;

    




    inline bool shouldAvoidTypeRemat(FrameEntry *fe);

    




    inline bool shouldAvoidDataRemat(FrameEntry *fe);

    



    inline void freeReg(RegisterID reg);

    




    inline RegisterID allocReg();

    


    inline RegisterID allocReg(uint32 mask);

    


    void takeReg(RegisterID reg);

    


    inline FrameEntry *peek(int32 depth);

    



    void storeTo(FrameEntry *fe, Address address, bool popHint = false);

    



    void loadForReturn(FrameEntry *fe, RegisterID typeReg, RegisterID dataReg, RegisterID tempReg);
    void loadThisForReturn(RegisterID typeReg, RegisterID dataReg, RegisterID tempReg);

    


    void storeLocal(uint32 n, bool popGuaranteed = false, bool typeChange = true);
    void storeArg(uint32 n, bool popGuaranteed = false);
    void storeTop(FrameEntry *target, bool popGuaranteed = false, bool typeChange = true);
    void finishStore(FrameEntry *fe, bool closed);

    


    void merge(Assembler &masm, Changes changes) const;

    


    void sync(Assembler &masm, Uses uses) const;

    



    void syncAndKill(Registers kill, Uses uses, Uses ignored);
    void syncAndKill(Registers kill, Uses uses) { syncAndKill(kill, uses, Uses(0)); }

    
    void syncAndKillEverything() {
        syncAndKill(Registers(Registers::AvailRegs), Uses(frameSlots()));
    }

    




    inline void syncAndForgetEverything(uint32 newStackDepth);

    



    void syncAndForgetEverything();

    



    void forgetEverything();

    


    void discardFrame();

    


    inline void learnType(FrameEntry *fe, JSValueType type);

    


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

    





    inline void pinReg(RegisterID reg);

    


    inline void unpinReg(RegisterID reg);

    


    inline void unpinKilledReg(RegisterID reg);

    
    MaybeRegisterID maybePinData(FrameEntry *fe);
    MaybeRegisterID maybePinType(FrameEntry *fe);
    void maybeUnpinReg(MaybeRegisterID reg);

    


    inline void dup();

    


    inline void dup2();

    


    inline void dupAt(int32 n);

    



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

    inline uint32 regsInUse() const { return Registers::AvailRegs & ~freeRegs.freeMask; }

  private:
    inline RegisterID allocReg(FrameEntry *fe, RematInfo::RematType type);
    inline void forgetReg(RegisterID reg);
    RegisterID evictSomeReg(uint32 mask);
    void evictReg(RegisterID reg);
    inline FrameEntry *rawPush();
    inline void addToTracker(FrameEntry *fe);

    
    inline void ensureFeSynced(const FrameEntry *fe, Assembler &masm) const;
    inline void ensureTypeSynced(const FrameEntry *fe, Assembler &masm) const;
    inline void ensureDataSynced(const FrameEntry *fe, Assembler &masm) const;

    
    inline void syncFe(FrameEntry *fe);
    inline void syncType(FrameEntry *fe);
    inline void syncData(FrameEntry *fe);

    inline FrameEntry *getOrTrack(uint32 index);
    inline FrameEntry *getLocal(uint32 slot);
    inline FrameEntry *getArg(uint32 slot);
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

    



    void forgetEntry(FrameEntry *fe);

    FrameEntry *entryFor(uint32 index) const {
        JS_ASSERT(entries[index].isTracked());
        return &entries[index];
    }

    RegisterID evictSomeReg() { return evictSomeReg(Registers::AvailRegs); }
    uint32 indexOf(int32 depth) const {
        JS_ASSERT(uint32((sp + depth) - entries) < feLimit());
        return uint32((sp + depth) - entries);
    }
    uint32 indexOfFe(FrameEntry *fe) const {
        JS_ASSERT(uint32(fe - entries) < feLimit());
        return uint32(fe - entries);
    }
    uint32 feLimit() const { return script->nslots + nargs + 2; }

    inline bool isClosedVar(uint32 slot);
    inline bool isClosedArg(uint32 slot);

  private:
    JSContext *cx;
    JSScript *script;
    JSFunction *fun;
    uint32 nargs;
    Assembler &masm;

    
    Registers freeRegs;

    
    FrameEntry *entries;

    FrameEntry *callee_;
    FrameEntry *this_;

    
    FrameEntry *args;

    
    FrameEntry *locals;

    
    FrameEntry *spBase;

    
    FrameEntry *sp;

    
    Tracker tracker;

    



    RegisterState regstate[Assembler::TotalRegisters];

#if defined JS_NUNBOX32
    mutable ImmutableSync reifier;
#endif

    JSPackedBool *closedVars;
    JSPackedBool *closedArgs;
    bool eval;
    bool usesArguments;
    bool inTryBlock;
};

class AutoPreserveAcrossSyncAndKill;

} 
} 

#endif 

