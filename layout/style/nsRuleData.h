










































#ifndef nsRuleData_h_
#define nsRuleData_h_

#include "nsCSSProps.h"
#include "nsStyleStructFwd.h"

class nsPresContext;
class nsStyleContext;
struct nsRuleData;

typedef void (*nsPostResolveFunc)(void* aStyleStruct, nsRuleData* aData);

struct nsRuleData
{
  const PRUint32 mSIDs;
  bool mCanStoreInRuleTree;
  bool mIsImportantRule;
  PRUint8 mLevel; 
  nsPresContext* const mPresContext;
  nsStyleContext* const mStyleContext;
  const nsPostResolveFunc mPostResolveCallback;

  
  
  
  
  
  
  
  
  
  
  
  nsCSSValue* const mValueStorage; 
  size_t mValueOffsets[nsStyleStructID_Length];

  nsRuleData(PRUint32 aSIDs, nsCSSValue* aValueStorage,
             nsPresContext* aContext, nsStyleContext* aStyleContext);

#ifdef DEBUG
  ~nsRuleData();
#else
  ~nsRuleData() {}
#endif

  






  nsCSSValue* ValueFor(nsCSSProperty aProperty)
  {
    NS_ABORT_IF_FALSE(aProperty < eCSSProperty_COUNT_no_shorthands,
                      "invalid or shorthand property");

    nsStyleStructID sid = nsCSSProps::kSIDTable[aProperty];
    size_t indexInStruct = nsCSSProps::PropertyIndexInStruct(aProperty);

    
    
    NS_ABORT_IF_FALSE(mSIDs & (1 << sid),
                      "calling nsRuleData::ValueFor on property not in mSIDs");
    NS_ABORT_IF_FALSE(sid != eStyleStruct_BackendOnly &&
                      indexInStruct != size_t(-1),
                      "backend-only property");

    return mValueStorage + mValueOffsets[sid] + indexInStruct;
  }

  const nsCSSValue* ValueFor(nsCSSProperty aProperty) const {
    return const_cast<nsRuleData*>(this)->ValueFor(aProperty);
  }

  








  #define CSS_PROP_DOMPROP_PREFIXED(prop_) prop_
  #define CSS_PROP(name_, id_, method_, flags_, parsevariant_, kwtable_,     \
                   stylestruct_, stylestructoffset_, animtype_)              \
    nsCSSValue* ValueFor##method_() {                                        \
      NS_ABORT_IF_FALSE(mSIDs & NS_STYLE_INHERIT_BIT(stylestruct_),          \
                        "Calling nsRuleData::ValueFor" #method_ " without "  \
                        "NS_STYLE_INHERIT_BIT(" #stylestruct_ " in mSIDs."); \
      nsStyleStructID sid = eStyleStruct_##stylestruct_;                     \
      size_t indexInStruct =                                                 \
        nsCSSProps::PropertyIndexInStruct(eCSSProperty_##id_);               \
      NS_ABORT_IF_FALSE(sid != eStyleStruct_BackendOnly &&                   \
                        indexInStruct != size_t(-1),                         \
                        "backend-only property");                            \
      return mValueStorage + mValueOffsets[sid] + indexInStruct;             \
    }                                                                        \
    const nsCSSValue* ValueFor##method_() const {                            \
      return const_cast<nsRuleData*>(this)->ValueFor##method_();             \
    }
  #define CSS_PROP_BACKENDONLY(name_, id_, method_, flags_, parsevariant_,   \
                               kwtable_)                                     \
    /* empty; backend-only structs are not in nsRuleData  */
  #include "nsCSSPropList.h"
  #undef CSS_PROP
  #undef CSS_PROP_DOMPROP_PREFIXED
  #undef CSS_PROP_BACKENDONLY

private:
  inline size_t GetPoisonOffset();

};

#endif
