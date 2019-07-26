










#ifndef BasicTableLayoutStrategy_h_
#define BasicTableLayoutStrategy_h_

#include "mozilla/Attributes.h"
#include "nsITableLayoutStrategy.h"

class nsTableFrame;

class BasicTableLayoutStrategy : public nsITableLayoutStrategy
{
public:
    BasicTableLayoutStrategy(nsTableFrame *aTableFrame);
    virtual ~BasicTableLayoutStrategy();

    
    virtual nscoord GetMinWidth(nsRenderingContext* aRenderingContext) MOZ_OVERRIDE;
    virtual nscoord GetPrefWidth(nsRenderingContext* aRenderingContext,
                                 bool aComputingSize) MOZ_OVERRIDE;
    virtual void MarkIntrinsicWidthsDirty() MOZ_OVERRIDE;
    virtual void ComputeColumnWidths(const nsHTMLReflowState& aReflowState) MOZ_OVERRIDE;

private:
    
    
    enum BtlsWidthType { BTLS_MIN_WIDTH, 
                         BTLS_PREF_WIDTH, 
                         BTLS_FINAL_WIDTH };

    
    void ComputeColumnIntrinsicWidths(nsRenderingContext* aRenderingContext);

    
    void DistributePctWidthToColumns(float aSpanPrefPct,
                                     int32_t aFirstCol,
                                     int32_t aColCount);

    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    void DistributeWidthToColumns(nscoord aWidth, 
                                  int32_t aFirstCol, 
                                  int32_t aColCount,
                                  BtlsWidthType aWidthType,
                                  bool aSpanHasSpecifiedWidth);
 

    
    
    void ComputeIntrinsicWidths(nsRenderingContext* aRenderingContext);

    nsTableFrame *mTableFrame;
    nscoord mMinWidth;
    nscoord mPrefWidth;
    nscoord mPrefWidthPctExpand;
    nscoord mLastCalcWidth;
};

#endif
