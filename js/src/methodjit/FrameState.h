






































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

    


    void pushSynced();

    


    void pushSyncedType(uint32 tag);

    


    void push(const Value &v);

    


    void push(Address address);

    


    void pushTypedPayload(uint32 tag, RegisterID payload);

    


    void pop();

    



    void popn(uint32 n);

    


    RegisterID tempRegForType(FrameEntry *fe);

    



    void freeReg(RegisterID reg);

    




    RegisterID allocReg();

    


    FrameEntry *peek(int32 depth);

    



    void storeTo(FrameEntry *fe, Address address, bool popHint);

    



    void syncAndKill(uint32 mask); 

    




    void forgetEverything(uint32 newStackDepth);

    



    void forgetEverything();

    


    uint32 stackDepth() const { return sp - spBase; }
    uint32 tos() const { return sp - base; }

#ifdef DEBUG
    void assertValidRegisterState() const;
#endif

  private:
    RegisterID alloc();
    RegisterID alloc(FrameEntry *fe, RematInfo::RematType type, bool weak);
    void forgetReg(RegisterID reg);
    void evictSomething();
    FrameEntry *rawPush();
    FrameEntry *addToTracker(uint32 index);
    void syncType(const FrameEntry *fe, Assembler &masm) const;
    void syncData(const FrameEntry *fe, Assembler &masm) const;

    Address addressOf(const FrameEntry *fe) const {
        uint32 index = (fe - entries);
        JS_ASSERT(index >= nargs);
        return Address(Assembler::FpReg, sizeof(JSStackFrame) + sizeof(Value) * index);
    }

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

