





#ifndef COMPILIER_BUILT_IN_FUNCTION_EMULATOR_H_
#define COMPILIER_BUILT_IN_FUNCTION_EMULATOR_H_

#include "compiler/translator/InfoSink.h"
#include "compiler/translator/IntermNode.h"






class BuiltInFunctionEmulator {
public:
    BuiltInFunctionEmulator(sh::GLenum shaderType);
    
    
    
    
    
    bool SetFunctionCalled(TOperator op, const TType& param);
    bool SetFunctionCalled(
        TOperator op, const TType& param1, const TType& param2);
    bool SetFunctionCalled(
        TOperator op, const TType& param1, const TType& param2, const TType& param3);

    
    
    void OutputEmulatedFunctionDefinition(TInfoSinkBase& out, bool withPrecision) const;

    void MarkBuiltInFunctionsForEmulation(TIntermNode* root);

    void Cleanup();

    
    static TString GetEmulatedFunctionName(const TString& name);

private:
    
    
    
    enum TBuiltInFunction {
        TFunctionCos1 = 0,  
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

        TFunctionNormalize1,  
        TFunctionNormalize2,  
        TFunctionNormalize3,  
        TFunctionNormalize4,  

        TFunctionReflect1_1,  
        TFunctionReflect2_2,  
        TFunctionReflect3_3,  
        TFunctionReflect4_4,  

        TFunctionFaceForward1_1_1,  
        TFunctionFaceForward2_2_2,  
        TFunctionFaceForward3_3_3,  
        TFunctionFaceForward4_4_4,  

        TFunctionUnknown
    };

    TBuiltInFunction IdentifyFunction(TOperator op, const TType& param);
    TBuiltInFunction IdentifyFunction(
        TOperator op, const TType& param1, const TType& param2);
    TBuiltInFunction IdentifyFunction(
        TOperator op, const TType& param1, const TType& param2, const TType& param3);

    bool SetFunctionCalled(TBuiltInFunction function);

    std::vector<TBuiltInFunction> mFunctions;

    const bool* mFunctionMask;  
    const char** mFunctionSource;
};

#endif
