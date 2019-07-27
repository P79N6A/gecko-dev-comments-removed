





#ifndef COMPILER_DETECT_RECURSION_H_
#define COMPILER_DETECT_RECURSION_H_

#include <limits.h>
#include "compiler/translator/IntermNode.h"
#include "compiler/translator/VariableInfo.h"

class TInfoSink;


class DetectCallDepth : public TIntermTraverser {
public:
    enum ErrorCode {
        kErrorMissingMain,
        kErrorRecursion,
        kErrorMaxDepthExceeded,
        kErrorNone
    };

    DetectCallDepth(TInfoSink& infoSync, bool limitCallStackDepth, int maxCallStackDepth);
    ~DetectCallDepth();

    virtual bool visitAggregate(Visit, TIntermAggregate*);

    bool checkExceedsMaxDepth(int depth);

    ErrorCode detectCallDepth();

private:
    class FunctionNode {
    public:
        static const int kInfiniteCallDepth = INT_MAX;

        FunctionNode(const TString& fname);

        const TString& getName() const;

        
        void addCallee(FunctionNode* callee);

        
        int detectCallDepth(DetectCallDepth* detectCallDepth, int depth);

        
        void reset();

    private:
        
        TString name;

        
        TVector<FunctionNode*> callees;

        Visit visit;
    };

    ErrorCode detectCallDepthForFunction(FunctionNode* func);
    FunctionNode* findFunctionByName(const TString& name);
    void resetFunctionNodes();

    TInfoSink& getInfoSink() { return infoSink; }

    TVector<FunctionNode*> functions;
    FunctionNode* currentFunction;
    TInfoSink& infoSink;
    int maxDepth;

    DetectCallDepth(const DetectCallDepth&);
    void operator=(const DetectCallDepth&);
};

#endif
