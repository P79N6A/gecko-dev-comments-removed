






































#ifndef nsMediaFeatures_h_
#define nsMediaFeatures_h_

#include "nscore.h"

class nsIAtom;
class nsPresContext;
class nsCSSValue;

typedef nsresult
(* PR_CALLBACK nsMediaFeatureValueGetter)(nsPresContext* aPresContext,
                                          nsCSSValue& aResult);

struct nsMediaFeature {
    nsIAtom **mName; 

    enum RangeType { eMinMaxAllowed, eMinMaxNotAllowed };
    RangeType mRangeType;

    enum ValueType {
        
        
        eLength,     
        eInteger,    
        eIntRatio,   
        eResolution, 
                     
        eEnumerated  

        
        
        
        
    };
    ValueType mValueType;

    
    const PRInt32* mKeywordTable;

    
    
    
    nsMediaFeatureValueGetter mGetter;
};

class nsMediaFeatures {
public:
    
    static const nsMediaFeature features[];
};

#endif 
