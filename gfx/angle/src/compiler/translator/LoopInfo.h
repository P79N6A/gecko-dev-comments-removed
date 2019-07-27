





#ifndef COMPILER_TRANSLATOR_LOOP_INFO_H_
#define COMPILER_TRANSLATOR_LOOP_INFO_H_

#include "compiler/translator/IntermNode.h"

class TLoopIndexInfo
{
  public:
    TLoopIndexInfo();

    
    
    
    void fillInfo(TIntermLoop *node);

    int getId() const { return mId; }
    void setId(int id) { mId = id; }
    TBasicType getType() const { return mType; }
    void setType(TBasicType type) { mType = type; }
    int getCurrentValue() const { return mCurrentValue; }

    void step() { mCurrentValue += mIncrementValue; }

    
    bool satisfiesLoopCondition() const;

  private:
    int mId;
    TBasicType mType;  

    
    int mInitValue;
    int mStopValue;
    int mIncrementValue;
    TOperator mOp;
    int mCurrentValue;
};

struct TLoopInfo
{
    TLoopIndexInfo index;
    TIntermLoop *loop;

    TLoopInfo();
    TLoopInfo(TIntermLoop *node);
};

class TLoopStack : public TVector<TLoopInfo>
{
  public:
    
    TIntermLoop *findLoop(TIntermSymbol *symbol);

    
    TLoopIndexInfo *getIndexInfo(TIntermSymbol *symbol);

    
    void step();

    
    bool satisfiesLoopCondition();

    
    bool needsToReplaceSymbolWithValue(TIntermSymbol *symbol);

    
    int getLoopIndexValue(TIntermSymbol *symbol);

    void push(TIntermLoop *info);
    void pop();
};

#endif 

