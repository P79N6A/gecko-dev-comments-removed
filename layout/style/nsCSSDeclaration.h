










































#ifndef nsCSSDeclaration_h___
#define nsCSSDeclaration_h___

#include "nsISupports.h"
#include "nsColor.h"
#include <stdio.h>
#include "nsString.h"
#include "nsCoord.h"
#include "nsCSSValue.h"
#include "nsCSSProps.h"
#include "nsTArray.h"
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

  PRBool HasImportantData() const { return mImportantData != nsnull; }
  PRBool GetValueIsImportant(nsCSSProperty aProperty) const;
  PRBool GetValueIsImportant(const nsAString& aProperty) const;

  PRUint32 Count() const {
    return mOrder.Length(); 
  }
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
    aExpandedData->Compress(getter_AddRefs(mData),
                            getter_AddRefs(mImportantData));
    aExpandedData->AssertInitialState();
  }

  






  void ExpandTo(nsCSSExpandedDataBlock *aExpandedData) {
    aExpandedData->AssertInitialState();

    NS_ASSERTION(mData, "oops");
    aExpandedData->Expand(&mData, &mImportantData);
    NS_ASSERTION(!mData && !mImportantData,
                 "Expand didn't null things out");
  }

  





  void* SlotForValue(nsCSSProperty aProperty) {
    NS_PRECONDITION(mData, "How did that happen?");
    if (nsCSSProps::IsShorthand(aProperty)) {
      return nsnull;
    }

    void* slot = mData->SlotForValue(aProperty);

    NS_ASSERTION(!slot || !mImportantData ||
                 !mImportantData->SlotForValue(aProperty),
                 "Property both important and not?");
    return slot;
  }

  



  void ClearData() {
    mData = nsnull;
    mImportantData = nsnull;
    mOrder.Clear();
  }

#ifdef DEBUG
  void List(FILE* out = stdout, PRInt32 aIndent = 0) const;
#endif
  
  
  static PRBool AppendCSSValueToString(nsCSSProperty aProperty,
                                       const nsCSSValue& aValue,
                                       nsAString& aResult);

  
  static PRBool AppendStorageToString(nsCSSProperty aProperty,
                                      const void* aStorage,
                                      nsAString& aResult);

private:
  
  nsCSSDeclaration& operator=(const nsCSSDeclaration& aCopy);
  PRBool operator==(const nsCSSDeclaration& aCopy) const;

  static void AppendImportanceToString(PRBool aIsImportant, nsAString& aString);
  
  PRBool   AppendValueToString(nsCSSProperty aProperty, nsAString& aResult) const;
  
  void     AppendPropertyAndValueToString(nsCSSProperty aProperty,
                                          nsAutoString& aValue,
                                          nsAString& aResult) const;

private:
    
    
    
    
    
    
    
    
    
    friend class CSSStyleRuleImpl;
    void AddRef(void) {
      if (mRefCnt == PR_UINT32_MAX) {
        NS_WARNING("refcount overflow, leaking object");
        return;
      }
      ++mRefCnt;
    }
    void Release(void) {
      if (mRefCnt == PR_UINT32_MAX) {
        NS_WARNING("refcount overflow, leaking object");
        return;
      }
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
    
  nsCSSProperty OrderValueAt(PRUint32 aValue) const {
    return nsCSSProperty(mOrder.ElementAt(aValue));
  }

private:
    nsAutoTArray<PRUint8, 8> mOrder;
    nsAutoRefCnt mRefCnt;

    
    nsRefPtr<nsCSSCompressedDataBlock> mData;

    
    nsRefPtr<nsCSSCompressedDataBlock> mImportantData;
};

#endif 
