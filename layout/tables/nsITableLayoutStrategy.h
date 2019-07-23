










































#ifndef nsITableLayoutStrategy_h_
#define nsITableLayoutStrategy_h_

#include "nscore.h"
#include "nsCoord.h"

class nsIRenderingContext;
struct nsHTMLReflowState;

class nsITableLayoutStrategy
{
public:
    virtual ~nsITableLayoutStrategy() {};

    
    virtual nscoord GetMinWidth(nsIRenderingContext* aRenderingContext) = 0;

    
    virtual nscoord GetPrefWidth(nsIRenderingContext* aRenderingContext,
                                 PRBool aComputingSize) = 0;

    
    virtual void MarkIntrinsicWidthsDirty() = 0;

    



    virtual void ComputeColumnWidths(const nsHTMLReflowState& aReflowState) = 0;
};

#endif 
