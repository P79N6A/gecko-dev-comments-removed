





#ifndef frontend_SharedContext_h
#define frontend_SharedContext_h

#include "jsatom.h"
#include "jsopcode.h"
#include "jspubtd.h"
#include "jsscript.h"
#include "jstypes.h"

#include "frontend/ParseMaps.h"
#include "frontend/ParseNode.h"
#include "frontend/TokenStream.h"
#include "vm/ScopeObject.h"

namespace js {
namespace frontend {


class AnyContextFlags
{
    
    friend class SharedContext;

    
    bool            hasExplicitUseStrict:1;

    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    bool            bindingsAccessedDynamically:1;

    
    
    
    bool            hasDebuggerStatement:1;

    
    bool            hasDirectEval:1;

  public:
    AnyContextFlags()
     :  hasExplicitUseStrict(false),
        bindingsAccessedDynamically(false),
        hasDebuggerStatement(false),
        hasDirectEval(false)
    { }
};

class FunctionContextFlags
{
    
    friend class FunctionBox;

    
    
    bool mightAliasLocals:1;

    
    
    
    
    
    
    
    bool hasExtensibleScope:1;

    
    
    bool needsDeclEnvObject:1;

    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    bool argumentsHasLocalBinding:1;

    
    
    
    
    
    
    
    
    
    bool definitelyNeedsArgsObj:1;

  public:
    FunctionContextFlags()
     :  mightAliasLocals(false),
        hasExtensibleScope(false),
        needsDeclEnvObject(false),
        argumentsHasLocalBinding(false),
        definitelyNeedsArgsObj(false)
    { }
};

class GlobalSharedContext;


class Directives
{
    bool strict_;
    bool asmJS_;

  public:
    explicit Directives(bool strict) : strict_(strict), asmJS_(false) {}
    template <typename ParseHandler> explicit Directives(ParseContext<ParseHandler>* parent);

    void setStrict() { strict_ = true; }
    bool strict() const { return strict_; }

    void setAsmJS() { asmJS_ = true; }
    bool asmJS() const { return asmJS_; }

    Directives& operator=(Directives rhs) {
        strict_ = rhs.strict_;
        asmJS_ = rhs.asmJS_;
        return *this;
    }
    bool operator==(const Directives& rhs) const {
        return strict_ == rhs.strict_ && asmJS_ == rhs.asmJS_;
    }
    bool operator!=(const Directives& rhs) const {
        return !(*this == rhs);
    }
};







class SharedContext
{
  public:
    ExclusiveContext* const context;
    AnyContextFlags anyCxFlags;
    bool strictScript;
    bool localStrict;
    bool extraWarnings;

    
    
    SharedContext(ExclusiveContext* cx, Directives directives, bool extraWarnings)
      : context(cx),
        anyCxFlags(),
        strictScript(directives.strict()),
        localStrict(false),
        extraWarnings(extraWarnings)
    {}

    virtual ObjectBox* toObjectBox() = 0;
    inline bool isFunctionBox() { return toObjectBox() && toObjectBox()->isFunctionBox(); }
    inline FunctionBox* asFunctionBox();

    bool hasExplicitUseStrict()        const { return anyCxFlags.hasExplicitUseStrict; }
    bool bindingsAccessedDynamically() const { return anyCxFlags.bindingsAccessedDynamically; }
    bool hasDebuggerStatement()        const { return anyCxFlags.hasDebuggerStatement; }
    bool hasDirectEval()               const { return anyCxFlags.hasDirectEval; }

    void setExplicitUseStrict()           { anyCxFlags.hasExplicitUseStrict        = true; }
    void setBindingsAccessedDynamically() { anyCxFlags.bindingsAccessedDynamically = true; }
    void setHasDebuggerStatement()        { anyCxFlags.hasDebuggerStatement        = true; }
    void setHasDirectEval()               { anyCxFlags.hasDirectEval               = true; }

    inline bool allLocalsAliased();

