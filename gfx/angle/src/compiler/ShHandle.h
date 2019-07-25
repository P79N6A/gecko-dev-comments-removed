





#ifndef _SHHANDLE_INCLUDED_
#define _SHHANDLE_INCLUDED_








#include "GLSLANG/ShaderLang.h"

#include "compiler/InfoSink.h"
#include "compiler/SymbolTable.h"

class TCompiler;
class TIntermNode;




class TShHandleBase {
public:
    TShHandleBase() { }
    virtual ~TShHandleBase() { }
    virtual TCompiler* getAsCompiler() { return 0; }
};





class TCompiler : public TShHandleBase {
public:
    TCompiler(EShLanguage l, EShSpec s) : language(l), spec(s) { }
    virtual ~TCompiler() { }

    EShLanguage getLanguage() const { return language; }
    EShSpec getSpec() const { return spec; }
    TSymbolTable& getSymbolTable() { return symbolTable; }
    TInfoSink& getInfoSink() { return infoSink; }

    virtual bool compile(TIntermNode* root) = 0;

    virtual TCompiler* getAsCompiler() { return this; }

protected:
    EShLanguage language;
    EShSpec spec;

    
    
    TSymbolTable symbolTable;
    
    TInfoSink infoSink;
};










TCompiler* ConstructCompiler(EShLanguage, EShSpec);
void DeleteCompiler(TCompiler*);

#endif 
