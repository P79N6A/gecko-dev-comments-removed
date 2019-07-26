
























#include "compiler/ArrayBoundsClamper.h"

const char* kIntClampBegin = "// BEGIN: Generated code for array bounds clamping\n\n";
const char* kIntClampEnd = "// END: Generated code for array bounds clamping\n\n";
const char* kIntClampDefinition = "int webgl_int_clamp(int value, int minValue, int maxValue) { return ((value < minValue) ? minValue : ((value > maxValue) ? maxValue : value)); }\n\n";

namespace {

class ArrayBoundsClamperMarker : public TIntermTraverser {
public:
    ArrayBoundsClamperMarker()
        : mNeedsClamp(false)
   {
   }

   virtual bool visitBinary(Visit visit, TIntermBinary* node)
   {
       if (node->getOp() == EOpIndexIndirect)
       {
           TIntermTyped* left = node->getLeft();
           if (left->isArray() || left->isVector() || left->isMatrix())
           {
               node->setAddIndexClamp();
               mNeedsClamp = true;
           }
       }
       return true;
   }

    bool GetNeedsClamp() { return mNeedsClamp; }

private:
    bool mNeedsClamp;
};

}  

ArrayBoundsClamper::ArrayBoundsClamper()
    : mArrayBoundsClampDefinitionNeeded(false)
{
}

void ArrayBoundsClamper::OutputClampingFunctionDefinition(TInfoSinkBase& out) const
{
    if (!mArrayBoundsClampDefinitionNeeded)
    {
        return;
    }
    out << kIntClampBegin << kIntClampDefinition << kIntClampEnd;
}

void ArrayBoundsClamper::MarkIndirectArrayBoundsForClamping(TIntermNode* root)
{
    ASSERT(root);

    ArrayBoundsClamperMarker clamper;
    root->traverse(&clamper);
    if (clamper.GetNeedsClamp())
    {
        SetArrayBoundsClampDefinitionNeeded();
    }
}