    bool strict() {
        return strictScript || localStrict;
    }
    bool setLocalStrictMode(bool strict) {
        bool retVal = localStrict;
        localStrict = strict;
        return retVal;
    }

    
    bool needStrictChecks() {
        return strict() || extraWarnings;
    }

    bool isDotVariable(JSAtom* atom) const {
        return atom == context->names().dotGenerator || atom == context->names().dotGenRVal;
    }
};

class GlobalSharedContext : public SharedContext
{
  public:
    GlobalSharedContext(ExclusiveContext* cx,
                        Directives directives, bool extraWarnings)
      : SharedContext(cx, directives, extraWarnings)
    {}

    ObjectBox* toObjectBox() { return nullptr; }
};

class FunctionBox : public ObjectBox, public SharedContext
{
  public:
    Bindings        bindings;               
    uint32_t        bufStart;
    uint32_t        bufEnd;
    uint32_t        startLine;
    uint32_t        startColumn;
    uint16_t        length;

    uint8_t         generatorKindBits_;     
    bool            inWith:1;               
    bool            inGenexpLambda:1;       
    bool            hasDestructuringArgs:1; 
    bool            useAsm:1;               
    bool            insideUseAsm:1;         

    
    bool            usesArguments:1;  
    bool            usesApply:1;      
    bool            usesThis:1;       

    FunctionContextFlags funCxFlags;

    template <typename ParseHandler>
    FunctionBox(ExclusiveContext* cx, ObjectBox* traceListHead, JSFunction* fun,
                ParseContext<ParseHandler>* pc, Directives directives,
                bool extraWarnings, GeneratorKind generatorKind);

    ObjectBox* toObjectBox() { return this; }
    JSFunction* function() const { return &object->as<JSFunction>(); }

    GeneratorKind generatorKind() const { return GeneratorKindFromBits(generatorKindBits_); }
    bool isGenerator() const { return generatorKind() != NotGenerator; }
    bool isLegacyGenerator() const { return generatorKind() == LegacyGenerator; }
    bool isStarGenerator() const { return generatorKind() == StarGenerator; }

    void setGeneratorKind(GeneratorKind kind) {
        
        
        
        MOZ_ASSERT(!isGenerator());
        generatorKindBits_ = GeneratorKindAsBits(kind);
    }

    bool mightAliasLocals()         const { return funCxFlags.mightAliasLocals; }
    bool hasExtensibleScope()       const { return funCxFlags.hasExtensibleScope; }
    bool needsDeclEnvObject()       const { return funCxFlags.needsDeclEnvObject; }
    bool argumentsHasLocalBinding() const { return funCxFlags.argumentsHasLocalBinding; }
    bool definitelyNeedsArgsObj()   const { return funCxFlags.definitelyNeedsArgsObj; }

    void setMightAliasLocals()             { funCxFlags.mightAliasLocals         = true; }
    void setHasExtensibleScope()           { funCxFlags.hasExtensibleScope       = true; }
    void setNeedsDeclEnvObject()           { funCxFlags.needsDeclEnvObject       = true; }
    void setArgumentsHasLocalBinding()     { funCxFlags.argumentsHasLocalBinding = true; }
    void setDefinitelyNeedsArgsObj()       { MOZ_ASSERT(funCxFlags.argumentsHasLocalBinding);
                                             funCxFlags.definitelyNeedsArgsObj   = true; }

    bool hasDefaults() const {
        return length != function()->nargs() - function()->hasRest();
    }

    
    
    
    
    
    bool useAsmOrInsideUseAsm() const {
        return useAsm || insideUseAsm;
    }

    void setStart(const TokenStream& tokenStream) {
        bufStart = tokenStream.currentToken().pos.begin;
        startLine = tokenStream.getLineno();
        startColumn = tokenStream.getColumn();
    }

