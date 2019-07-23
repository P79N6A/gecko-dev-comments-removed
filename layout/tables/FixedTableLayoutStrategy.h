










































#ifndef FixedTableLayoutStrategy_h_
#define FixedTableLayoutStrategy_h_

#include "nsITableLayoutStrategy.h"

class nsTableFrame;

class FixedTableLayoutStrategy : public nsITableLayoutStrategy
{
public:
    FixedTableLayoutStrategy(nsTableFrame *aTableFrame);
    virtual ~FixedTableLayoutStrategy();

    
    virtual nscoord GetMinWidth(nsIRenderingContext* aRenderingContext);
    virtual nscoord GetPrefWidth(nsIRenderingContext* aRenderingContext,
                                 PRBool aComputingSize);
    virtual void MarkIntrinsicWidthsDirty();
    virtual void ComputeColumnWidths(const nsHTMLReflowState& aReflowState);

private:
    nsTableFrame *mTableFrame;
    nscoord mMinWidth;
    nscoord mLastCalcWidth;
};

#endif 
