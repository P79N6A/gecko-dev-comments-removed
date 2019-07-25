





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

    virtual void visitSymbol(TIntermSymbol*);
    virtual void visitConstantUnion(TIntermConstantUnion*);
    virtual bool visitBinary(Visit, TIntermBinary*);
    virtual bool visitUnary(Visit, TIntermUnary*);
    virtual bool visitSelection(Visit, TIntermSelection*);
    virtual bool visitAggregate(Visit, TIntermAggregate*);
    virtual bool visitLoop(Visit, TIntermLoop*);
    virtual bool visitBranch(Visit, TIntermBranch*);

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
