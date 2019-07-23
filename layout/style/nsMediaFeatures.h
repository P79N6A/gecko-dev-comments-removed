






































#ifndef nsMediaFeatures_h_
#define nsMediaFeatures_h_

#include "nscore.h"

class nsIAtom;
class nsPresContext;
class nsCSSValue;

typedef nsresult
(* nsMediaFeatureValueGetter)(nsPresContext* aPresContext, nsCSSValue& aResult);

struct nsMediaFeature {
    nsIAtom **mName; 

    enum RangeType { eMinMaxAllowed, eMinMaxNotAllowed };
    RangeType mRangeType;

    enum ValueType {
        
        
        eLength,     
        eInteger,    
        eBoolInteger,
        eIntRatio,   
        eResolution, 
                     
        eEnumerated  

        
        
        
        
    };
    ValueType mValueType;

    union {
      
      
      
      const void* mInitializer_;
      
      
      const PRInt32* mKeywordTable;
    } mData;

    
    
    
    nsMediaFeatureValueGetter mGetter;
};

class nsMediaFeatures {
public:
    
    static const nsMediaFeature features[];
};

#endif 
