










#ifndef FixedTableLayoutStrategy_h_
#define FixedTableLayoutStrategy_h_

#include "mozilla/Attributes.h"
#include "nsITableLayoutStrategy.h"

class nsTableFrame;

class FixedTableLayoutStrategy : public nsITableLayoutStrategy
{
public:
    FixedTableLayoutStrategy(nsTableFrame *aTableFrame);
    virtual ~FixedTableLayoutStrategy();

    
    virtual nscoord GetMinWidth(nsRenderingContext* aRenderingContext) MOZ_OVERRIDE;
    virtual nscoord GetPrefWidth(nsRenderingContext* aRenderingContext,
                                 bool aComputingSize) MOZ_OVERRIDE;
    virtual void MarkIntrinsicWidthsDirty() MOZ_OVERRIDE;
    virtual void ComputeColumnWidths(const nsHTMLReflowState& aReflowState) MOZ_OVERRIDE;

private:
    nsTableFrame *mTableFrame;
    nscoord mMinWidth;
    nscoord mLastCalcWidth;
};

#endif 
