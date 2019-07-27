





#ifndef frontend_BytecodeEmitter_h
#define frontend_BytecodeEmitter_h





#include "jscntxt.h"
#include "jsopcode.h"
#include "jsscript.h"

#include "frontend/ParseMaps.h"
#include "frontend/Parser.h"
#include "frontend/SharedContext.h"
#include "frontend/SourceNotes.h"

namespace js {

class StaticEvalObject;

namespace frontend {

class FullParseHandler;
class ObjectBox;
class ParseNode;
template <typename ParseHandler> class Parser;
class SharedContext;
class TokenStream;

class CGConstList {
    Vector<Value> list;
  public:
    explicit CGConstList(ExclusiveContext* cx) : list(cx) {}
    bool append(Value v) { MOZ_ASSERT_IF(v.isString(), v.toString()->isAtom()); return list.append(v); }
    size_t length() const { return list.length(); }
    void finish(ConstArray* array);
};

struct CGObjectList {
    uint32_t            length;     
    ObjectBox*          lastbox;   

    CGObjectList() : length(0), lastbox(nullptr) {}

    unsigned add(ObjectBox* objbox);
    unsigned indexOf(JSObject* obj);
    void finish(ObjectArray* array);
    ObjectBox* find(uint32_t index);
};

struct CGTryNoteList {
    Vector<JSTryNote> list;
    explicit CGTryNoteList(ExclusiveContext* cx) : list(cx) {}

    bool append(JSTryNoteKind kind, uint32_t stackDepth, size_t start, size_t end);
    size_t length() const { return list.length(); }
    void finish(TryNoteArray* array);
};

struct CGBlockScopeList {
    Vector<BlockScopeNote> list;
    explicit CGBlockScopeList(ExclusiveContext* cx) : list(cx) {}

    bool append(uint32_t scopeObject, uint32_t offset, uint32_t parent);
    uint32_t findEnclosingScope(uint32_t index);
    void recordEnd(uint32_t index, uint32_t offset);
    size_t length() const { return list.length(); }
    void finish(BlockScopeArray* array);
};

struct CGYieldOffsetList {
    Vector<uint32_t> list;
    explicit CGYieldOffsetList(ExclusiveContext* cx) : list(cx) {}

    bool append(uint32_t offset) { return list.append(offset); }
    size_t length() const { return list.length(); }
    void finish(YieldOffsetArray& array, uint32_t prologueLength);
};

struct LoopStmtInfo;
struct StmtInfoBCE;



typedef Vector<jsbytecode, 0> BytecodeVector;
typedef Vector<jssrcnote, 0> SrcNotesVector;







enum VarEmitOption {
    DefineVars        = 0,
    PushInitialValues = 1,
    InitializeVars    = 2
};

struct BytecodeEmitter
{
    typedef StmtInfoBCE StmtInfo;

    SharedContext* const sc;      

    ExclusiveContext* const cx;

    BytecodeEmitter* const parent;  

    Rooted<JSScript*> script;       

    Rooted<LazyScript*> lazyScript; 


    struct EmitSection {
        BytecodeVector code;        
        SrcNotesVector notes;       
        ptrdiff_t   lastNoteOffset; 
        uint32_t    currentLine;    
        uint32_t    lastColumn;     


        EmitSection(ExclusiveContext* cx, uint32_t lineNum)
          : code(cx), notes(cx), lastNoteOffset(0), currentLine(lineNum), lastColumn(0)
        {}
    };
    EmitSection prologue, main, *current;

    
    Parser<FullParseHandler>* const parser;

    HandleScript    evalCaller;     
    Handle<StaticEvalObject*> evalStaticScope;
                                   

    StmtInfoBCE*    topStmt;       
    StmtInfoBCE*    topScopeStmt;  
    Rooted<NestedScopeObject*> staticScope;
                                    

    OwnedAtomIndexMapPtr atomIndices; 
    unsigned        firstLine;      

    




    Vector<uint32_t, 16> localsToFrameSlots_;

    int32_t         stackDepth;     
    uint32_t        maxStackDepth;  

    uint32_t        arrayCompDepth; 

    unsigned        emitLevel;      

    CGConstList     constList;      

    CGObjectList    objectList;     
    CGObjectList    regexpList;     

    CGTryNoteList   tryNoteList;    
    CGBlockScopeList blockScopeList;

    



    CGYieldOffsetList yieldOffsetList;

    uint16_t        typesetCount;   

    bool            hasSingletons:1;    

