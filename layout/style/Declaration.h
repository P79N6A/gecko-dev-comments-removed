









#ifndef mozilla_css_Declaration_h
#define mozilla_css_Declaration_h



#ifndef MOZILLA_INTERNAL_API
#error "This file should only be included within libxul"
#endif

#include "mozilla/Attributes.h"
#include "mozilla/MemoryReporting.h"
#include "CSSVariableDeclarations.h"
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
  void GetAuthoredValue(nsCSSProperty aProperty, nsAString& aValue) const;

  bool HasImportantData() const {
    return mImportantData || mImportantVariables;
  }
  bool GetValueIsImportant(nsCSSProperty aProperty) const;
  bool GetValueIsImportant(const nsAString& aProperty) const;

  










  void AddVariableDeclaration(const nsAString& aName,
                              CSSVariableDeclarations::Type aType,
                              const nsString& aValue,
                              bool aIsImportant,
                              bool aOverrideImportant);

  




  void RemoveVariableDeclaration(const nsAString& aName);

  





  bool HasVariableDeclaration(const nsAString& aName) const;

  








  void GetVariableDeclaration(const nsAString& aName, nsAString& aValue) const;

  



  bool GetVariableValueIsImportant(const nsAString& aName) const;

  uint32_t Count() const {
    return mOrder.Length();
  }

  
  bool GetNthProperty(uint32_t aIndex, nsAString& aReturn) const;

  void ToString(nsAString& aString) const;

  nsCSSCompressedDataBlock* GetNormalBlock() const { return mData; }
  nsCSSCompressedDataBlock* GetImportantBlock() const { return mImportantData; }

  


  void InitializeEmpty();

  




  void CompressFrom(nsCSSExpandedDataBlock *aExpandedData) {
    NS_ABORT_IF_FALSE(!mData, "oops");
    NS_ABORT_IF_FALSE(!mImportantData, "oops");
    aExpandedData->Compress(getter_Transfers(mData),
                            getter_Transfers(mImportantData),
                            mOrder);
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
    if (mVariables) {
      mVariables->MapRuleInfoInto(aRuleData);
    }
  }
  void MapImportantRuleInfoInto(nsRuleData *aRuleData) const {
    NS_ABORT_IF_FALSE(mData, "called while expanded");
    NS_ABORT_IF_FALSE(mImportantData || mImportantVariables,
                      "must have important data or variables");
    if (mImportantData) {
      mImportantData->MapRuleInfoInto(aRuleData);
    }
    if (mImportantVariables) {
      mImportantVariables->MapRuleInfoInto(aRuleData);
    }
  }

  









  bool TryReplaceValue(nsCSSProperty aProperty, bool aIsImportant,
                         nsCSSExpandedDataBlock& aFromBlock,
                         bool* aChanged)
  {
    AssertMutable();
    NS_ABORT_IF_FALSE(mData, "called while expanded");

    if (nsCSSProps::IsShorthand(aProperty)) {
      *aChanged = false;
      return false;
    }
    nsCSSCompressedDataBlock *block = aIsImportant ? mImportantData : mData;
    
    if (!block) {
      *aChanged = false;
      return false;
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

  



  void SetImmutable() const { mImmutable = true; }

  



  void ClearData() {
    AssertMutable();
    mData = nullptr;
    mImportantData = nullptr;
    mVariables = nullptr;
    mImportantVariables = nullptr;
    mOrder.Clear();
    mVariableOrder.Clear();
  }

#ifdef DEBUG
  void List(FILE* out = stdout, int32_t aIndent = 0) const;
#endif

private:
  Declaration& operator=(const Declaration& aCopy) = delete;
  bool operator==(const Declaration& aCopy) const = delete;

  void GetValue(nsCSSProperty aProperty, nsAString& aValue,
                nsCSSValue::Serialization aValueSerialization) const;

  static void AppendImportanceToString(bool aIsImportant, nsAString& aString);
  
  bool AppendValueToString(nsCSSProperty aProperty, nsAString& aResult) const;
  bool AppendValueToString(nsCSSProperty aProperty, nsAString& aResult,
                           nsCSSValue::Serialization aValueSerialization) const;
  
  void AppendPropertyAndValueToString(nsCSSProperty aProperty,
                                      nsAutoString& aValue,
                                      nsAString& aResult) const;
  
  
  void AppendVariableAndValueToString(const nsAString& aName,
                                      nsAString& aResult) const;

public:
  




  nsCSSProperty GetPropertyAt(uint32_t aIndex) const {
    uint32_t value = mOrder[aIndex];
    if (value >= eCSSProperty_COUNT) {
      return eCSSPropertyExtra_variable;
    }
    return nsCSSProperty(value);
  }

  



  void GetCustomPropertyNameAt(uint32_t aIndex, nsAString& aResult) const {
    MOZ_ASSERT(mOrder[aIndex] >= eCSSProperty_COUNT);
    uint32_t variableIndex = mOrder[aIndex] - eCSSProperty_COUNT;
    aResult.Truncate();
    aResult.AppendLiteral("--");
    aResult.Append(mVariableOrder[variableIndex]);
  }

  size_t SizeOfIncludingThis(mozilla::MallocSizeOf aMallocSizeOf) const;

private:
  
  
  
  
  
  
  
  nsAutoTArray<uint32_t, 8> mOrder;

  
  nsTArray<nsString> mVariableOrder;

  
  
  nsAutoPtr<nsCSSCompressedDataBlock> mData;

  
  nsAutoPtr<nsCSSCompressedDataBlock> mImportantData;

  
  nsAutoPtr<CSSVariableDeclarations> mVariables;

  
  nsAutoPtr<CSSVariableDeclarations> mImportantVariables;

  
  
  mutable bool mImmutable;
};

} 
} 

#endif 
