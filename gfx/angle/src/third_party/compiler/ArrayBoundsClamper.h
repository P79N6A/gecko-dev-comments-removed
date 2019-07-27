
























#ifndef THIRD_PARTY_COMPILER_ARRAY_BOUNDS_CLAMPER_H_
#define THIRD_PARTY_COMPILER_ARRAY_BOUNDS_CLAMPER_H_

#include "compiler/translator/InfoSink.h"
#include "compiler/translator/IntermNode.h"

class ArrayBoundsClamper {
public:
    ArrayBoundsClamper();

    
    
    void SetClampingStrategy(ShArrayIndexClampingStrategy clampingStrategy);

    
    
    void MarkIndirectArrayBoundsForClamping(TIntermNode* root);

    
    void OutputClampingFunctionDefinition(TInfoSinkBase& out) const;

    void Cleanup()
    {
        mArrayBoundsClampDefinitionNeeded = false;
    }

private:
    bool GetArrayBoundsClampDefinitionNeeded() const { return mArrayBoundsClampDefinitionNeeded; }
    void SetArrayBoundsClampDefinitionNeeded() { mArrayBoundsClampDefinitionNeeded = true; }

    ShArrayIndexClampingStrategy mClampingStrategy;
    bool mArrayBoundsClampDefinitionNeeded;
};

#endif 
