






#ifndef BytecodeEmitter_h__
#define BytecodeEmitter_h__




#include "jstypes.h"
#include "jsatom.h"
#include "jsopcode.h"
#include "jsscript.h"
#include "jsprvtd.h"
#include "jspubtd.h"

#include "frontend/BytecodeCompiler.h"
#include "frontend/Parser.h"
#include "frontend/ParseMaps.h"
#include "frontend/SharedContext.h"

#include "vm/ScopeObject.h"

namespace js {
namespace frontend {

struct CGTryNoteList {
    Vector<JSTryNote> list;
    CGTryNoteList(JSContext *cx) : list(cx) {}

    bool append(JSTryNoteKind kind, unsigned stackDepth, size_t start, size_t end);
    size_t length() const { return list.length(); }
    void finish(TryNoteArray *array);
};

struct CGObjectList {
    uint32_t            length;     
    ObjectBox           *lastbox;   

    CGObjectList() : length(0), lastbox(NULL) {}

    unsigned add(ObjectBox *objbox);
    unsigned indexOf(JSObject *obj);
    void finish(ObjectArray *array);
};

class CGConstList {
    Vector<Value> list;
  public:
    CGConstList(JSContext *cx) : list(cx) {}
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
        unsigned    currentLine;    
        unsigned    lastColumn;     


        EmitSection(JSContext *cx, unsigned lineno)
          : code(cx), notes(cx), lastNoteOffset(0), currentLine(lineno), lastColumn(0)
        {
            
            
            code.reserve(1024);
            notes.reserve(1024);
        }
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


    const bool      hasGlobalScope:1;   


    const bool      selfHostingMode:1;  




    





    BytecodeEmitter(BytecodeEmitter *parent, Parser<FullParseHandler> *parser, SharedContext *sc,
                    HandleScript script, HandleScript evalCaller, bool hasGlobalScope,
                    unsigned lineno, bool selfHostingMode = false);
    bool init();

    ~BytecodeEmitter();

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

    void tellDebuggerAboutCompiledScript(JSContext *cx);

    TokenStream *tokenStream() { return &parser->tokenStream; }

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
Emit1(JSContext *cx, BytecodeEmitter *bce, JSOp op);




ptrdiff_t
Emit2(JSContext *cx, BytecodeEmitter *bce, JSOp op, jsbytecode op1);




ptrdiff_t
Emit3(JSContext *cx, BytecodeEmitter *bce, JSOp op, jsbytecode op1, jsbytecode op2);




ptrdiff_t
EmitN(JSContext *cx, BytecodeEmitter *bce, JSOp op, size_t extra);




bool
EmitTree(JSContext *cx, BytecodeEmitter *bce, ParseNode *pn);




bool
EmitFunctionScript(JSContext *cx, BytecodeEmitter *bce, ParseNode *body);

} 























enum SrcNoteType {
    SRC_NULL        = 0,        

    SRC_IF          = 1,        
    SRC_IF_ELSE     = 2,        
    SRC_COND        = 3,        

    SRC_FOR         = 4,        

    SRC_WHILE       = 5,        


    SRC_FOR_IN      = 6,        

    SRC_CONTINUE    = 7,        
    SRC_BREAK       = 8,        
    SRC_BREAK2LABEL = 9,        
    SRC_SWITCHBREAK = 10,       

    SRC_TABLESWITCH = 11,       

    SRC_CONDSWITCH  = 12,       


    SRC_PCDELTA     = 13,       



    SRC_ASSIGNOP    = 14,       

    SRC_HIDDEN      = 15,       

    SRC_CATCH       = 16,       

    
    SRC_LAST_GETTABLE = SRC_CATCH,

    SRC_COLSPAN     = 17,       
    SRC_NEWLINE     = 18,       
    SRC_SETLINE     = 19,       

    SRC_UNUSED20    = 20,
    SRC_UNUSED21    = 21,
    SRC_UNUSED22    = 22,
    SRC_UNUSED23    = 23,

