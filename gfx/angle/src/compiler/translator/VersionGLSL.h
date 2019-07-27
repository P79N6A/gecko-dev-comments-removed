





#ifndef COMPILER_TRANSLATOR_VERSIONGLSL_H_
#define COMPILER_TRANSLATOR_VERSIONGLSL_H_

#include "compiler/translator/IntermNode.h"

#include "compiler/translator/Pragma.h"
















class TVersionGLSL : public TIntermTraverser
{
  public:
    TVersionGLSL(sh::GLenum type, const TPragma &pragma);

    
    
    
    
    
    
    int getVersion() { return mVersion; }

    virtual void visitSymbol(TIntermSymbol *);
    virtual bool visitAggregate(Visit, TIntermAggregate *);

  protected:
    void updateVersion(int version);

  private:
    int mVersion;
};

#endif  