    bool            hasTryFinally:1;    

    bool            emittingForInit:1;  

    bool            emittingRunOnceLambda:1; 


    bool isRunOnceLambda();

    bool            insideEval:1;       


    const bool      insideNonGlobalEval:1;  


    enum EmitterMode {
        Normal,

        




        SelfHosting,

        



        LazyFunction
    };

    const EmitterMode emitterMode;

    





    BytecodeEmitter(BytecodeEmitter* parent, Parser<FullParseHandler>* parser, SharedContext* sc,
                    HandleScript script, Handle<LazyScript*> lazyScript,
                    bool insideEval, HandleScript evalCaller,
                    Handle<StaticEvalObject*> evalStaticScope, bool insideNonGlobalEval,
                    uint32_t lineNum, EmitterMode emitterMode = Normal);
    bool init();
    bool updateLocalsToFrameSlots();

    bool isAliasedName(ParseNode* pn);

    MOZ_ALWAYS_INLINE
    bool makeAtomIndex(JSAtom* atom, jsatomid* indexp) {
        AtomIndexAddPtr p = atomIndices->lookupForAdd(atom);
        if (p) {
            *indexp = p.value();
            return true;
        }

        jsatomid index = atomIndices->count();
        if (!atomIndices->add(p, atom, index))
            return false;

        *indexp = index;
        return true;
    }

    bool isInLoop();
    bool checkSingletonContext();

    
    
    bool checkRunOnceContext();

    bool needsImplicitThis();

    void tellDebuggerAboutCompiledScript(ExclusiveContext* cx);

    inline TokenStream* tokenStream();

    BytecodeVector& code() const { return current->code; }
    jsbytecode* code(ptrdiff_t offset) const { return current->code.begin() + offset; }
    ptrdiff_t offset() const { return current->code.end() - current->code.begin(); }
    ptrdiff_t prologueOffset() const { return prologue.code.end() - prologue.code.begin(); }
    void switchToMain() { current = &main; }
    void switchToPrologue() { current = &prologue; }

    SrcNotesVector& notes() const { return current->notes; }
    ptrdiff_t lastNoteOffset() const { return current->lastNoteOffset; }
    unsigned currentLine() const { return current->currentLine; }
    unsigned lastColumn() const { return current->lastColumn; }

    bool reportError(ParseNode* pn, unsigned errorNumber, ...);
    bool reportStrictWarning(ParseNode* pn, unsigned errorNumber, ...);
    bool reportStrictModeError(ParseNode* pn, unsigned errorNumber, ...);

    
    
    
    
    
    
    
    
    
    
    bool checkSideEffects(ParseNode* pn, bool* answer);

    bool inTryBlockWithFinally();

#ifdef DEBUG
    bool checkStrictOrSloppy(JSOp op);
#endif

    
    
    
    bool newSrcNote(SrcNoteType type, unsigned* indexp = nullptr);
    bool newSrcNote2(SrcNoteType type, ptrdiff_t offset, unsigned* indexp = nullptr);
    bool newSrcNote3(SrcNoteType type, ptrdiff_t offset1, ptrdiff_t offset2,
                     unsigned* indexp = nullptr);

    void copySrcNotes(jssrcnote* destination, uint32_t nsrcnotes);
    bool setSrcNoteOffset(unsigned index, unsigned which, ptrdiff_t offset);

    
    bool addToSrcNoteDelta(jssrcnote* sn, ptrdiff_t delta);

    
    
    bool finishTakingSrcNotes(uint32_t* out);

    void setJumpOffsetAt(ptrdiff_t off);

    
    bool emitTree(ParseNode* pn);

    
    bool emitFunctionScript(ParseNode* body);

    
    
    void checkTypeSet(JSOp op);

    void updateDepth(ptrdiff_t target);
    bool updateLineNumberNotes(uint32_t offset);
    bool updateSourceCoordNotes(uint32_t offset);

    bool bindNameToSlot(ParseNode* pn);
    bool bindNameToSlotHelper(ParseNode* pn);

    void strictifySetNameNode(ParseNode* pn);
    JSOp strictifySetNameOp(JSOp op);

    bool tryConvertFreeName(ParseNode* pn);

    void popStatement();
    void pushStatement(StmtInfoBCE* stmt, StmtType type, ptrdiff_t top);
    void pushStatementInner(StmtInfoBCE* stmt, StmtType type, ptrdiff_t top);
    void pushLoopStatement(LoopStmtInfo* stmt, StmtType type, ptrdiff_t top);

    
    
