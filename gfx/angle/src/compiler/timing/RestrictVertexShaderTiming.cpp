





#include "compiler/timing/RestrictVertexShaderTiming.h"

void RestrictVertexShaderTiming::visitSymbol(TIntermSymbol* node)
{
    if (IsSampler(node->getBasicType())) {
        ++mNumErrors;
        mSink.prefix(EPrefixError);
        mSink.location(node->getLine());
        mSink << "Samplers are not permitted in vertex shaders.\n";
    }
}
