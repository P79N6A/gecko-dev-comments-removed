
























#ifndef COMPILER_ARRAY_BOUNDS_CLAMPER_H_
#define COMPILER_ARRAY_BOUNDS_CLAMPER_H_

#include "GLSLANG/ShaderLang.h"

#include "compiler/InfoSink.h"
#include "compiler/intermediate.h"

class ArrayBoundsClamper {
public:
    ArrayBoundsClamper();

    
    void OutputClampingFunctionDefinition(TInfoSinkBase& out) const;

    
    
    void MarkIndirectArrayBoundsForClamping(TIntermNode* root);

    void Cleanup()
    {
        mArrayBoundsClampDefinitionNeeded = false;
    }

private:
    bool GetArrayBoundsClampDefinitionNeeded() const { return mArrayBoundsClampDefinitionNeeded; }
    void SetArrayBoundsClampDefinitionNeeded() { mArrayBoundsClampDefinitionNeeded = true; }
    
    bool mArrayBoundsClampDefinitionNeeded;
};

#endif 
