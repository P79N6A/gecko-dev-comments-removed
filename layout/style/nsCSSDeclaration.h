










































#ifndef nsCSSDeclaration_h___
#define nsCSSDeclaration_h___

#include "nsISupports.h"
#include "nsColor.h"
#include <stdio.h>
#include "nsString.h"
#include "nsCoord.h"
#include "nsCSSValue.h"
#include "nsCSSProps.h"
#include "nsValueArray.h"
#include "nsCSSDataBlock.h"
#include "nsCSSStruct.h"

class nsCSSDeclaration {
public:
  




  nsCSSDeclaration();

  nsCSSDeclaration(const nsCSSDeclaration& aCopy);

  




  nsresult ValueAppended(nsCSSProperty aProperty);

  nsresult AppendComment(const nsAString& aComment);
  nsresult RemoveProperty(nsCSSProperty aProperty);

  nsresult GetValue(nsCSSProperty aProperty, nsAString& aValue) const;
  nsresult GetValue(const nsAString& aProperty, nsAString& aValue) const;

  PRBool HasImportantData() const { return mImportantData != nsnull; }
  PRBool GetValueIsImportant(nsCSSProperty aProperty) const;
  PRBool GetValueIsImportant(const nsAString& aProperty) const;

  PRUint32 Count() const;
  nsresult GetNthProperty(PRUint32 aIndex, nsAString& aReturn) const;

  nsresult ToString(nsAString& aString) const;

  nsCSSDeclaration* Clone() const;

  nsresult MapRuleInfoInto(nsRuleData *aRuleData) const {
    return mData->MapRuleInfoInto(aRuleData);
  }

  nsresult MapImportantRuleInfoInto(nsRuleData *aRuleData) const {
    return mImportantData->MapRuleInfoInto(aRuleData);
  }

  



  PRBool InitializeEmpty();

  



  void CompressFrom(nsCSSExpandedDataBlock *aExpandedData) {
    NS_ASSERTION(!mData, "oops");
    NS_ASSERTION(!mImportantData, "oops");
    aExpandedData->Compress(&mData, &mImportantData);
  }

  






  void ExpandTo(nsCSSExpandedDataBlock *aExpandedData) {
    aExpandedData->AssertInitialState();

    NS_ASSERTION(mData, "oops");
    aExpandedData->Expand(&mData, &mImportantData);
    NS_ASSERTION(!mData && !mImportantData,
                 "Expand didn't null things out");
  }

  



  void ClearData() {
    mData->Destroy();
    mData = nsnull;
    if (mImportantData) {
      mImportantData->Destroy();
      mImportantData = nsnull;
    }
    mOrder.Clear();
  }

#ifdef DEBUG
  void List(FILE* out = stdout, PRInt32 aIndent = 0) const;
#endif
  
private:
  
  nsCSSDeclaration& operator=(const nsCSSDeclaration& aCopy);
  PRBool operator==(const nsCSSDeclaration& aCopy) const;

  static void AppendImportanceToString(PRBool aIsImportant, nsAString& aString);
  
  PRBool   AppendValueToString(nsCSSProperty aProperty, nsAString& aResult) const;
  
  static PRBool AppendCSSValueToString(nsCSSProperty aProperty,
                                       const nsCSSValue& aValue,
                                       nsAString& aResult);

  
  nsresult GetValueOrImportantValue(nsCSSProperty aProperty, nsCSSValue& aValue) const;

  void   PropertyIsSet(PRInt32 & aPropertyIndex, PRInt32 aIndex, PRUint32 & aSet, PRUint32 aValue) const;
  PRBool TryBorderShorthand(nsAString & aString, PRUint32 aPropertiesSet,
                            PRInt32 aBorderTopWidth,
                            PRInt32 aBorderTopStyle,
                            PRInt32 aBorderTopColor,
                            PRInt32 aBorderBottomWidth,
                            PRInt32 aBorderBottomStyle,
                            PRInt32 aBorderBottomColor,
                            PRInt32 aBorderLeftWidth,
                            PRInt32 aBorderLeftStyle,
                            PRInt32 aBorderLeftColor,
                            PRInt32 aBorderRightWidth,
                            PRInt32 aBorderRightStyle,
                            PRInt32 aBorderRightColor) const;
  PRBool  TryBorderSideShorthand(nsAString & aString,
                                 nsCSSProperty  aShorthand,
                                 PRInt32 aBorderWidth,
                                 PRInt32 aBorderStyle,
                                 PRInt32 aBorderColor) const;
  PRBool  TryFourSidesShorthand(nsAString & aString,
                                nsCSSProperty aShorthand,
                                PRInt32 & aTop,
                                PRInt32 & aBottom,
                                PRInt32 & aLeft,
                                PRInt32 & aRight,
                                PRBool aClearIndexes) const;
  void  TryBackgroundShorthand(nsAString & aString,
                               PRInt32 & aBgColor, PRInt32 & aBgImage,
                               PRInt32 & aBgRepeat, PRInt32 & aBgAttachment,
                               PRInt32 & aBgPositionX,
                               PRInt32 & aBgPositionY) const;
  void  UseBackgroundPosition(nsAString & aString,
                              PRInt32 & aBgPositionX,
                              PRInt32 & aBgPositionY) const;
  void  TryOverflowShorthand(nsAString & aString,
                             PRInt32 & aOverflowX, PRInt32 & aOverflowY) const;
#ifdef MOZ_SVG
  void  TryMarkerShorthand(nsAString & aString,
                           PRInt32 & aMarkerEnd,
                           PRInt32 & aMarkerMid,
                           PRInt32 & aMarkerStart) const;
#endif

  PRBool   AllPropertiesSameImportance(PRInt32 aFirst, PRInt32 aSecond,
                                       PRInt32 aThird, PRInt32 aFourth,
                                       PRInt32 aFifth, PRInt32 aSixth,
                                       PRBool & aImportance) const;
  PRBool   AllPropertiesSameValue(PRInt32 aFirst, PRInt32 aSecond,
                                  PRInt32 aThird, PRInt32 aFourth) const;
  void     AppendPropertyAndValueToString(nsCSSProperty aProperty,
                                          nsAString& aResult) const
  {
    AppendPropertyAndValueToString(aProperty, aProperty, aResult);
  }
  void     AppendPropertyAndValueToString(nsCSSProperty aProperty,
                                          nsCSSProperty aPropertyName,
                                          nsAString& aResult) const;

private:
    
    
    
    
    
    
    
    
    
    friend class CSSStyleRuleImpl;
    void AddRef(void) {
      ++mRefCnt;
    }
    void Release(void) {
      NS_ASSERTION(0 < mRefCnt, "bad Release");
      if (0 == --mRefCnt) {
        delete this;
      }
    }
public:
    void RuleAbort(void) {
      NS_ASSERTION(0 == mRefCnt, "bad RuleAbort");
      delete this;
    }
private:
  
  ~nsCSSDeclaration(void);
    
  nsCSSProperty OrderValueAt(nsValueArrayIndex aValue) const {
    return nsCSSProperty(mOrder.ValueAt(aValue));
  }

private:
    nsValueArray mOrder;
    nsAutoRefCnt mRefCnt;
    nsCSSCompressedDataBlock *mData; 
    nsCSSCompressedDataBlock *mImportantData; 
};

#endif 
