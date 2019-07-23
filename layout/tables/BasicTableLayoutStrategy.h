










































#ifndef BasicTableLayoutStrategy_h_
#define BasicTableLayoutStrategy_h_

#include "nsITableLayoutStrategy.h"

class nsTableFrame;

class BasicTableLayoutStrategy : public nsITableLayoutStrategy
{
public:
    BasicTableLayoutStrategy(nsTableFrame *aTableFrame);
    virtual ~BasicTableLayoutStrategy();

    
    virtual nscoord GetMinWidth(nsIRenderingContext* aRenderingContext);
    virtual nscoord GetPrefWidth(nsIRenderingContext* aRenderingContext,
                                 PRBool aComputingSize);
    virtual void MarkIntrinsicWidthsDirty();
    virtual void ComputeColumnWidths(const nsHTMLReflowState& aReflowState);

private:
    
    void ComputeColumnIntrinsicWidths(nsIRenderingContext* aRenderingContext);

    
    
    void ComputeIntrinsicWidths(nsIRenderingContext* aRenderingContext);

    nsTableFrame *mTableFrame;
    nscoord mMinWidth;
    nscoord mPrefWidth;
    nscoord mPrefWidthPctExpand;
    nscoord mLastCalcWidth;
};

#endif 
