










































#ifndef nsITableLayoutStrategy_h_
#define nsITableLayoutStrategy_h_

#include "nscore.h"
#include "nsCoord.h"

class nsRenderingContext;
struct nsHTMLReflowState;

class nsITableLayoutStrategy
{
public:
    virtual ~nsITableLayoutStrategy() {}

    
    virtual nscoord GetMinWidth(nsRenderingContext* aRenderingContext) = 0;

    
    virtual nscoord GetPrefWidth(nsRenderingContext* aRenderingContext,
                                 PRBool aComputingSize) = 0;

    
    virtual void MarkIntrinsicWidthsDirty() = 0;

    



    virtual void ComputeColumnWidths(const nsHTMLReflowState& aReflowState) = 0;

    



    enum Type { Auto, Fixed };
    Type GetType() const { return mType; }

protected:
    nsITableLayoutStrategy(Type aType) : mType(aType) {}
private:
    Type mType;
};

#endif 
