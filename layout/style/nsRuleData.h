










































#ifndef nsRuleData_h_
#define nsRuleData_h_

#include "nsCSSProps.h"
#include "nsCSSStruct.h"
#include "nsStyleStructFwd.h"

class nsPresContext;
class nsStyleContext;
struct nsRuleData;

typedef void (*nsPostResolveFunc)(void* aStyleStruct, nsRuleData* aData);

struct nsRuleData
{
  PRUint32 mSIDs;
  PRPackedBool mCanStoreInRuleTree;
  PRPackedBool mIsImportantRule;
  PRUint8 mLevel; 
  nsPresContext* mPresContext;
  nsStyleContext* mStyleContext;
  nsPostResolveFunc mPostResolveCallback;

  
  nsRuleDataFont* mFontData;
  nsRuleDataDisplay* mDisplayData;
  nsRuleDataMargin* mMarginData;
  nsRuleDataList* mListData;
  nsRuleDataPosition* mPositionData;
  nsRuleDataTable* mTableData;
  nsRuleDataColor* mColorData;
  nsRuleDataContent* mContentData;
  nsRuleDataText* mTextData;
  nsRuleDataUserInterface* mUserInterfaceData;
  nsRuleDataXUL* mXULData;
  nsRuleDataSVG* mSVGData;
  nsRuleDataColumn* mColumnData;

  nsRuleData(PRUint32 aSIDs,
             nsPresContext* aContext,
             nsStyleContext* aStyleContext)
    : mSIDs(aSIDs),
      mCanStoreInRuleTree(PR_TRUE),
      mPresContext(aContext),
      mStyleContext(aStyleContext),
      mPostResolveCallback(nsnull),
      mFontData(nsnull),
      mDisplayData(nsnull),
      mMarginData(nsnull),
      mListData(nsnull),
      mPositionData(nsnull),
      mTableData(nsnull),
      mColorData(nsnull),
      mContentData(nsnull),
      mTextData(nsnull),
      mUserInterfaceData(nsnull),
      mXULData(nsnull),
      mSVGData(nsnull),
      mColumnData(nsnull)
  {}
  ~nsRuleData() {}

  






  nsCSSValue* ValueFor(nsCSSProperty aProperty);

  const nsCSSValue* ValueFor(nsCSSProperty aProperty) const {
    return const_cast<nsRuleData*>(this)->ValueFor(aProperty);
  }

  








  #define CSS_PROP_INCLUDE_NOT_CSS
  #define CSS_PROP_DOMPROP_PREFIXED(prop_) prop_
  #define CSS_PROP(name_, id_, method_, flags_, datastruct_, member_,        \
                   parsevariant_, kwtable_, stylestruct_, stylestructoffset_,\
                   animtype_)                                                \
    nsCSSValue* ValueFor##method_() {                                        \
      NS_ABORT_IF_FALSE(mSIDs & NS_STYLE_INHERIT_BIT(stylestruct_),          \
                        "Calling nsRuleData::ValueFor" #method_ " without "  \
                        "NS_STYLE_INHERIT_BIT(" #stylestruct_ " in mSIDs."); \
      nsRuleData##datastruct_ *cssstruct = m##datastruct_##Data;             \
      NS_ABORT_IF_FALSE(cssstruct, "nsRuleNode::Get" #stylestruct_ "Data "   \
                                   "set up nsRuleData incorrectly");         \
      return &cssstruct->member_;                                            \
    }                                                                        \
    const nsCSSValue* ValueFor##method_() const {                            \
      NS_ABORT_IF_FALSE(mSIDs & NS_STYLE_INHERIT_BIT(stylestruct_),          \
                        "Calling nsRuleData::ValueFor" #method_ " without "  \
                        "NS_STYLE_INHERIT_BIT(" #stylestruct_ " in mSIDs."); \
      const nsRuleData##datastruct_ *cssstruct = m##datastruct_##Data;       \
      NS_ABORT_IF_FALSE(cssstruct, "nsRuleNode::Get" #stylestruct_ "Data "   \
                                   "set up nsRuleData incorrectly");         \
      return &cssstruct->member_;                                            \
    }
  #define CSS_PROP_BACKENDONLY(name_, id_, method_, flags_, datastruct_,     \
                               member_, parsevariant_, kwtable_)             \
    /* empty; backend-only structs are not in nsRuleData  */
  #include "nsCSSPropList.h"
  #undef CSS_PROP_INCLUDE_NOT_CSS
  #undef CSS_PROP
  #undef CSS_PROP_DOMPROP_PREFIXED
  #undef CSS_PROP_BACKENDONLY

};

#endif