    JSObject* enclosingStaticScope();

    
    
    unsigned dynamicNestedScopeDepth();

    bool enterNestedScope(StmtInfoBCE* stmt, ObjectBox* objbox, StmtType stmtType);
    bool leaveNestedScope(StmtInfoBCE* stmt);

    bool enterBlockScope(StmtInfoBCE* stmtInfo, ObjectBox* objbox, JSOp initialValueOp,
                         unsigned alreadyPushed = 0);

    bool computeAliasedSlots(Handle<StaticBlockObject*> blockObj);

    bool lookupAliasedName(HandleScript script, PropertyName* name, uint32_t* pslot,
                           ParseNode* pn = nullptr);
    bool lookupAliasedNameSlot(PropertyName* name, ScopeCoordinate* sc);

    
    
    bool assignHops(ParseNode* pn, unsigned src, ScopeCoordinate* dst);

    
    
    
    
    void computeLocalOffset(Handle<StaticBlockObject*> blockObj);

    bool flushPops(int* npops);

    bool emitCheck(ptrdiff_t delta, ptrdiff_t* offset);

    
    bool emit1(JSOp op);

    
    
    bool emit2(JSOp op, jsbytecode op1);

    
    bool emit3(JSOp op, jsbytecode op1, jsbytecode op2);

    
    
    
    
    
    
    
    
    
    
    bool emitDupAt(unsigned slot);

    
    
    bool emitUint16Operand(JSOp op, uint32_t i);

    
    bool emitN(JSOp op, size_t extra, ptrdiff_t* offset = nullptr);

    bool emitNumberOp(double dval);

    bool emitJump(JSOp op, ptrdiff_t off, ptrdiff_t* jumpOffset = nullptr);
    bool emitCall(JSOp op, uint16_t argc, ParseNode* pn = nullptr);

    bool emitLoopHead(ParseNode* nextpn);
    bool emitLoopEntry(ParseNode* nextpn);

    
    
    
    bool emitBackPatchOp(ptrdiff_t* lastp);
    void backPatch(ptrdiff_t last, jsbytecode* target, jsbytecode op);

    bool emitGoto(StmtInfoBCE* toStmt, ptrdiff_t* lastp, SrcNoteType noteType = SRC_NULL);

    bool emitIndex32(JSOp op, uint32_t index);
    bool emitIndexOp(JSOp op, uint32_t index);

    bool emitAtomOp(JSAtom* atom, JSOp op);
    bool emitAtomOp(ParseNode* pn, JSOp op);

    bool emitArray(ParseNode* pn, uint32_t count);
    bool emitArrayComp(ParseNode* pn);

    bool emitInternedObjectOp(uint32_t index, JSOp op);
    bool emitObjectOp(ObjectBox* objbox, JSOp op);
    bool emitObjectPairOp(ObjectBox* objbox1, ObjectBox* objbox2, JSOp op);
    bool emitRegExp(uint32_t index);

    MOZ_NEVER_INLINE bool emitFunction(ParseNode* pn, bool needsProto = false);
    MOZ_NEVER_INLINE bool emitObject(ParseNode* pn);

    bool emitPropertyList(ParseNode* pn, MutableHandlePlainObject objp, PropListType type);

    
    
    
    
    
    bool emitLocalOp(JSOp op, uint32_t slot);

    bool emitScopeCoordOp(JSOp op, ScopeCoordinate sc);
    bool emitAliasedVarOp(JSOp op, ParseNode* pn);
    bool emitAliasedVarOp(JSOp op, ScopeCoordinate sc, MaybeCheckLexical checkLexical);
    bool emitUnaliasedVarOp(JSOp op, uint32_t slot, MaybeCheckLexical checkLexical);

    bool emitVarOp(ParseNode* pn, JSOp op);
    bool emitVarIncDec(ParseNode* pn);

    bool emitNameOp(ParseNode* pn, bool callContext);
    bool emitNameIncDec(ParseNode* pn);

    bool maybeEmitVarDecl(JSOp prologueOp, ParseNode* pn, jsatomid* result);
    bool emitVariables(ParseNode* pn, VarEmitOption emitOption, bool isLetExpr = false);

    bool emitNewInit(JSProtoKey key);
    bool emitSingletonInitialiser(ParseNode* pn);

    bool emitPrepareIteratorResult();
    bool emitFinishIteratorResult(bool done);
    bool iteratorResultShape(unsigned* shape);

