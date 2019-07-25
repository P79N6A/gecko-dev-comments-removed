














































#if !defined(__SYMBOLS_H)
#define __SYMBOLS_H 1

#include "compiler/preprocessor/memory.h"

typedef enum symbolkind {
   MACRO_S
} symbolkind;



typedef struct Scope_Rec Scope;
typedef struct Symbol_Rec Symbol;

typedef struct SymbolList_Rec {
    struct SymbolList_Rec *next;
    Symbol *symb;
} SymbolList;

struct Scope_Rec {
    Scope *next, *prev;     
    Scope *parent;
    Scope *funScope;        
    MemoryPool *pool;       
    Symbol *symbols;
    
	int level;              

    
    SymbolList *programs;   
};




#include "compiler/preprocessor/cpp.h"        

struct Symbol_Rec {
    Symbol *left, *right;
    Symbol *next;
    int name;       
    SourceLoc loc;
    symbolkind kind;
    union {
        MacroSymbol mac;
    } details;
};

extern Scope *CurrentScope;
extern Scope *GlobalScope;
extern Scope *ScopeList;

Scope *NewScopeInPool(MemoryPool *);
#define NewScope()      NewScopeInPool(CurrentScope->pool)
void PushScope(Scope *fScope);
Scope *PopScope(void);
Symbol *NewSymbol(SourceLoc *loc, Scope *fScope, int name, symbolkind kind);
Symbol *AddSymbol(SourceLoc *loc, Scope *fScope, int atom, symbolkind kind);
Symbol *LookUpLocalSymbol(Scope *fScope, int atom);
Symbol *LookUpSymbol(Scope *fScope, int atom);
void CPPErrorToInfoLog(char *);


#endif 

