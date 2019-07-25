










































#ifndef mozilla_css_Declaration_h
#define mozilla_css_Declaration_h



#ifndef _IMPL_NS_LAYOUT
#error "This file should only be included within the layout library"
#endif

#include "nsCSSDataBlock.h"
#include "nsCSSProperty.h"
#include "nsCSSProps.h"
#include "nsStringFwd.h"
#include "nsTArray.h"
#include <stdio.h>

namespace mozilla {
namespace css {









class Declaration {
public:
  




  Declaration();

  Declaration(const Declaration& aCopy);

  ~Declaration();

  




  void ValueAppended(nsCSSProperty aProperty);

  void RemoveProperty(nsCSSProperty aProperty);

  bool HasProperty(nsCSSProperty aProperty) const;

  void GetValue(nsCSSProperty aProperty, nsAString& aValue) const;

  bool HasImportantData() const { return mImportantData != nsnull; }
  bool GetValueIsImportant(nsCSSProperty aProperty) const;
  bool GetValueIsImportant(const nsAString& aProperty) const;

  PRUint32 Count() const {
    return mOrder.Length();
  }
  void GetNthProperty(PRUint32 aIndex, nsAString& aReturn) const;

  void ToString(nsAString& aString) const;

  nsCSSCompressedDataBlock* GetNormalBlock() const { return mData; }
  nsCSSCompressedDataBlock* GetImportantBlock() const { return mImportantData; }

  


  void InitializeEmpty();

  



  void CompressFrom(nsCSSExpandedDataBlock *aExpandedData) {
    NS_ABORT_IF_FALSE(!mData, "oops");
    NS_ABORT_IF_FALSE(!mImportantData, "oops");
    aExpandedData->Compress(getter_Transfers(mData),
                            getter_Transfers(mImportantData));
    aExpandedData->AssertInitialState();
  }

  






  void ExpandTo(nsCSSExpandedDataBlock *aExpandedData) {
    AssertMutable();
    aExpandedData->AssertInitialState();

    NS_ABORT_IF_FALSE(mData, "oops");
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

  









  bool TryReplaceValue(nsCSSProperty aProperty, bool aIsImportant,
                         nsCSSExpandedDataBlock& aFromBlock,
                         bool* aChanged)
  {
    AssertMutable();
    NS_ABORT_IF_FALSE(mData, "called while expanded");

    if (nsCSSProps::IsShorthand(aProperty)) {
      *aChanged = PR_FALSE;
      return PR_FALSE;
    }
    nsCSSCompressedDataBlock *block = aIsImportant ? mImportantData : mData;
    
    if (!block) {
      *aChanged = PR_FALSE;
      return PR_FALSE;
    }

#ifdef DEBUG
    {
      nsCSSCompressedDataBlock *other = aIsImportant ? mData : mImportantData;
      NS_ABORT_IF_FALSE(!other || !other->ValueFor(aProperty) ||
                        !block->ValueFor(aProperty),
                        "Property both important and not?");
    }
#endif
    return block->TryReplaceValue(aProperty, aFromBlock, aChanged);
  }

  bool HasNonImportantValueFor(nsCSSProperty aProperty) const {
    NS_ABORT_IF_FALSE(!nsCSSProps::IsShorthand(aProperty), "must be longhand");
    return !!mData->ValueFor(aProperty);
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
  bool operator==(const Declaration& aCopy) const;

  static void AppendImportanceToString(bool aIsImportant, nsAString& aString);
  
  bool AppendValueToString(nsCSSProperty aProperty, nsAString& aResult) const;
  
  void AppendPropertyAndValueToString(nsCSSProperty aProperty,
                                      nsAutoString& aValue,
                                      nsAString& aResult) const;

public:
  nsCSSProperty OrderValueAt(PRUint32 aValue) const {
    return nsCSSProperty(mOrder.ElementAt(aValue));
  }

private:
  nsAutoTArray<PRUint8, 8> mOrder;

  
  
  nsAutoPtr<nsCSSCompressedDataBlock> mData;

  
  nsAutoPtr<nsCSSCompressedDataBlock> mImportantData;

  
  
  mutable bool mImmutable;
};

} 
} 

#endif 
