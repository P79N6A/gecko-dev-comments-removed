








































#ifndef jsion_bytecode_analyzer_h__
#define jsion_bytecode_analyzer_h__

#include "MIR.h"

namespace js {
namespace ion {

class BytecodeAnalyzer : public MIRGenerator
{
    enum ControlStatus {
        ControlStatus_Error,
        ControlStatus_Ended,        
        ControlStatus_Joined,       
        ControlStatus_Jumped,       
        ControlStatus_None          
    };

    
    
    
    
    struct CFGState {
        enum State {
            IF_TRUE,            
            IF_TRUE_EMPTY_ELSE, 
            IF_ELSE_TRUE,       
            IF_ELSE_FALSE,      
            DO_WHILE_LOOP
        };

        State state;            
        jsbytecode *stopAt;     
        
        
        union {
            struct {
                MBasicBlock *ifFalse;
                jsbytecode *falseEnd;
                MBasicBlock *ifTrue;    
            } branch;
            struct {
                MBasicBlock *entry;     
            } loop;
        };

        static CFGState If(jsbytecode *join, MBasicBlock *ifFalse);
        static CFGState IfElse(jsbytecode *trueEnd, jsbytecode *falseEnd, MBasicBlock *ifFalse);
        static CFGState DoWhile(jsbytecode *ifne, MBasicBlock *entry);
    };

  public:
    BytecodeAnalyzer(JSContext *cx, JSScript *script, JSFunction *fun, TempAllocator &temp);

  public:
    bool analyze();

  private:
    bool traverseBytecode();
    ControlStatus snoopControlFlow(JSOp op);
    bool inspectOpcode(JSOp op);
    uint32 readIndex(jsbytecode *pc);

    ControlStatus processControlEnd();
    ControlStatus processCfgStack();
    ControlStatus processCfgEntry(CFGState &state);
    ControlStatus processIfEnd(CFGState &state);
    ControlStatus processIfElseTrueEnd(CFGState &state);
    ControlStatus processIfElseFalseEnd(CFGState &state);
    ControlStatus processDoWhileEnd(CFGState &state);
    ControlStatus processReturn(JSOp op);

    bool addBlock(MBasicBlock *block);
    MBasicBlock *newBlock(MBasicBlock *predecessor, jsbytecode *pc);
    MBasicBlock *newLoopHeader(MBasicBlock *predecessor, jsbytecode *pc);
    MBasicBlock *newBlock(jsbytecode *pc) {
        return newBlock(NULL, pc);
    }

    void assertValidTraceOp(JSOp op);
    bool maybeLoop(JSOp op, jssrcnote *sn);
    bool jsop_ifeq(JSOp op);

    bool forLoop(JSOp op, jssrcnote *sn) {
        return false;
    }
    bool whileLoop(JSOp op, jssrcnote *sn) {
        return false;
    }
    bool forInLoop(JSOp op, jssrcnote *sn) {
        return false;
    }
    bool doWhileLoop(JSOp op, jssrcnote *sn);

    bool pushConstant(const Value &v);
    bool jsop_binary(JSOp op);

  private:
    JSAtom **atoms;
    MBasicBlock *current;
    Vector<CFGState, 8, TempAllocPolicy> cfgStack_;
};

class MBasicBlock : public TempObject
{
    static const uint32 NotACopy = uint32(-1);

    struct StackSlot {
        MInstruction *ins;
        uint32 copyOf;
        union {
            uint32 firstCopy; 
            uint32 nextCopy;  
        };

        void set(MInstruction *ins) {
            this->ins = ins;
            copyOf = NotACopy;
            firstCopy = NotACopy;
        }
        bool isCopy() const {
            return copyOf != NotACopy;
        }
        bool isCopied() const {
            if (isCopy())
                return false;
            return firstCopy != NotACopy;
        }
    };

  private:
    MBasicBlock(MIRGenerator *gen, jsbytecode *pc);
    bool init();
    bool initLoopHeader();
    bool inherit(MBasicBlock *pred);
    void assertUsesAreNotWithin(MOperand *use);

    
    void setSlot(uint32 slot, MInstruction *ins);

    
    void pushCopy(uint32 slot);

    
    void pushVariable(uint32 slot);

    
    
    bool setVariable(uint32 slot);

    MInstruction *getSlot(uint32 index);

  public:
    
    
    static MBasicBlock *New(MIRGenerator *gen, MBasicBlock *pred, jsbytecode *entryPc);
    static MBasicBlock *NewLoopHeader(MIRGenerator *gen, MBasicBlock *pred, jsbytecode *entryPc);

    void setId(uint32 id) {
        id_ = id;
    }

    
    MInstruction *peek(int32 depth);

    
    
    void initSlot(uint32 index, MInstruction *ins);

    
    
    bool setLocal(uint32 local);
    bool setArg(uint32 arg);

    
    void push(MInstruction *ins);
    void pushArg(uint32 arg);
    void pushLocal(uint32 local);

    
    MInstruction *pop();

    
    
    bool add(MInstruction *ins);

    
    
    bool end(MControlInstruction *ins);

    
    bool addPhi(MPhi *phi);

    
    
    
    bool addPredecessor(MBasicBlock *pred);

    
    
    
    bool addBackedge(MBasicBlock *block, MBasicBlock *successor);

    jsbytecode *pc() const {
        return pc_;
    }
    uint32 id() const {
        return id_;
    }
    uint32 numPredecessors() const {
        return predecessors_.length();
    }
    MBasicBlock *getPredecessor(uint32 i) const {
        return predecessors_[i];
    }
    MControlInstruction *lastIns() const {
        return lastIns_;
    }

  private:
    MIRGenerator *gen;
    Vector<MInstruction *, 4, TempAllocPolicy> instructions_;
    Vector<MBasicBlock *, 1, TempAllocPolicy> predecessors_;
    Vector<MPhi *, 2, TempAllocPolicy> phis_;
    StackSlot *slots_;
    uint32 stackPosition_;
    MControlInstruction *lastIns_;
    jsbytecode *pc_;
    uint32 id_;

    
    
    StackSlot *header_;
};


} 
} 

#endif 

