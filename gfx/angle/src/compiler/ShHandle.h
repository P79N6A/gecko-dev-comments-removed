





#ifndef _SHHANDLE_INCLUDED_
#define _SHHANDLE_INCLUDED_








#include "GLSLANG/ShaderLang.h"

#include "compiler/BuiltInFunctionEmulator.h"
#include "compiler/ExtensionBehavior.h"
#include "compiler/HashNames.h"
#include "compiler/InfoSink.h"
#include "compiler/SymbolTable.h"
#include "compiler/VariableInfo.h"
#include "third_party/compiler/ArrayBoundsClamper.h"

class LongNameMap;
class TCompiler;
class TDependencyGraph;
class TranslatorHLSL;





bool isWebGLBasedSpec(ShShaderSpec spec);




class TShHandleBase {
public:
    TShHandleBase();
    virtual ~TShHandleBase();
    virtual TCompiler* getAsCompiler() { return 0; }
    virtual TranslatorHLSL* getAsTranslatorHLSL() { return 0; }

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
                 size_t numStrings,
                 int compileOptions);

    
    TInfoSink& getInfoSink() { return infoSink; }
    const TVariableInfoList& getAttribs() const { return attribs; }
    const TVariableInfoList& getUniforms() const { return uniforms; }
    int getMappedNameMaxLength() const;

    ShHashFunction64 getHashFunction() const { return hashFunction; }
    NameMap& getNameMap() { return nameMap; }
    TSymbolTable& getSymbolTable() { return symbolTable; }

protected:
    ShShaderType getShaderType() const { return shaderType; }
    ShShaderSpec getShaderSpec() const { return shaderSpec; }
    
    bool InitBuiltInSymbolTable(const ShBuiltInResources& resources);
    
    void clearResults();
    
    bool detectCallDepth(TIntermNode* root, TInfoSink& infoSink, bool limitCallStackDepth);
    
    void rewriteCSSShader(TIntermNode* root);
    
    
    bool validateLimitations(TIntermNode* root);
    
    void collectAttribsUniforms(TIntermNode* root);
    
    void mapLongVariableNames(TIntermNode* root);
    
    virtual void translate(TIntermNode* root) = 0;
    
    
    bool enforcePackingRestrictions();
    
    bool enforceTimingRestrictions(TIntermNode* root, bool outputGraph);
    
    bool enforceVertexShaderTimingRestrictions(TIntermNode* root);
    
    
    bool enforceFragmentShaderTimingRestrictions(const TDependencyGraph& graph);
    
    bool limitExpressionComplexity(TIntermNode* root);
    
    const TExtensionBehavior& getExtensionBehavior() const;
    
    const ShBuiltInResources& getResources() const;

    const ArrayBoundsClamper& getArrayBoundsClamper() const;
    ShArrayIndexClampingStrategy getArrayIndexClampingStrategy() const;
    const BuiltInFunctionEmulator& getBuiltInFunctionEmulator() const;

private:
    ShShaderType shaderType;
    ShShaderSpec shaderSpec;

    int maxUniformVectors;
    int maxExpressionComplexity;
    int maxCallStackDepth;

    ShBuiltInResources compileResources;

    
    
    TSymbolTable symbolTable;
    
    TExtensionBehavior extensionBehavior;
    bool fragmentPrecisionHigh;

    ArrayBoundsClamper arrayBoundsClamper;
    ShArrayIndexClampingStrategy clampingStrategy;
    BuiltInFunctionEmulator builtInFunctionEmulator;

    
    TInfoSink infoSink;  
    TVariableInfoList attribs;  
    TVariableInfoList uniforms;  

    
    LongNameMap* longNameMap;

    
    ShHashFunction64 hashFunction;
    NameMap nameMap;
};










TCompiler* ConstructCompiler(
    ShShaderType type, ShShaderSpec spec, ShShaderOutput output);
void DeleteCompiler(TCompiler*);

#endif 
