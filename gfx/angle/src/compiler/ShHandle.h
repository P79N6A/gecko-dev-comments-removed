





#ifndef _SHHANDLE_INCLUDED_
#define _SHHANDLE_INCLUDED_








#include "GLSLANG/ShaderLang.h"

#include "compiler/BuiltInFunctionEmulator.h"
#include "compiler/ExtensionBehavior.h"
#include "compiler/InfoSink.h"
#include "compiler/SymbolTable.h"
#include "compiler/VariableInfo.h"

class TCompiler;




class TShHandleBase {
public:
    TShHandleBase();
    virtual ~TShHandleBase();
    virtual TCompiler* getAsCompiler() { return 0; }

protected:
    
    
    TPoolAllocator allocator;
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
    int getMappedNameMaxLength() const;

protected:
    ShShaderType getShaderType() const { return shaderType; }
    ShShaderSpec getShaderSpec() const { return shaderSpec; }
    
    bool InitBuiltInSymbolTable(const ShBuiltInResources& resources);
    
    void clearResults();
    
    bool detectRecursion(TIntermNode* root);
    
    
    bool validateLimitations(TIntermNode* root);
    
    void collectAttribsUniforms(TIntermNode* root);
    
    void mapLongVariableNames(TIntermNode* root);
    
    virtual void translate(TIntermNode* root) = 0;
    
    const TExtensionBehavior& getExtensionBehavior() const;

    const BuiltInFunctionEmulator& getBuiltInFunctionEmulator() const;

private:
    ShShaderType shaderType;
    ShShaderSpec shaderSpec;

    
    
    TSymbolTable symbolTable;
    
    TExtensionBehavior extensionBehavior;

    BuiltInFunctionEmulator builtInFunctionEmulator;

    
    TInfoSink infoSink;  
    TVariableInfoList attribs;  
    TVariableInfoList uniforms;  

    
    TMap<TString, TString> varyingLongNameMap;
};










TCompiler* ConstructCompiler(
    ShShaderType type, ShShaderSpec spec, ShShaderOutput output);
void DeleteCompiler(TCompiler*);

#endif 