    SRC_XDELTA      = 24        
};

#define SN_TYPE_BITS            5
#define SN_DELTA_BITS           3
#define SN_XDELTA_BITS          6
#define SN_TYPE_MASK            (JS_BITMASK(SN_TYPE_BITS) << SN_DELTA_BITS)
#define SN_DELTA_MASK           ((ptrdiff_t)JS_BITMASK(SN_DELTA_BITS))
#define SN_XDELTA_MASK          ((ptrdiff_t)JS_BITMASK(SN_XDELTA_BITS))

#define SN_MAKE_NOTE(sn,t,d)    (*(sn) = (jssrcnote)                          \
                                          (((t) << SN_DELTA_BITS)             \
                                           | ((d) & SN_DELTA_MASK)))
#define SN_MAKE_XDELTA(sn,d)    (*(sn) = (jssrcnote)                          \
                                          ((SRC_XDELTA << SN_DELTA_BITS)      \
                                           | ((d) & SN_XDELTA_MASK)))

#define SN_IS_XDELTA(sn)        ((*(sn) >> SN_DELTA_BITS) >= SRC_XDELTA)
#define SN_TYPE(sn)             ((js::SrcNoteType)(SN_IS_XDELTA(sn)           \
                                                   ? SRC_XDELTA               \
                                                   : *(sn) >> SN_DELTA_BITS))
#define SN_SET_TYPE(sn,type)    SN_MAKE_NOTE(sn, type, SN_DELTA(sn))
#define SN_IS_GETTABLE(sn)      (SN_TYPE(sn) <= SRC_LAST_GETTABLE)

#define SN_DELTA(sn)            ((ptrdiff_t)(SN_IS_XDELTA(sn)                 \
                                             ? *(sn) & SN_XDELTA_MASK         \
                                             : *(sn) & SN_DELTA_MASK))
#define SN_SET_DELTA(sn,delta)  (SN_IS_XDELTA(sn)                             \
                                 ? SN_MAKE_XDELTA(sn, delta)                  \
                                 : SN_MAKE_NOTE(sn, SN_TYPE(sn), delta))

#define SN_DELTA_LIMIT          ((ptrdiff_t)JS_BIT(SN_DELTA_BITS))
#define SN_XDELTA_LIMIT         ((ptrdiff_t)JS_BIT(SN_XDELTA_BITS))






#define SN_3BYTE_OFFSET_FLAG    0x80
#define SN_3BYTE_OFFSET_MASK    0x7f












#define SN_COLSPAN_DOMAIN       ptrdiff_t(SN_3BYTE_OFFSET_FLAG << 16)

#define SN_MAX_OFFSET ((size_t)((ptrdiff_t)SN_3BYTE_OFFSET_FLAG << 16) - 1)

#define SN_LENGTH(sn)           ((js_SrcNoteSpec[SN_TYPE(sn)].arity == 0) ? 1 \
                                 : js_SrcNoteLength(sn))
#define SN_NEXT(sn)             ((sn) + SN_LENGTH(sn))


#define SN_MAKE_TERMINATOR(sn)  (*(sn) = SRC_NULL)
#define SN_IS_TERMINATOR(sn)    (*(sn) == SRC_NULL)

namespace frontend {







int
NewSrcNote(JSContext *cx, BytecodeEmitter *bce, SrcNoteType type);

int
NewSrcNote2(JSContext *cx, BytecodeEmitter *bce, SrcNoteType type, ptrdiff_t offset);

int
NewSrcNote3(JSContext *cx, BytecodeEmitter *bce, SrcNoteType type, ptrdiff_t offset1,
               ptrdiff_t offset2);


bool
AddToSrcNoteDelta(JSContext *cx, BytecodeEmitter *bce, jssrcnote *sn, ptrdiff_t delta);

bool
FinishTakingSrcNotes(JSContext *cx, BytecodeEmitter *bce, jssrcnote *notes);











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

struct JSSrcNoteSpec {
    const char      *name;      
    int8_t          arity;      
};

extern JS_FRIEND_DATA(JSSrcNoteSpec)  js_SrcNoteSpec[];
extern JS_FRIEND_API(unsigned)         js_SrcNoteLength(jssrcnote *sn);




extern JS_FRIEND_API(ptrdiff_t)
js_GetSrcNoteOffset(jssrcnote *sn, unsigned which);

#endif 
