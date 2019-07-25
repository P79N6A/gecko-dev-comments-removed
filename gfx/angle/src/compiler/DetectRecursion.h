





#ifndef COMPILER_DETECT_RECURSION_H_
#define COMPILER_DETECT_RECURSION_H_

#include "GLSLANG/ShaderLang.h"

#include "compiler/intermediate.h"
#include "compiler/VariableInfo.h"


class DetectRecursion : public TIntermTraverser {
public:
    enum ErrorCode {
        kErrorMissingMain,
        kErrorRecursion,
        kErrorNone
    };

    DetectRecursion();
    ~DetectRecursion();

    virtual bool visitAggregate(Visit, TIntermAggregate*);

    ErrorCode detectRecursion();

private:
    class FunctionNode {
    public:
        FunctionNode(const TString& fname);

        const TString& getName() const;

        
        void addCallee(FunctionNode* callee);

        
        bool detectRecursion();

    private:
        
        TString name;

        
        TVector<FunctionNode*> callees;

        Visit visit;
    };

    FunctionNode* findFunctionByName(const TString& name);

    TVector<FunctionNode*> functions;
    FunctionNode* currentFunction;
};

#endif
