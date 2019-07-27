





#ifndef COMPILER_VERSIONGLSL_H_
#define COMPILER_VERSIONGLSL_H_

#include "compiler/translator/intermediate.h"
















class TVersionGLSL : public TIntermTraverser {
public:
    TVersionGLSL(ShShaderType type);

    
    
    
    
    
    
    int getVersion() { return mVersion; }

    virtual void visitSymbol(TIntermSymbol*);
    virtual void visitConstantUnion(TIntermConstantUnion*);
    virtual bool visitBinary(Visit, TIntermBinary*);
    virtual bool visitUnary(Visit, TIntermUnary*);
    virtual bool visitSelection(Visit, TIntermSelection*);
    virtual bool visitAggregate(Visit, TIntermAggregate*);
    virtual bool visitLoop(Visit, TIntermLoop*);
    virtual bool visitBranch(Visit, TIntermBranch*);

protected:
    void updateVersion(int version);

private:
    ShShaderType mShaderType;
    int mVersion;
};

#endif  
