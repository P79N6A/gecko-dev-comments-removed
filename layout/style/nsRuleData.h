










































#ifndef nsRuleData_h_
#define nsRuleData_h_

#include "nsCSSStruct.h"
#include "nsRuleNode.h"

struct nsRuleData
{
  nsStyleStructID mSID;
  PRPackedBool mCanStoreInRuleTree;
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

#ifdef MOZ_SVG
  nsRuleDataSVG* mSVGData;
#endif

  nsRuleDataColumn* mColumnData;

  nsRuleData(const nsStyleStructID& aSID, nsPresContext* aContext, nsStyleContext* aStyleContext) 
    :mSID(aSID), mPresContext(aContext), mStyleContext(aStyleContext), mPostResolveCallback(nsnull),
     mFontData(nsnull), mDisplayData(nsnull), mMarginData(nsnull), mListData(nsnull), 
     mPositionData(nsnull), mTableData(nsnull), mColorData(nsnull), mContentData(nsnull), mTextData(nsnull),
     mUserInterfaceData(nsnull), mColumnData(nsnull)
  {
    mCanStoreInRuleTree = PR_TRUE;
    mXULData = nsnull;
#ifdef MOZ_SVG
    mSVGData = nsnull;
#endif
  }
  ~nsRuleData() {}
};

#endif
