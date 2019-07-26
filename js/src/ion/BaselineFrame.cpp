






#include "BaselineFrame.h"

using namespace js;
using namespace js::ion;

void
BaselineFrame::trace(JSTracer *trc)
{
    gc::MarkObjectRoot(trc, &scopeChain_, "baseline-scopechain");

    
    size_t nvalues = numValueSlots();
    if (nvalues > 0) {
        
        Value *last = valueSlot(nvalues - 1);
        gc::MarkValueRootRange(trc, nvalues, last, "baseline-stack");
    }
}
