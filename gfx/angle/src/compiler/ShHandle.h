





#ifndef _SHHANDLE_INCLUDED_
#define _SHHANDLE_INCLUDED_








#include "GLSLANG/ShaderLang.h"

#include "compiler/ExtensionBehavior.h"
#include "compiler/InfoSink.h"
#include "compiler/SymbolTable.h"
#include "compiler/VariableInfo.h"

class TCompiler;




class TShHandleBase {
public:
    TShHandleBase() { }
    virtual ~TShHandleBase() { }
    virtual TCompiler* getAsCompiler() { return 0; }
};





class TCompiler : public TShHandleBase {
public:
    TCompiler(ShShaderType type, ShShaderSpec spec);
    virtual ~TCompiler();
    virtual TCompiler* getAsCompiler() { return this; }

    bool Init(const ShBuiltInResources& resources);
    bool compile(const char* const shaderStrings[],
                 const int numStrings,
                 int compileOptions);

    
    TInfoSink& getInfoSink() { return infoSink; }
    const TVariableInfoList& getAttribs() const { return attribs; }
    const TVariableInfoList& getUniforms() const { return uniforms; }

protected:
    
    bool InitBuiltInSymbolTable(const ShBuiltInResources& resources);
    
    void clearResults();
    
    void collectAttribsUniforms(TIntermNode* root);
    
    virtual void translate(TIntermNode* root) = 0;

private:
    ShShaderType shaderType;
    ShShaderSpec shaderSpec;

    
    
    TSymbolTable symbolTable;
    
    TExtensionBehavior extensionBehavior;

    
    TInfoSink infoSink;  
    TVariableInfoList attribs;  
    TVariableInfoList uniforms;  
};










TCompiler* ConstructCompiler(ShShaderType type, ShShaderSpec spec);
void DeleteCompiler(TCompiler*);

#endif 
