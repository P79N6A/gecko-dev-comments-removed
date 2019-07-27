





#ifndef _SHHANDLE_INCLUDED_
#define _SHHANDLE_INCLUDED_








#include "compiler/translator/BuiltInFunctionEmulator.h"
#include "compiler/translator/ExtensionBehavior.h"
#include "compiler/translator/HashNames.h"
#include "compiler/translator/InfoSink.h"
#include "compiler/translator/SymbolTable.h"
#include "compiler/translator/VariableInfo.h"
#include "third_party/compiler/ArrayBoundsClamper.h"

class TCompiler;
class TDependencyGraph;
class TranslatorHLSL;





bool IsWebGLBasedSpec(ShShaderSpec spec);




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
    TCompiler(ShShaderType type, ShShaderSpec spec, ShShaderOutput output);
    virtual ~TCompiler();
    virtual TCompiler* getAsCompiler() { return this; }

    bool Init(const ShBuiltInResources& resources);
    bool compile(const char* const shaderStrings[],
                 size_t numStrings,
                 int compileOptions);

    
    int getShaderVersion() const { return shaderVersion; }
    TInfoSink& getInfoSink() { return infoSink; }
    const TVariableInfoList& getAttribs() const { return attribs; }
    const TVariableInfoList& getUniforms() const { return uniforms; }
    const TVariableInfoList& getVaryings() const { return varyings; }

    ShHashFunction64 getHashFunction() const { return hashFunction; }
    NameMap& getNameMap() { return nameMap; }
    TSymbolTable& getSymbolTable() { return symbolTable; }
    ShShaderSpec getShaderSpec() const { return shaderSpec; }
    ShShaderOutput getOutputType() const { return outputType; }
    std::string getBuiltInResourcesString() const { return builtInResourcesString; }

protected:
    ShShaderType getShaderType() const { return shaderType; }
    
    bool InitBuiltInSymbolTable(const ShBuiltInResources& resources);
    
    void setResourceString();
    
    void clearResults();
    
    bool detectCallDepth(TIntermNode* root, TInfoSink& infoSink, bool limitCallStackDepth);
    
    bool validateOutputs(TIntermNode* root);
    
    void rewriteCSSShader(TIntermNode* root);
    
    
    bool validateLimitations(TIntermNode* root);
    
    void collectVariables(TIntermNode* root);
    
    virtual void translate(TIntermNode* root) = 0;
    
    
    bool enforcePackingRestrictions();
    
    
    
    
    void initializeVaryingsWithoutStaticUse(TIntermNode* root);
    
    
    
    
    void initializeGLPosition(TIntermNode* root);
    
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
    ShShaderOutput outputType;

    int maxUniformVectors;
    int maxExpressionComplexity;
    int maxCallStackDepth;

    ShBuiltInResources compileResources;
    std::string builtInResourcesString;

    
    
    TSymbolTable symbolTable;
    
    TExtensionBehavior extensionBehavior;
    bool fragmentPrecisionHigh;

    ArrayBoundsClamper arrayBoundsClamper;
    ShArrayIndexClampingStrategy clampingStrategy;
    BuiltInFunctionEmulator builtInFunctionEmulator;

    
    int shaderVersion;
    TInfoSink infoSink;  
    TVariableInfoList attribs;  
    TVariableInfoList uniforms;  
    TVariableInfoList varyings;  

    
    ShHashFunction64 hashFunction;
    NameMap nameMap;
};










TCompiler* ConstructCompiler(
    ShShaderType type, ShShaderSpec spec, ShShaderOutput output);
void DeleteCompiler(TCompiler*);

#endif 
