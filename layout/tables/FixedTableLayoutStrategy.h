










































#ifndef FixedTableLayoutStrategy_h_
#define FixedTableLayoutStrategy_h_

#include "nsITableLayoutStrategy.h"

class nsTableFrame;

class FixedTableLayoutStrategy : public nsITableLayoutStrategy
{
public:
    FixedTableLayoutStrategy(nsTableFrame *aTableFrame);
    virtual ~FixedTableLayoutStrategy();

    
    virtual nscoord GetMinWidth(nsRenderingContext* aRenderingContext);
    virtual nscoord GetPrefWidth(nsRenderingContext* aRenderingContext,
                                 PRBool aComputingSize);
    virtual void MarkIntrinsicWidthsDirty();
    virtual void ComputeColumnWidths(const nsHTMLReflowState& aReflowState);

private:
    nsTableFrame *mTableFrame;
    nscoord mMinWidth;
    nscoord mLastCalcWidth;
};

#endif 
