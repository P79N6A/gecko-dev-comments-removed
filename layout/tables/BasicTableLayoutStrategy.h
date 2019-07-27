










#ifndef BasicTableLayoutStrategy_h_
#define BasicTableLayoutStrategy_h_

#include "mozilla/Attributes.h"
#include "nsITableLayoutStrategy.h"

class nsTableFrame;

class BasicTableLayoutStrategy : public nsITableLayoutStrategy
{
public:
    explicit BasicTableLayoutStrategy(nsTableFrame *aTableFrame);
    virtual ~BasicTableLayoutStrategy();

    
    virtual nscoord GetMinISize(nsRenderingContext* aRenderingContext) override;
    virtual nscoord GetPrefISize(nsRenderingContext* aRenderingContext,
                                 bool aComputingSize) override;
    virtual void MarkIntrinsicISizesDirty() override;
    virtual void ComputeColumnISizes(const nsHTMLReflowState& aReflowState) override;

private:
    
    
    enum BtlsWidthType { BTLS_MIN_WIDTH,
                         BTLS_PREF_WIDTH,
                         BTLS_FINAL_WIDTH };

    
    void ComputeColumnIntrinsicISizes(nsRenderingContext* aRenderingContext);

    
    void DistributePctWidthToColumns(float aSpanPrefPct,
                                     int32_t aFirstCol,
                                     int32_t aColCount);

    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    void DistributeWidthToColumns(nscoord aWidth,
                                  int32_t aFirstCol,
                                  int32_t aColCount,
                                  BtlsWidthType aWidthType,
                                  bool aSpanHasSpecifiedWidth);


    
    
    void ComputeIntrinsicISizes(nsRenderingContext* aRenderingContext);

    nsTableFrame *mTableFrame;
    nscoord mMinWidth;
    nscoord mPrefWidth;
    nscoord mPrefWidthPctExpand;
    nscoord mLastCalcWidth;
};

#endif
