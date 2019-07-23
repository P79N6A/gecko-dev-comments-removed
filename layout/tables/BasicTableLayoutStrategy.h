










































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
    
    
    enum BtlsWidthType { BTLS_MIN_WIDTH, 
                         BTLS_PREF_WIDTH, 
                         BTLS_FINAL_WIDTH };

    
    void ComputeColumnIntrinsicWidths(nsIRenderingContext* aRenderingContext);

    
    void DistributePctWidthToColumns(float aSpanPrefPct,
                                     PRInt32 aFirstCol,
                                     PRInt32 aColCount);

    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    void DistributeWidthToColumns(nscoord aWidth, 
                                  PRInt32 aFirstCol, 
                                  PRInt32 aColCount,
                                  BtlsWidthType aWidthType,
                                  PRBool aSpanHasSpecifiedWidth);
 

    
    
    void ComputeIntrinsicWidths(nsIRenderingContext* aRenderingContext);

    nsTableFrame *mTableFrame;
    nscoord mMinWidth;
    nscoord mPrefWidth;
    nscoord mPrefWidthPctExpand;
    nscoord mLastCalcWidth;
};

#endif
