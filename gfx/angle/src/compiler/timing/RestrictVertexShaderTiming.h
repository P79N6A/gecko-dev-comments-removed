





#ifndef COMPILER_TIMING_RESTRICT_VERTEX_SHADER_TIMING_H_
#define COMPILER_TIMING_RESTRICT_VERTEX_SHADER_TIMING_H_

#include "GLSLANG/ShaderLang.h"

#include "compiler/intermediate.h"
#include "compiler/InfoSink.h"

class TInfoSinkBase;

class RestrictVertexShaderTiming : public TIntermTraverser {
public:
    RestrictVertexShaderTiming(TInfoSinkBase& sink)
        : TIntermTraverser(true, false, false)
        , mSink(sink)
        , mNumErrors(0) {}

    void enforceRestrictions(TIntermNode* root) { root->traverse(this); }
    int numErrors() { return mNumErrors; }

    virtual void visitSymbol(TIntermSymbol*);
private:
    TInfoSinkBase& mSink;
    int mNumErrors;
};

#endif  
