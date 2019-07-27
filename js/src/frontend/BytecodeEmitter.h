





#ifndef frontend_BytecodeEmitter_h
#define frontend_BytecodeEmitter_h





#include "jscntxt.h"
#include "jsopcode.h"
#include "jsscript.h"

#include "frontend/ParseMaps.h"
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
    explicit CGConstList(ExclusiveContext *cx) : list(cx) {}
    bool append(Value v) { MOZ_ASSERT_IF(v.isString(), v.toString()->isAtom()); return list.append(v); }
    size_t length() const { return list.length(); }
    void finish(ConstArray *array);
};

struct CGObjectList {
    uint32_t            length;     
    ObjectBox           *lastbox;   

    CGObjectList() : length(0), lastbox(nullptr) {}

    unsigned add(ObjectBox *objbox);
    unsigned indexOf(JSObject *obj);
    void finish(ObjectArray *array);
    ObjectBox* find(uint32_t index);
};

struct CGTryNoteList {
    Vector<JSTryNote> list;
    explicit CGTryNoteList(ExclusiveContext *cx) : list(cx) {}

    bool append(JSTryNoteKind kind, uint32_t stackDepth, size_t start, size_t end);
    size_t length() const { return list.length(); }
    void finish(TryNoteArray *array);
};

struct CGBlockScopeList {
    Vector<BlockScopeNote> list;
    explicit CGBlockScopeList(ExclusiveContext *cx) : list(cx) {}

    bool append(uint32_t scopeObject, uint32_t offset, uint32_t parent);
    uint32_t findEnclosingScope(uint32_t index);
    void recordEnd(uint32_t index, uint32_t offset);
    size_t length() const { return list.length(); }
    void finish(BlockScopeArray *array);
};

struct CGYieldOffsetList {
    Vector<uint32_t> list;
    explicit CGYieldOffsetList(ExclusiveContext *cx) : list(cx) {}

    bool append(uint32_t offset) { return list.append(offset); }
    size_t length() const { return list.length(); }
    void finish(YieldOffsetArray &array, uint32_t prologLength);
};

struct StmtInfoBCE;



typedef Vector<jsbytecode, 0> BytecodeVector;
typedef Vector<jssrcnote, 0> SrcNotesVector;

struct BytecodeEmitter
{
    typedef StmtInfoBCE StmtInfo;

    SharedContext   *const sc;      

    BytecodeEmitter *const parent;  

    Rooted<JSScript*> script;       

    Rooted<LazyScript *> lazyScript; 


    struct EmitSection {
        BytecodeVector code;        
        SrcNotesVector notes;       
        ptrdiff_t   lastNoteOffset; 
        uint32_t    currentLine;    
        uint32_t    lastColumn;     


        EmitSection(ExclusiveContext *cx, uint32_t lineNum)
          : code(cx), notes(cx), lastNoteOffset(0), currentLine(lineNum), lastColumn(0)
        {}
    };
    EmitSection prolog, main, *current;

    
    Parser<FullParseHandler> *const parser;

    HandleScript    evalCaller;     
    Handle<StaticEvalObject *> evalStaticScope;
                                   

    StmtInfoBCE     *topStmt;       
    StmtInfoBCE     *topScopeStmt;  
    Rooted<NestedScopeObject *> staticScope;
                                    

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


    const bool      hasGlobalScope:1;   


    enum EmitterMode {
        Normal,

        




        SelfHosting,

        



        LazyFunction
    };

    const EmitterMode emitterMode;

    





    BytecodeEmitter(BytecodeEmitter *parent, Parser<FullParseHandler> *parser, SharedContext *sc,
                    HandleScript script, Handle<LazyScript *> lazyScript,
                    bool insideEval, HandleScript evalCaller,
                    Handle<StaticEvalObject *> evalStaticScope, bool hasGlobalScope,
                    uint32_t lineNum, EmitterMode emitterMode = Normal);
    bool init();
    bool updateLocalsToFrameSlots();

    bool isAliasedName(ParseNode *pn);

    MOZ_ALWAYS_INLINE
    bool makeAtomIndex(JSAtom *atom, jsatomid *indexp) {
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

    bool needsImplicitThis();

    void tellDebuggerAboutCompiledScript(ExclusiveContext *cx);

    inline TokenStream *tokenStream();

    BytecodeVector &code() const { return current->code; }
    jsbytecode *code(ptrdiff_t offset) const { return current->code.begin() + offset; }
    ptrdiff_t offset() const { return current->code.end() - current->code.begin(); }
    ptrdiff_t prologOffset() const { return prolog.code.end() - prolog.code.begin(); }
    void switchToMain() { current = &main; }
    void switchToProlog() { current = &prolog; }

    SrcNotesVector &notes() const { return current->notes; }
    ptrdiff_t lastNoteOffset() const { return current->lastNoteOffset; }
    unsigned currentLine() const { return current->currentLine; }
    unsigned lastColumn() const { return current->lastColumn; }

    bool reportError(ParseNode *pn, unsigned errorNumber, ...);
    bool reportStrictWarning(ParseNode *pn, unsigned errorNumber, ...);
    bool reportStrictModeError(ParseNode *pn, unsigned errorNumber, ...);

    
    bool emit1(JSOp op);
};




bool
Emit2(ExclusiveContext *cx, BytecodeEmitter *bce, JSOp op, jsbytecode op1);




bool
Emit3(ExclusiveContext *cx, BytecodeEmitter *bce, JSOp op, jsbytecode op1, jsbytecode op2);




ptrdiff_t
EmitN(ExclusiveContext *cx, BytecodeEmitter *bce, JSOp op, size_t extra);




bool
EmitTree(ExclusiveContext *cx, BytecodeEmitter *bce, ParseNode *pn);




bool
EmitFunctionScript(ExclusiveContext *cx, BytecodeEmitter *bce, ParseNode *body);







int
NewSrcNote(ExclusiveContext *cx, BytecodeEmitter *bce, SrcNoteType type);

int
NewSrcNote2(ExclusiveContext *cx, BytecodeEmitter *bce, SrcNoteType type, ptrdiff_t offset);

int
NewSrcNote3(ExclusiveContext *cx, BytecodeEmitter *bce, SrcNoteType type, ptrdiff_t offset1,
               ptrdiff_t offset2);


bool
AddToSrcNoteDelta(ExclusiveContext *cx, BytecodeEmitter *bce, jssrcnote *sn, ptrdiff_t delta);

bool
FinishTakingSrcNotes(ExclusiveContext *cx, BytecodeEmitter *bce, uint32_t *out);

void
CopySrcNotes(BytecodeEmitter *bce, jssrcnote *destination, uint32_t nsrcnotes);

} 
} 

#endif 
