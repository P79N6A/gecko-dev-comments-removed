








































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

    struct LoopInfo {
        uint32 cfgEntry;

        LoopInfo(uint32 cfgEntry)
          : cfgEntry(cfgEntry)
        { }
    };

    
    
    
    
    struct CFGState {
        enum State {
            IF_TRUE,            
            IF_TRUE_EMPTY_ELSE, 
            IF_ELSE_TRUE,       
            IF_ELSE_FALSE,      
            DO_WHILE_LOOP,      
            WHILE_LOOP_COND,    
            WHILE_LOOP_BODY     
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

                
                jsbytecode *bodyStart;
                jsbytecode *bodyEnd;

                
                jsbytecode *exitpc;

                
                MBasicBlock *repeat;

                
                MBasicBlock *exit;

                
                MBasicBlock *successor;
            } loop;
        };

        inline bool isLoop() const {
            switch (state) {
              case DO_WHILE_LOOP:
              case WHILE_LOOP_COND:
              case WHILE_LOOP_BODY:
                return true;
              default:
                return false;
            }
        }

        static CFGState If(jsbytecode *join, MBasicBlock *ifFalse);
        static CFGState IfElse(jsbytecode *trueEnd, jsbytecode *falseEnd, MBasicBlock *ifFalse);
    };

  public:
    BytecodeAnalyzer(JSContext *cx, JSScript *script, JSFunction *fun, TempAllocator &temp,
                     MIRGraph &graph);

  public:
    bool analyze();

  private:
    bool traverseBytecode();
    ControlStatus snoopControlFlow(JSOp op);
    bool inspectOpcode(JSOp op);
    uint32 readIndex(jsbytecode *pc);

    void popCfgStack();
    CFGState &findInnermostLoop();
    ControlStatus processControlEnd();
    ControlStatus processCfgStack();
    ControlStatus processCfgEntry(CFGState &state);
    ControlStatus processIfEnd(CFGState &state);
    ControlStatus processIfElseTrueEnd(CFGState &state);
    ControlStatus processIfElseFalseEnd(CFGState &state);
    ControlStatus processDoWhileEnd(CFGState &state);
    ControlStatus processWhileCondEnd(CFGState &state);
    ControlStatus processWhileBodyEnd(CFGState &state);
    ControlStatus processReturn(JSOp op);
    ControlStatus simpleContinue(JSOp op, jssrcnote *sn);
    ControlStatus simpleBreak(JSOp op, jssrcnote *sn);
    bool pushLoop(CFGState::State state, jsbytecode *stopAt, MBasicBlock *entry,
                  jsbytecode *bodyStart, jsbytecode *bodyEnd, jsbytecode *exitpc);

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
    bool forInLoop(JSOp op, jssrcnote *sn) {
        return false;
    }
    bool whileLoop(JSOp op, jssrcnote *sn);
    bool doWhileLoop(JSOp op, jssrcnote *sn);

    bool pushConstant(const Value &v);
    bool jsop_binary(JSOp op);

  private:
    JSAtom **atoms;
    MBasicBlock *current;
    Vector<CFGState, 8, TempAllocPolicy> cfgStack_;
    Vector<LoopInfo, 4, TempAllocPolicy> loops_;
};

} 
} 

#endif 

