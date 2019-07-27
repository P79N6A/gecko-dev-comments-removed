





#include "compiler/translator/timing/RestrictVertexShaderTiming.h"

void RestrictVertexShaderTiming::visitSymbol(TIntermSymbol* node)
{
    if (IsSampler(node->getBasicType())) {
        ++mNumErrors;
        mSink.message(EPrefixError,
                      node->getLine(),
                      "Samplers are not permitted in vertex shaders.\n");
    }
}
