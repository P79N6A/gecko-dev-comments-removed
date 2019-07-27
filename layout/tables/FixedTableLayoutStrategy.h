










#ifndef FixedTableLayoutStrategy_h_
#define FixedTableLayoutStrategy_h_

#include "mozilla/Attributes.h"
#include "nsITableLayoutStrategy.h"

class nsTableFrame;

class FixedTableLayoutStrategy : public nsITableLayoutStrategy
{
public:
    explicit FixedTableLayoutStrategy(nsTableFrame *aTableFrame);
    virtual ~FixedTableLayoutStrategy();

    
    virtual nscoord GetMinISize(nsRenderingContext* aRenderingContext) override;
    virtual nscoord GetPrefISize(nsRenderingContext* aRenderingContext,
                                 bool aComputingSize) override;
    virtual void MarkIntrinsicISizesDirty() override;
    virtual void ComputeColumnISizes(const nsHTMLReflowState& aReflowState) override;

private:
    nsTableFrame *mTableFrame;
    nscoord mMinWidth;
    nscoord mLastCalcWidth;
};

#endif 
