








































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

    struct ControlFlowInfo {
        
        uint32 cfgEntry;

        
        jsbytecode *continuepc;

        ControlFlowInfo(uint32 cfgEntry, jsbytecode *continuepc)
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
            FOR_LOOP_UPDATE,    
            TABLE_SWITCH,       
            AND_OR              
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
            struct {
                
                jsbytecode *exitpc;

                
                DeferredEdge *breaks;

                
                MTableSwitch *ins;

                
                uint32 currentSuccessor;

            } tableswitch;
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
        static CFGState AndOr(jsbytecode *join, MBasicBlock *joinStart);
    };

    static int CmpSuccessors(const void *a, const void *b);

    struct InliningData
    {
        bool shouldInline;
        JSFunction *callee;

        InliningData()
          : shouldInline(false), callee(NULL)
        { }
    };

  public:
    IonBuilder(JSContext *cx, TempAllocator &temp, MIRGraph &graph, TypeOracle *oracle,
               CompileInfo &info, size_t inliningDepth = 0);

    bool build();
    bool buildInline(MResumePoint *callerResumePoint, MDefinition *thisDefn,
                     MDefinitionVector &args);

  private:
    bool traverseBytecode();
    ControlStatus snoopControlFlow(JSOp op);
    bool inspectOpcode(JSOp op);
    uint32 readIndex(jsbytecode *pc);
    JSAtom *readAtom(jsbytecode *pc);

    static bool inliningEnabled() {
        return js_IonOptions.inlining;
    }

    bool shouldInlineCurrentCall(uint32 argc, InliningData *data);
    bool getInliningTarget(uint32 argc, jsbytecode *pc, JSFunction **out);

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
    ControlStatus processNextTableSwitchCase(CFGState &state);
    ControlStatus processTableSwitchEnd(CFGState &state);
    ControlStatus processAndOrEnd(CFGState &state);
    ControlStatus processSwitchBreak(JSOp op, jssrcnote *sn);
    ControlStatus processReturn(JSOp op);
    ControlStatus processContinue(JSOp op, jssrcnote *sn);
    ControlStatus processBreak(JSOp op, jssrcnote *sn);
    ControlStatus maybeLoop(JSOp op, jssrcnote *sn);
    bool pushLoop(CFGState::State state, jsbytecode *stopAt, MBasicBlock *entry,
                  jsbytecode *bodyStart, jsbytecode *bodyEnd, jsbytecode *exitpc,
                  jsbytecode *continuepc = NULL);

    MBasicBlock *addBlock(MBasicBlock *block);
    MBasicBlock *newBlock(MBasicBlock *predecessor, jsbytecode *pc);
    MBasicBlock *newOsrPreheader(MBasicBlock *header, jsbytecode *pc);
    MBasicBlock *newPendingLoopHeader(MBasicBlock *predecessor, jsbytecode *pc);
    MBasicBlock *newBlock(jsbytecode *pc) {
        return newBlock(NULL, pc);
    }

    
    
    MBasicBlock *createBreakCatchBlock(DeferredEdge *edge, jsbytecode *pc);

    
    
    ControlStatus processBrokenLoop(CFGState &state);

    
    
    ControlStatus finishLoop(CFGState &state, MBasicBlock *successor);

    void assertValidLoopHeadOp(jsbytecode *pc);

    bool forInLoop(JSOp op, jssrcnote *sn) {
        return false;
    }
    ControlStatus forLoop(JSOp op, jssrcnote *sn);
    ControlStatus whileLoop(JSOp op, jssrcnote *sn);
    ControlStatus doWhileLoop(JSOp op, jssrcnote *sn);
    ControlStatus tableSwitch(JSOp op, jssrcnote *sn);

    
    
    bool resumeAt(MInstruction *ins, jsbytecode *pc);
    bool resumeAfter(MInstruction *ins);

    void insertRecompileCheck();

    void initParameters();
    void rewriteParameters();
    bool pushConstant(const Value &v);
    bool pushTypeBarrier(MInstruction *ins, types::TypeSet *actual, types::TypeSet *observed);
    bool jsop_bitnot();
    bool jsop_bitop(JSOp op);
    bool jsop_binary(JSOp op);
    bool jsop_binary(JSOp op, MDefinition *left, MDefinition *right);
    bool jsop_pos();
    bool jsop_neg();
    bool jsop_notearg();
    bool jsop_call(uint32 argc);
    bool jsop_ifeq(JSOp op);
    bool jsop_andor(JSOp op);
    bool jsop_dup2();
    bool jsop_loophead(jsbytecode *pc);
    bool jsop_incslot(JSOp op, uint32 slot);
    bool jsop_localinc(JSOp op);
    bool jsop_arginc(JSOp op);
    bool jsop_compare(JSOp op);
    bool jsop_getgname(JSAtom *atom);
    bool jsop_callgname(JSAtom *atom);
    bool jsop_setgname(JSAtom *atom);
    bool jsop_getelem();
    bool jsop_getelem_dense();
    bool jsop_setelem();
    bool jsop_setelem_dense();
    bool jsop_length();
    bool jsop_length_fastPath();
    bool jsop_getprop(JSAtom *atom);
    bool jsop_newarray(uint32 count);
    bool jsop_this();

    

    enum InliningStatus
    {
        InliningStatus_Error,
        InliningStatus_NotInlined,
        InliningStatus_Inlined
    };

    bool jsop_call_inline(uint32 argc, IonBuilder &inlineBuilder, InliningData *data);
    InliningStatus maybeInline(uint32 argc);
    bool makeInliningDecision(uint32 argc, InliningData *data);

  public:
    
    JSScript * const script;

  private:
    jsbytecode *pc;
    MBasicBlock *current;
    MResumePoint *lastResumePoint_;

    
    MResumePoint *callerResumePoint_;
    jsbytecode *callerPC() {
        return callerResumePoint_ ? callerResumePoint_->pc() : NULL;
    }

    Vector<CFGState, 8, IonAllocPolicy> cfgStack_;
    Vector<ControlFlowInfo, 4, IonAllocPolicy> loops_;
    Vector<ControlFlowInfo, 0, IonAllocPolicy> switches_;
    TypeOracle *oracle;
    size_t inliningDepth;
};

} 
} 

#endif 