    bool isHeavyweight()
    {
        
        return bindings.hasAnyAliasedBindings() ||
               hasExtensibleScope() ||
               needsDeclEnvObject() ||
               isGenerator();
    }
};

inline FunctionBox*
SharedContext::asFunctionBox()
{
    MOZ_ASSERT(isFunctionBox());
    return static_cast<FunctionBox*>(this);
}






inline bool
SharedContext::allLocalsAliased()
{
    return bindingsAccessedDynamically() || (isFunctionBox() && asFunctionBox()->isGenerator());
}











enum StmtType {
    STMT_LABEL,                 
    STMT_IF,                    
    STMT_ELSE,                  
    STMT_SEQ,                   
    STMT_BLOCK,                 
    STMT_SWITCH,                
    STMT_WITH,                  
    STMT_CATCH,                 
    STMT_TRY,                   
    STMT_FINALLY,               
    STMT_SUBROUTINE,            
    STMT_DO_LOOP,               
    STMT_FOR_LOOP,              
    STMT_FOR_IN_LOOP,           
    STMT_FOR_OF_LOOP,           
    STMT_WHILE_LOOP,            
    STMT_SPREAD,                
    STMT_LIMIT
};



























struct StmtInfoBase {
    
    uint16_t        type;

    
    
    
    bool isBlockScope:1;

    
    bool isNestedScope:1;

    
    bool isForLetBlock:1;

    
    RootedAtom      label;

    
    
    Rooted<NestedScopeObject*> staticScope;

    explicit StmtInfoBase(ExclusiveContext* cx)
        : isBlockScope(false), isNestedScope(false), isForLetBlock(false),
          label(cx), staticScope(cx)
    {}

    bool maybeScope() const {
        return STMT_BLOCK <= type && type <= STMT_SUBROUTINE && type != STMT_WITH;
    }

    bool linksScope() const {
        return isNestedScope;
    }

    void setStaticScope() {
    }

    StaticBlockObject& staticBlock() const {
        MOZ_ASSERT(isNestedScope);
        MOZ_ASSERT(isBlockScope);
        return staticScope->as<StaticBlockObject>();
    }

    bool isLoop() const {
        return type >= STMT_DO_LOOP;
    }

    bool isTrying() const {
        return STMT_TRY <= type && type <= STMT_SUBROUTINE;
    }
};


template <class ContextT>
void
PushStatement(ContextT* ct, typename ContextT::StmtInfo* stmt, StmtType type)
{
    stmt->type = type;
    stmt->isBlockScope = false;
    stmt->isNestedScope = false;
    stmt->isForLetBlock = false;
    stmt->label = nullptr;
    stmt->staticScope = nullptr;
    stmt->down = ct->topStmt;
    ct->topStmt = stmt;
    if (stmt->linksScope()) {
        stmt->downScope = ct->topScopeStmt;
        ct->topScopeStmt = stmt;
    } else {
        stmt->downScope = nullptr;
    }
}

template <class ContextT>
void
FinishPushNestedScope(ContextT* ct, typename ContextT::StmtInfo* stmt, NestedScopeObject& staticScope)
{
    stmt->isNestedScope = true;
    stmt->downScope = ct->topScopeStmt;
    ct->topScopeStmt = stmt;
    ct->staticScope = &staticScope;
    stmt->staticScope = &staticScope;
}




template <class ContextT>
void
FinishPopStatement(ContextT* ct)
{
    typename ContextT::StmtInfo* stmt = ct->topStmt;
    ct->topStmt = stmt->down;
    if (stmt->linksScope()) {
        ct->topScopeStmt = stmt->downScope;
        if (stmt->isNestedScope) {
            MOZ_ASSERT(stmt->staticScope);
            ct->staticScope = stmt->staticScope->enclosingNestedScope();
        }
    }
}















template <class ContextT>
typename ContextT::StmtInfo*
LexicalLookup(ContextT* ct, HandleAtom atom, int* slotp, typename ContextT::StmtInfo* stmt);

} 

} 

#endif 