    bool emitYield(ParseNode* pn);
    bool emitYieldOp(JSOp op);
    bool emitYieldStar(ParseNode* iter, ParseNode* gen);

    bool emitPropLHS(ParseNode* pn, JSOp op);
    bool emitPropOp(ParseNode* pn, JSOp op);
    bool emitPropIncDec(ParseNode* pn);

    
    
    
    bool emitElemOperands(ParseNode* pn, JSOp op);

    bool emitElemOpBase(JSOp op);
    bool emitElemOp(ParseNode* pn, JSOp op);
    bool emitElemIncDec(ParseNode* pn);

    bool emitCatch(ParseNode* pn);
    bool emitIf(ParseNode* pn);
    bool emitWith(ParseNode* pn);

    MOZ_NEVER_INLINE bool emitLabeledStatement(const LabeledStatement* pn);
    MOZ_NEVER_INLINE bool emitLet(ParseNode* pnLet);
    MOZ_NEVER_INLINE bool emitLexicalScope(ParseNode* pn);
    MOZ_NEVER_INLINE bool emitSwitch(ParseNode* pn);
    MOZ_NEVER_INLINE bool emitTry(ParseNode* pn);

    
    
    
    
    
    
    
    
    
    
    bool emitDestructuringLHS(ParseNode* target, VarEmitOption emitOption);

    bool emitDestructuringOps(ParseNode* pattern, bool isLet = false);
    bool emitDestructuringOpsHelper(ParseNode* pattern, VarEmitOption emitOption);
    bool emitDestructuringOpsArrayHelper(ParseNode* pattern, VarEmitOption emitOption);
    bool emitDestructuringOpsObjectHelper(ParseNode* pattern, VarEmitOption emitOption);

    typedef bool
    (*DestructuringDeclEmitter)(BytecodeEmitter* bce, JSOp prologueOp, ParseNode* pn);

    template <DestructuringDeclEmitter EmitName>
    bool emitDestructuringDeclsWithEmitter(JSOp prologueOp, ParseNode* pattern);

    bool emitDestructuringDecls(JSOp prologueOp, ParseNode* pattern);

    
    
    bool emitInitializeDestructuringDecls(JSOp prologueOp, ParseNode* pattern);

    
    
    bool emitIterator();

    
    
    bool emitIteratorNext(ParseNode* pn);

    
    
    bool emitDefault(ParseNode* defaultExpr);

    bool emitCallSiteObject(ParseNode* pn);
    bool emitTemplateString(ParseNode* pn);
    bool emitAssignment(ParseNode* lhs, JSOp op, ParseNode* rhs);

    bool emitReturn(ParseNode* pn);
    bool emitStatement(ParseNode* pn);
    bool emitStatementList(ParseNode* pn, ptrdiff_t top);
    bool emitSyntheticStatements(ParseNode* pn, ptrdiff_t top);

    bool emitDelete(ParseNode* pn);
    bool emitLogical(ParseNode* pn);
    bool emitUnary(ParseNode* pn);

    MOZ_NEVER_INLINE bool emitIncOrDec(ParseNode* pn);

    bool emitConditionalExpression(ConditionalExpression& conditional);

    bool emitCallOrNew(ParseNode* pn);
    bool emitSelfHostedCallFunction(ParseNode* pn);
    bool emitSelfHostedResumeGenerator(ParseNode* pn);
    bool emitSelfHostedForceInterpreter(ParseNode* pn);

    bool emitDo(ParseNode* pn);
    bool emitFor(ParseNode* pn, ptrdiff_t top);
    bool emitForIn(ParseNode* pn, ptrdiff_t top);
    bool emitForInOrOfVariables(ParseNode* pn, bool* letDecl);
    bool emitNormalFor(ParseNode* pn, ptrdiff_t top);
    bool emitWhile(ParseNode* pn, ptrdiff_t top);

    bool emitBreak(PropertyName* label);
    bool emitContinue(PropertyName* label);

    bool emitDefaults(ParseNode* pn);
    bool emitLexicalInitialization(ParseNode* pn, JSOp globalDefOp);

    bool pushInitialConstants(JSOp op, unsigned n);
    bool initializeBlockScopedLocalsFromStack(Handle<StaticBlockObject*> blockObj);

    
    
    
    
    
    
    bool emitSpread();

    
    
    
    
    
    
    
    
    bool emitForOf(StmtType type, ParseNode* pn, ptrdiff_t top);

    bool emitClass(ParseNode* pn);
};

} 
} 

#endif 
