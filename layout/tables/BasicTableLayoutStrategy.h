










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
    
    
    enum BtlsISizeType { BTLS_MIN_ISIZE,
                         BTLS_PREF_ISIZE,
                         BTLS_FINAL_ISIZE };

    
    void ComputeColumnIntrinsicISizes(nsRenderingContext* aRenderingContext);

    
    void DistributePctISizeToColumns(float aSpanPrefPct,
                                     int32_t aFirstCol,
                                     int32_t aColCount);

    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    void DistributeISizeToColumns(nscoord aISize,
                                  int32_t aFirstCol,
                                  int32_t aColCount,
                                  BtlsISizeType aISizeType,
                                  bool aSpanHasSpecifiedISize);


    
    
    void ComputeIntrinsicISizes(nsRenderingContext* aRenderingContext);

    nsTableFrame *mTableFrame;
    nscoord mMinISize;
    nscoord mPrefISize;
    nscoord mPrefISizePctExpand;
    nscoord mLastCalcISize;
};

#endif
