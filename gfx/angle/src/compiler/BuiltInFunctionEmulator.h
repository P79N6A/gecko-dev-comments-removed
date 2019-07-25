





#ifndef COMPILIER_BUILT_IN_FUNCTION_EMULATOR_H_
#define COMPILIER_BUILT_IN_FUNCTION_EMULATOR_H_

#include "compiler/InfoSink.h"
#include "compiler/intermediate.h"





enum TBuiltInFunctionGroup {
    TFunctionGroupNormalize      = 1 << 0,
    TFunctionGroupAbs            = 1 << 1,
    TFunctionGroupSign           = 1 << 2,
    TFunctionGroupAll            =
        TFunctionGroupNormalize | TFunctionGroupAbs | TFunctionGroupSign
};






class BuiltInFunctionEmulator {
public:
    BuiltInFunctionEmulator();

    
    
    
    
    void SetFunctionGroupMask(unsigned int functionGroupMask);

    
    
    
    
    
    
    
    
    bool SetFunctionCalled(TOperator op, const TType& returnType);

    
    
    void OutputEmulatedFunctionDefinition(TInfoSinkBase& out, bool withPrecision) const;

    void MarkBuiltInFunctionsForEmulation(TIntermNode* root);

    
    static TString GetEmulatedFunctionName(const TString& name);

private:
    
    
    
    enum TBuiltInFunction {
        TFunctionNormalize1 = 0,  
        TFunctionNormalize2,  
        TFunctionNormalize3,  
        TFunctionNormalize4,  
        TFunctionAbs1,  
        TFunctionAbs2,  
        TFunctionAbs3,  
        TFunctionAbs4,  
        TFunctionSign1,  
        TFunctionSign2,  
        TFunctionSign3,  
        TFunctionSign4,  
        TFunctionUnknown
    };

    
    TBuiltInFunction IdentifyFunction(TOperator op, const TType& returnType);

    TVector<TBuiltInFunction> mFunctions;
    unsigned int mFunctionGroupMask;  
};

#endif
