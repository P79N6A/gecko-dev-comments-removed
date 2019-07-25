










































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

  



  void* StorageFor(nsCSSProperty aProperty);
};

#endif
