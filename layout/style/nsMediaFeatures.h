






































#ifndef nsMediaFeatures_h_
#define nsMediaFeatures_h_

#include "nscore.h"

class nsIAtom;
class nsPresContext;
class nsCSSValue;

struct nsMediaFeature;
typedef nsresult
(* nsMediaFeatureValueGetter)(nsPresContext* aPresContext,
                              const nsMediaFeature* aFeature,
                              nsCSSValue& aResult);

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
      
      
      nsIAtom * const * mMetric;
    } mData;

    
    
    
    nsMediaFeatureValueGetter mGetter;
};

class nsMediaFeatures {
public:
    
    static const nsMediaFeature features[];
};

#endif 
