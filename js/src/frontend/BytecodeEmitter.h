





#ifndef frontend_BytecodeEmitter_h
#define frontend_BytecodeEmitter_h





#include "jscntxt.h"
#include "jsopcode.h"
#include "jsscript.h"

#include "frontend/ParseMaps.h"
#include "frontend/SourceNotes.h"

namespace js {
namespace frontend {

class FullParseHandler;
class ObjectBox;
class ParseNode;
template <typename ParseHandler> class Parser;
class SharedContext;
class TokenStream;

struct CGTryNoteList {
    Vector<JSTryNote> list;
    CGTryNoteList(ExclusiveContext *cx) : list(cx) {}

    bool append(JSTryNoteKind kind, unsigned stackDepth, size_t start, size_t end);
    size_t length() const { return list.length(); }
    void finish(TryNoteArray *array);
};

struct CGObjectList {
    uint32_t            length;     
    ObjectBox           *lastbox;   

    CGObjectList() : length(0), lastbox(nullptr) {}

    unsigned add(ObjectBox *objbox);
    unsigned indexOf(JSObject *obj);
    void finish(ObjectArray *array);
};

class CGConstList {
    Vector<Value> list;
  public:
    CGConstList(ExclusiveContext *cx) : list(cx) {}
    bool append(Value v) { JS_ASSERT_IF(v.isString(), v.toString()->isAtom()); return list.append(v); }
    size_t length() const { return list.length(); }
    void finish(ConstArray *array);
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

    StmtInfoBCE     *topStmt;       
    StmtInfoBCE     *topScopeStmt;  
    Rooted<StaticBlockObject *> blockChain;
                                    

    OwnedAtomIndexMapPtr atomIndices; 
    unsigned        firstLine;      

    int             stackDepth;     
    unsigned        maxStackDepth;  

    CGTryNoteList   tryNoteList;    

    unsigned        arrayCompDepth; 

    unsigned        emitLevel;      

    CGConstList     constList;      

    CGObjectList    objectList;     
    CGObjectList    regexpList;     


    uint16_t        typesetCount;   

    bool            hasSingletons:1;    

    bool            emittingForInit:1;  

    bool            emittingRunOnceLambda:1; 

    bool            lazyRunOnceLambda:1; 


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
                    HandleScript script, bool insideEval, HandleScript evalCaller,
                    bool hasGlobalScope, uint32_t lineNum, EmitterMode emitterMode = Normal);
    bool init();

    bool isAliasedName(ParseNode *pn);

    JS_ALWAYS_INLINE
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

    inline ptrdiff_t countFinalSourceNotes();

    bool reportError(ParseNode *pn, unsigned errorNumber, ...);
    bool reportStrictWarning(ParseNode *pn, unsigned errorNumber, ...);
    bool reportStrictModeError(ParseNode *pn, unsigned errorNumber, ...);
};




ptrdiff_t
Emit1(ExclusiveContext *cx, BytecodeEmitter *bce, JSOp op);




ptrdiff_t
Emit2(ExclusiveContext *cx, BytecodeEmitter *bce, JSOp op, jsbytecode op1);




ptrdiff_t
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
FinishTakingSrcNotes(ExclusiveContext *cx, BytecodeEmitter *bce, jssrcnote *notes);











inline ptrdiff_t
BytecodeEmitter::countFinalSourceNotes()
{
    ptrdiff_t diff = prologOffset() - prolog.lastNoteOffset;
    ptrdiff_t cnt = prolog.notes.length() + main.notes.length() + 1;
    if (prolog.notes.length() && prolog.currentLine != firstLine) {
        if (diff > SN_DELTA_MASK)
            cnt += JS_HOWMANY(diff - SN_DELTA_MASK, SN_XDELTA_MASK);
        cnt += 2 + ((firstLine > SN_3BYTE_OFFSET_MASK) << 1);
    } else if (diff > 0) {
        if (main.notes.length()) {
            jssrcnote *sn = main.notes.begin();
            diff -= SN_IS_XDELTA(sn)
                    ? SN_XDELTA_MASK - (*sn & SN_XDELTA_MASK)
                    : SN_DELTA_MASK - (*sn & SN_DELTA_MASK);
        }
        if (diff > 0)
            cnt += JS_HOWMANY(diff, SN_XDELTA_MASK);
    }
    return cnt;
}

} 
} 

#endif 
