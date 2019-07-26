





#ifndef ForLoopUnroll_h
#define ForLoopUnroll_h

#include "compiler/intermediate.h"

struct TLoopIndexInfo {
    int id;
    int initValue;
    int stopValue;
    int incrementValue;
    TOperator op;
    int currentValue;
};

class ForLoopUnroll {
public:
    ForLoopUnroll() { }

    void FillLoopIndexInfo(TIntermLoop* node, TLoopIndexInfo& info);

    
    void Step();

    
    bool SatisfiesLoopCondition();

    
    bool NeedsToReplaceSymbolWithValue(TIntermSymbol* symbol);

    
    int GetLoopIndexValue(TIntermSymbol* symbol);

    void Push(TLoopIndexInfo& info);
    void Pop();

    static void MarkForLoopsWithIntegerIndicesForUnrolling(TIntermNode* root);

private:
    int getLoopIncrement(TIntermLoop* node);

    int evaluateIntConstant(TIntermConstantUnion* node);

    TVector<TLoopIndexInfo> mLoopIndexStack;
};

#endif
