





#ifndef COMPILIER_BUILT_IN_FUNCTION_EMULATOR_H_
#define COMPILIER_BUILT_IN_FUNCTION_EMULATOR_H_

#include "GLSLANG/ShaderLang.h"

#include "compiler/InfoSink.h"
#include "compiler/intermediate.h"






class BuiltInFunctionEmulator {
public:
    BuiltInFunctionEmulator(ShShaderType shaderType);
    
    
    
    
    
    bool SetFunctionCalled(TOperator op, const TType& param);
    bool SetFunctionCalled(
        TOperator op, const TType& param1, const TType& param2);

    
    
    void OutputEmulatedFunctionDefinition(TInfoSinkBase& out, bool withPrecision) const;

    void MarkBuiltInFunctionsForEmulation(TIntermNode* root);

    void Cleanup();

    
    static TString GetEmulatedFunctionName(const TString& name);

private:
    
    
    
    enum TBuiltInFunction {
        TFunctionAtan1_1 = 0,  
        TFunctionAtan2_2,  
        TFunctionAtan3_3,  
        TFunctionAtan4_4,  

        TFunctionCos1,  
        TFunctionCos2,  
        TFunctionCos3,  
        TFunctionCos4,  

        TFunctionDistance1_1,  
        TFunctionDistance2_2,  
        TFunctionDistance3_3,  
        TFunctionDistance4_4,  

        TFunctionDot1_1,  
        TFunctionDot2_2,  
        TFunctionDot3_3,  
        TFunctionDot4_4,  

        TFunctionLength1,  
        TFunctionLength2,  
        TFunctionLength3,  
        TFunctionLength4,  

        TFunctionMod1_1,  
        TFunctionMod2_2,  
        TFunctionMod3_3,  
        TFunctionMod4_4,  

        TFunctionNormalize1,  
        TFunctionNormalize2,  
        TFunctionNormalize3,  
        TFunctionNormalize4,  

        TFunctionReflect1_1,  
        TFunctionReflect2_2,  
        TFunctionReflect3_3,  
        TFunctionReflect4_4,  

        TFunctionUnknown
    };

    TBuiltInFunction IdentifyFunction(TOperator op, const TType& param);
    TBuiltInFunction IdentifyFunction(
        TOperator op, const TType& param1, const TType& param2);

    bool SetFunctionCalled(TBuiltInFunction function);

    std::vector<TBuiltInFunction> mFunctions;

    const bool* mFunctionMask;  
    const char** mFunctionSource;
};

#endif
