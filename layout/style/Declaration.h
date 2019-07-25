










































#ifndef mozilla_css_Declaration_h
#define mozilla_css_Declaration_h



#ifndef _IMPL_NS_LAYOUT
#error "This file should only be included within the layout library"
#endif

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

namespace mozilla {
namespace css {









class Declaration {
public:
  




  Declaration();

  Declaration(const Declaration& aCopy);

  ~Declaration();

  




  void ValueAppended(nsCSSProperty aProperty);

  void RemoveProperty(nsCSSProperty aProperty);

  void GetValue(nsCSSProperty aProperty, nsAString& aValue) const;

  PRBool HasImportantData() const { return mImportantData != nsnull; }
  PRBool GetValueIsImportant(nsCSSProperty aProperty) const;
  PRBool GetValueIsImportant(const nsAString& aProperty) const;

  PRUint32 Count() const {
    return mOrder.Length();
  }
  void GetNthProperty(PRUint32 aIndex, nsAString& aReturn) const;

  void ToString(nsAString& aString) const;

  nsCSSCompressedDataBlock* GetNormalBlock() const { return mData; }
  nsCSSCompressedDataBlock* GetImportantBlock() const { return mImportantData; }

  


  void InitializeEmpty();

  



  void CompressFrom(nsCSSExpandedDataBlock *aExpandedData) {
    NS_ASSERTION(!mData, "oops");
    NS_ASSERTION(!mImportantData, "oops");
    aExpandedData->Compress(getter_Transfers(mData),
                            getter_Transfers(mImportantData));
    aExpandedData->AssertInitialState();
  }

  






  void ExpandTo(nsCSSExpandedDataBlock *aExpandedData) {
    AssertMutable();
    aExpandedData->AssertInitialState();

    NS_ASSERTION(mData, "oops");
    aExpandedData->Expand(mData.forget(), mImportantData.forget());
  }

  



  void MapNormalRuleInfoInto(nsRuleData *aRuleData) const {
    NS_ABORT_IF_FALSE(mData, "called while expanded");
    mData->MapRuleInfoInto(aRuleData);
  }
  void MapImportantRuleInfoInto(nsRuleData *aRuleData) const {
    NS_ABORT_IF_FALSE(mData, "called while expanded");
    NS_ABORT_IF_FALSE(mImportantData, "must have important data");
    mImportantData->MapRuleInfoInto(aRuleData);
  }

  







  void* SlotForValue(nsCSSProperty aProperty, PRBool aIsImportant) {
    AssertMutable();
    NS_ABORT_IF_FALSE(mData, "called while expanded");

    if (nsCSSProps::IsShorthand(aProperty)) {
      return nsnull;
    }
    nsCSSCompressedDataBlock *block = aIsImportant ? mImportantData : mData;
    
    if (!block) {
      return nsnull;
    }

    void *slot = block->SlotForValue(aProperty);
#ifdef DEBUG
    {
      nsCSSCompressedDataBlock *other = aIsImportant ? mData : mImportantData;
      NS_ABORT_IF_FALSE(!slot || !other || !other->StorageFor(aProperty),
                        "Property both important and not?");
    }
#endif
    return slot;
  }

  PRBool HasNonImportantValueFor(nsCSSProperty aProperty) const {
    NS_ABORT_IF_FALSE(!nsCSSProps::IsShorthand(aProperty), "must be longhand");
    return !!mData->StorageFor(aProperty);
  }

  


  bool IsMutable() const {
    return !mImmutable;
  }

  


  Declaration* EnsureMutable();

  


  void AssertMutable() const {
    NS_ABORT_IF_FALSE(IsMutable(), "someone forgot to call EnsureMutable");
  }

  



  void SetImmutable() const { mImmutable = PR_TRUE; }

  



  void ClearData() {
    AssertMutable();
    mData = nsnull;
    mImportantData = nsnull;
    mOrder.Clear();
  }

#ifdef DEBUG
  void List(FILE* out = stdout, PRInt32 aIndent = 0) const;
#endif

private:
  
  Declaration& operator=(const Declaration& aCopy);
  PRBool operator==(const Declaration& aCopy) const;

  static void AppendImportanceToString(PRBool aIsImportant, nsAString& aString);
  
  PRBool AppendValueToString(nsCSSProperty aProperty, nsAString& aResult) const;
  
  void AppendPropertyAndValueToString(nsCSSProperty aProperty,
                                      nsAutoString& aValue,
                                      nsAString& aResult) const;

  nsCSSProperty OrderValueAt(PRUint32 aValue) const {
    return nsCSSProperty(mOrder.ElementAt(aValue));
  }

  nsAutoTArray<PRUint8, 8> mOrder;

  
  
  nsAutoPtr<nsCSSCompressedDataBlock> mData;

  
  nsAutoPtr<nsCSSCompressedDataBlock> mImportantData;

  
  
  mutable PRPackedBool mImmutable;
};

} 
} 

#endif 
