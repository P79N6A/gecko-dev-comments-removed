








































#ifndef jsion_bytecode_analyzer_h__
#define jsion_bytecode_analyzer_h__




#include "MIR.h"
#include "MIRGraph.h"

namespace js {
namespace ion {

class IonBuilder : public MIRGenerator
{
    enum ControlStatus {
        ControlStatus_Error,
        ControlStatus_Ended,        
        ControlStatus_Joined,       
        ControlStatus_Jumped,       
        ControlStatus_None          
    };

    struct DeferredEdge : public TempObject
    {
        MBasicBlock *block;
        DeferredEdge *next;

        DeferredEdge(MBasicBlock *block, DeferredEdge *next)
          : block(block), next(next)
        { }
    };

    struct LoopInfo {
        
        uint32 cfgEntry;

        
        jsbytecode *continuepc;

        LoopInfo(uint32 cfgEntry, jsbytecode *continuepc)
          : cfgEntry(cfgEntry),
            continuepc(continuepc)
        { }
    };

    
    
    
    
    struct CFGState {
        enum State {
            IF_TRUE,            
            IF_TRUE_EMPTY_ELSE, 
            IF_ELSE_TRUE,       
            IF_ELSE_FALSE,      
            DO_WHILE_LOOP_BODY, 
            DO_WHILE_LOOP_COND, 
            WHILE_LOOP_COND,    
            WHILE_LOOP_BODY,    
            FOR_LOOP_COND,      
            FOR_LOOP_BODY,      
            FOR_LOOP_UPDATE     
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

                
                MBasicBlock *successor;

                
                DeferredEdge *breaks;
                DeferredEdge *continues;

                
                jsbytecode *condpc;
                jsbytecode *updatepc;
                jsbytecode *updateEnd;
            } loop;
        };

        inline bool isLoop() const {
            switch (state) {
              case DO_WHILE_LOOP_COND:
              case DO_WHILE_LOOP_BODY:
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
    IonBuilder(JSContext *cx, JSScript *script, JSFunction *fun, TempAllocator &temp,
               MIRGraph &graph, TypeOracle *oracle);

    static void SetupOpcodeFlags();

  public:
    bool build();

  private:
    bool traverseBytecode();
    ControlStatus snoopControlFlow(JSOp op);
    bool inspectOpcode(JSOp op);
    uint32 readIndex(jsbytecode *pc);

    void popCfgStack();
    bool processDeferredContinues(CFGState &state);
    ControlStatus processControlEnd();
    ControlStatus processCfgStack();
    ControlStatus processCfgEntry(CFGState &state);
    ControlStatus processIfEnd(CFGState &state);
    ControlStatus processIfElseTrueEnd(CFGState &state);
    ControlStatus processIfElseFalseEnd(CFGState &state);
    ControlStatus processDoWhileBodyEnd(CFGState &state);
    ControlStatus processDoWhileCondEnd(CFGState &state);
    ControlStatus processWhileCondEnd(CFGState &state);
    ControlStatus processWhileBodyEnd(CFGState &state);
    ControlStatus processForCondEnd(CFGState &state);
    ControlStatus processForBodyEnd(CFGState &state);
    ControlStatus processForUpdateEnd(CFGState &state);
    ControlStatus processReturn(JSOp op);
    ControlStatus processContinue(JSOp op, jssrcnote *sn);
    ControlStatus processBreak(JSOp op, jssrcnote *sn);
    ControlStatus maybeLoop(JSOp op, jssrcnote *sn);
    bool pushLoop(CFGState::State state, jsbytecode *stopAt, MBasicBlock *entry,
                  jsbytecode *bodyStart, jsbytecode *bodyEnd, jsbytecode *exitpc,
                  jsbytecode *continuepc = NULL);

    MBasicBlock *newBlock(MBasicBlock *predecessor, jsbytecode *pc);
    MBasicBlock *newLoopHeader(MBasicBlock *predecessor, jsbytecode *pc);
    MBasicBlock *newBlock(jsbytecode *pc) {
        return newBlock(NULL, pc);
    }
    bool finalizeLoop(CFGState &state, MDefinition *last);
    void assertValidTraceOp(JSOp op);
    bool forInLoop(JSOp op, jssrcnote *sn) {
        return false;
    }
    ControlStatus forLoop(JSOp op, jssrcnote *sn);
    ControlStatus whileLoop(JSOp op, jssrcnote *sn);
    ControlStatus doWhileLoop(JSOp op, jssrcnote *sn);

    
    
    bool snapshotAfter();
    bool snapshotBefore();
    bool snapshot(jsbytecode *pc);

    bool pushConstant(const Value &v);
    bool jsop_binary(JSOp op);
    bool jsop_ifeq(JSOp op);

  private:
    JSContext *cx;
    JSAtom **atoms;
    MBasicBlock *current;
    Vector<CFGState, 8, IonAllocPolicy> cfgStack_;
    Vector<LoopInfo, 4, IonAllocPolicy> loops_;
    TypeOracle *oracle;
    bool sideEffectsOccurred_;
};

} 
} 

#endif 

