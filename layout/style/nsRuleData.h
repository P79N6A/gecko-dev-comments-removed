










































#ifndef nsRuleData_h_
#define nsRuleData_h_

#include "nsCSSStruct.h"
#include "nsStyleStructFwd.h"
#include "nsTArray.h"
class nsPresContext;
class nsStyleContext;

struct nsRuleData;
typedef void (*nsPostResolveFunc)(void* aStyleStruct, nsRuleData* aData,
                                  nsIStyleRule* aRule);

struct nsPostResolveCallback {
  nsPostResolveFunc mFunc;
  nsIStyleRule *mRule;
};

struct nsRuleData
{
  PRUint32 mSIDs;
  PRPackedBool mCanStoreInRuleTree;
  PRPackedBool mIsImportantRule;
  PRUint8 mLevel; 
  nsPresContext* mPresContext;
  nsStyleContext* mStyleContext;
  
  
  nsTArray<nsPostResolveCallback> mPostResolveCallbacks;
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

  nsRuleData(PRUint32 aSIDs, nsPresContext* aContext, nsStyleContext* aStyleContext) 
    :mSIDs(aSIDs), mPresContext(aContext), mStyleContext(aStyleContext),
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
