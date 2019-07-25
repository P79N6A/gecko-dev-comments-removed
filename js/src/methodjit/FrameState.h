






































#if !defined jsjaeger_framestate_h__ && defined JS_METHODJIT
#define jsjaeger_framestate_h__

#include "jsapi.h"
#include "methodjit/MachineRegs.h"
#include "methodjit/FrameEntry.h"
#include "CodeGenIncludes.h"

namespace js {
namespace mjit {






































class FrameState
{
    typedef JSC::MacroAssembler::RegisterID RegisterID;
    typedef JSC::MacroAssembler::Address Address;
    typedef JSC::MacroAssembler::Jump Jump;
    typedef JSC::MacroAssembler::Imm32 Imm32;

    struct Tracker {
        Tracker()
          : entries(NULL), nentries(0)
        { }

        void add(uint32 index) {
            entries[nentries++] = index;
        }

        void reset() {
            nentries = 0;
        }

        uint32 operator [](uint32 n) const {
            return entries[n];
        }

        uint32 *entries;
        uint32 nentries;
    };

    struct RegisterState {
        RegisterState()
        { }

        RegisterState(FrameEntry *fe, RematInfo::RematType type, bool weak)
          : fe(fe), type(type), weak(weak)
        { }

        
        FrameEntry *fe;
        
        
        RematInfo::RematType type;

        
        bool weak;
    };

  public:
    FrameState(JSContext *cx, JSScript *script, Assembler &masm);
    ~FrameState();
    bool init(uint32 nargs);

    


    inline void pushSynced();

    


    inline void pushSyncedType(JSValueMask32 tag);

    


    inline void push(const Value &v);

    


    inline void push(Address address);

    


    inline void pushTypedPayload(JSValueMask32 tag, RegisterID payload);

    




    inline void pushUntypedPayload(JSValueMask32 tag, RegisterID payload);

    


    inline void pop();

    



    inline void popn(uint32 n);

    




    inline RegisterID tempRegForType(FrameEntry *fe);

    




    inline RegisterID tempRegForData(FrameEntry *fe);

    








    RegisterID ownRegForData(FrameEntry *fe);

    



    RegisterID copyData(FrameEntry *fe);

    




    inline bool shouldAvoidTypeRemat(FrameEntry *fe);

    




    inline bool shouldAvoidDataRemat(FrameEntry *fe);

    



    inline void freeReg(RegisterID reg);

    




    inline RegisterID allocReg();

    


    inline FrameEntry *peek(int32 depth);

    



    void storeTo(FrameEntry *fe, Address address, bool popHint);

    


    void merge(Assembler &masm, uint32 ivD) const;

    


    void sync(Assembler &masm) const;

    



    void syncAndKill(uint32 mask); 

    




    inline void forgetEverything(uint32 newStackDepth);

    



    void forgetEverything();

    


    inline void learnType(FrameEntry *fe, JSValueMask32 tag);

    



    inline Jump testInt32(Assembler::Condition cond, FrameEntry *fe);

    


    uint32 stackDepth() const { return sp - spBase; }
    uint32 tos() const { return sp - base; }

#ifdef DEBUG
    void assertValidRegisterState() const;
#endif

    Address addressOf(const FrameEntry *fe) const;

  private:
    inline RegisterID alloc();
    inline RegisterID alloc(FrameEntry *fe, RematInfo::RematType type, bool weak);
    inline void forgetReg(RegisterID reg);
    RegisterID evictSomething();
    inline FrameEntry *rawPush();
    inline FrameEntry *addToTracker(uint32 index);
    inline void syncType(const FrameEntry *fe, Assembler &masm) const;
    inline void syncData(const FrameEntry *fe, Assembler &masm) const;

    uint32 indexOf(int32 depth) {
        return uint32((sp + depth) - base);
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
};

} 
} 

#endif 

