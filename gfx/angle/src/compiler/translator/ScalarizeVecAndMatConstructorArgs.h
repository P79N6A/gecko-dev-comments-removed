





#ifndef COMPILER_TRANSLATOR_SCALARIZE_VEC_AND_MAT_CONSTRUCTOR_ARGS_H_
#define COMPILER_TRANSLATOR_SCALARIZE_VEC_AND_MAT_CONSTRUCTOR_ARGS_H_

#include "compiler/translator/IntermNode.h"

class ScalarizeVecAndMatConstructorArgs : public TIntermTraverser
{
  public:
    ScalarizeVecAndMatConstructorArgs(sh::GLenum shaderType,
                                      bool fragmentPrecisionHigh)
        : mTempVarCount(0),
          mShaderType(shaderType),
          mFragmentPrecisionHigh(fragmentPrecisionHigh) {}

  protected:
    virtual bool visitAggregate(Visit visit, TIntermAggregate *node);

  private:
    void scalarizeArgs(TIntermAggregate *aggregate,
                       bool scalarizeVector, bool scalarizeMatrix);

    
    
    
    
    
    
    
    
    
    
    TString createTempVariable(TIntermTyped *original);

    std::vector<TIntermSequence> mSequenceStack;
    int mTempVarCount;

    sh::GLenum mShaderType;
    bool mFragmentPrecisionHigh;
};

#endif  
