






#ifndef nsMediaFeatures_h_
#define nsMediaFeatures_h_

#include "nsCSSProps.h"

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
        eFloat,      
        eBoolInteger,
        eIntRatio,   
        eResolution, 
                     
                     
        eEnumerated, 
        eIdent       
        
        
        
        
    };
    ValueType mValueType;

    union {
      
      
      
      const void* mInitializer_;
      
      
      const nsCSSProps::KTableValue* mKeywordTable;
      
      
      nsIAtom * const * mMetric;
    } mData;

    
    
    
    nsMediaFeatureValueGetter mGetter;
};

class nsMediaFeatures {
public:
    
    static const nsMediaFeature features[];
};

#endif 
