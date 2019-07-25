






































#if !defined jsjaeger_framestate_h__ && defined JS_METHODJIT
#define jsjaeger_framestate_h__

#include "jsapi.h"
#include "methodjit/MachineRegs.h"
#include "methodjit/FrameEntry.h"
#include "CodeGenIncludes.h"
#include "ImmutableSync.h"

namespace js {
namespace mjit {

struct StateRemat {
    typedef JSC::MacroAssembler::RegisterID RegisterID;
    union {
        RegisterID reg : 31;
        uint32 offset  : 31;
    };
    bool inReg : 1;
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
        RegisterState()
        { }

        RegisterState(FrameEntry *fe, RematInfo::RematType type, bool weak)
          : fe(fe), type(type), weak(weak)
        { }

        
        FrameEntry *fe;

        
        FrameEntry *save;
        
        
        RematInfo::RematType type;

        
        bool weak;
    };

  public:
    FrameState(JSContext *cx, JSScript *script, Assembler &masm);
    ~FrameState();
    bool init(uint32 nargs);

    


    inline void pushSynced();

    


    inline void pushSyncedType(JSValueTag tag);

    


    inline void pushSynced(JSValueTag tag, RegisterID reg);

    


    inline void push(const Value &v);

    


    inline void push(Address address);

    


    inline void pushTypedPayload(JSValueTag tag, RegisterID payload);

    


    inline void pushRegs(RegisterID type, RegisterID data);

    








    inline void pushUntypedPayload(JSValueTag tag, RegisterID payload,
                                   bool popGuaranteed = false);

    


    inline void pop();

    



    inline void popn(uint32 n);

    


    inline void enterBlock(uint32 n);
    inline void leaveBlock(uint32 n);

    


    void pushLocal(uint32 n);

    




    inline RegisterID predictRegForType(FrameEntry *fe);

    




    inline RegisterID tempRegForType(FrameEntry *fe);

    




    inline RegisterID tempRegForData(FrameEntry *fe);

    


    inline RegisterID tempRegForData(FrameEntry *fe, RegisterID reg);

    



    inline RegisterID tempRegForConstant(FrameEntry *fe);

    



    inline void emitLoadTypeTag(FrameEntry *fe, RegisterID reg) const;
    inline void emitLoadTypeTag(Assembler &masm, FrameEntry *fe, RegisterID reg) const;

    



    inline void convertInt32ToDouble(Assembler &masm, FrameEntry *fe,
                                     FPRegisterID fpreg) const;

    


    inline bool peekTypeInRegister(FrameEntry *fe) const;

    








    RegisterID ownRegForData(FrameEntry *fe);

    








    RegisterID ownRegForType(FrameEntry *fe);

    



    RegisterID copyDataIntoReg(FrameEntry *fe);
    RegisterID copyDataIntoReg(Assembler &masm, FrameEntry *fe);

    



    FPRegisterID copyEntryIntoFPReg(FrameEntry *fe, FPRegisterID fpreg);
    FPRegisterID copyEntryIntoFPReg(Assembler &masm, FrameEntry *fe,
                                    FPRegisterID fpreg);

    



    RegisterID copyTypeIntoReg(FrameEntry *fe);

    




    inline bool shouldAvoidTypeRemat(FrameEntry *fe);

    




    inline bool shouldAvoidDataRemat(FrameEntry *fe);

    



    inline void freeReg(RegisterID reg);

    




    inline RegisterID allocReg();

    


    inline RegisterID allocReg(uint32 mask);

    


    void takeReg(RegisterID reg);

    


    inline FrameEntry *peek(int32 depth);

    



    void storeTo(FrameEntry *fe, Address address, bool popHint);

    


    void storeLocal(uint32 n, bool popGuaranteed = false, bool typeChange = true);

    


    void merge(Assembler &masm, uint32 ivD) const;

    


    void sync(Assembler &masm) const;

    



    void syncAndKill(uint32 mask); 

    


    void syncAllRegs(uint32 mask);

    


    void syncForCall(uint32 argc);

    




    inline void forgetEverything(uint32 newStackDepth);

    



    void forgetEverything();


    


    void throwaway();

    


    inline void learnType(FrameEntry *fe, JSValueTag tag);

    


    inline void forgetType(FrameEntry *fe);

    



    inline Jump testInt32(Assembler::Condition cond, FrameEntry *fe);

    



    inline Jump testDouble(Assembler::Condition cond, FrameEntry *fe);

    



    inline Jump testBoolean(Assembler::Condition cond, FrameEntry *fe);

    



    inline Jump testNonFunObj(Assembler::Condition cond, FrameEntry *fe);

    



    inline Jump testFunObj(Assembler::Condition cond, FrameEntry *fe);

    



    inline Jump testPrimitive(Assembler::Condition cond, FrameEntry *fe);

    




    inline void pinReg(RegisterID reg);

    


    inline void unpinReg(RegisterID reg);

    


    inline void dup();

    


    inline void dup2();

    


    inline void dupAt(int32 n);

    


    uint32 stackDepth() const { return sp - spBase; }
    uint32 frameDepth() const { return stackDepth() + script->nfixed; }
    inline FrameEntry *tosFe() const;

#ifdef DEBUG
    void assertValidRegisterState() const;
#endif

    Address addressOf(const FrameEntry *fe) const;

    inline StateRemat dataRematInfo(const FrameEntry *fe) const;

    





    inline void eviscerate(FrameEntry *fe);

    



    void shimmy(uint32 n);

    




    void shift(int32 n);

    inline void addEscaping(uint32 local);

    
    inline void forgetReg(RegisterID reg);
  private:
    inline RegisterID allocReg(FrameEntry *fe, RematInfo::RematType type, bool weak);
    RegisterID evictSomeReg(uint32 mask);
    void evictReg(RegisterID reg);
    inline FrameEntry *rawPush();
    inline FrameEntry *addToTracker(uint32 index);
    inline void syncType(const FrameEntry *fe, Address to, Assembler &masm) const;
    inline void syncData(const FrameEntry *fe, Address to, Assembler &masm) const;
    inline FrameEntry *getLocal(uint32 slot);
    inline void forgetAllRegs(FrameEntry *fe);
    inline void swapInTracker(FrameEntry *lhs, FrameEntry *rhs);
    inline uint32 localIndex(uint32 n);
    void pushCopyOf(uint32 index);
    void syncFancy(Assembler &masm, Registers avail, uint32 resumeAt) const;

    





    void uncopy(FrameEntry *original);

    FrameEntry *entryFor(uint32 index) const {
        JS_ASSERT(base[index]);
        return &entries[index];
    }

    void moveOwnership(RegisterID reg, FrameEntry *newFe) {
        regstate[reg].fe = newFe;
    }

    RegisterID evictSomeReg() {
        return evictSomeReg(Registers::AvailRegs);
    }

    uint32 indexOf(int32 depth) {
        return uint32((sp + depth) - base);
    }

    uint32 indexOfFe(FrameEntry *fe) {
        return uint32(fe - entries);
    }

  private:
    JSContext *cx;
    JSScript *script;
    uint32 nargs;
    Assembler &masm;

    
    Registers freeRegs;

    
    FrameEntry *entries;

    
    FrameEntry **base;

    
    FrameEntry **args;

    
    FrameEntry **locals;

    
    FrameEntry **spBase;

    
    FrameEntry **sp;

    
    Tracker tracker;

    



    RegisterState regstate[Assembler::TotalRegisters];

    mutable ImmutableSync reifier;

    uint32 *escaping;
    bool eval;
};

} 
} 

#endif 

