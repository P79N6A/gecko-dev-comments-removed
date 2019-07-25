









#ifndef nsCSSDataBlock_h__
#define nsCSSDataBlock_h__

#include "nsCSSProps.h"
#include "nsCSSPropertySet.h"

struct nsRuleData;
class nsCSSExpandedDataBlock;

namespace mozilla {
namespace css {
class Declaration;
}
}







class nsCSSCompressedDataBlock {
private:
    friend class nsCSSExpandedDataBlock;

    
    
    nsCSSCompressedDataBlock(PRUint32 aNumProps)
      : mStyleBits(0), mNumProps(aNumProps)
    {}

public:
    ~nsCSSCompressedDataBlock();

    



    void MapRuleInfoInto(nsRuleData *aRuleData) const;

    








    const nsCSSValue* ValueFor(nsCSSProperty aProperty) const;

    







    bool TryReplaceValue(nsCSSProperty aProperty,
                           nsCSSExpandedDataBlock& aFromBlock,
                           bool* aChanged);

    


    nsCSSCompressedDataBlock* Clone() const;

    


    static nsCSSCompressedDataBlock* CreateEmptyBlock();

    size_t SizeOfIncludingThis(nsMallocSizeOfFun aMallocSizeOf) const;

private:
    void* operator new(size_t aBaseSize, PRUint32 aNumProps) {
        NS_ABORT_IF_FALSE(aBaseSize == sizeof(nsCSSCompressedDataBlock),
                          "unexpected size for nsCSSCompressedDataBlock");
        return ::operator new(aBaseSize + DataSize(aNumProps));
    }

public:
    
    
    
    
    typedef PRInt16 CompressedCSSProperty;
    static const size_t MaxCompressedCSSProperty = PR_INT16_MAX;

private:
    static size_t DataSize(PRUint32 aNumProps) {
        return size_t(aNumProps) *
               (sizeof(nsCSSValue) + sizeof(CompressedCSSProperty));
    }

    PRInt32 mStyleBits; 
                        
    PRUint32 mNumProps;
    
    
    
    
    
    

    nsCSSValue* Values() const {
        return (nsCSSValue*)(this + 1);
    }

    CompressedCSSProperty* CompressedProperties() const {
        return (CompressedCSSProperty*)(Values() + mNumProps);
    }

    nsCSSValue* ValueAtIndex(PRUint32 i) const {
        NS_ABORT_IF_FALSE(i < mNumProps, "value index out of range");
        return Values() + i;
    }

    nsCSSProperty PropertyAtIndex(PRUint32 i) const {
        NS_ABORT_IF_FALSE(i < mNumProps, "property index out of range");
        nsCSSProperty prop = (nsCSSProperty)CompressedProperties()[i];
        NS_ABORT_IF_FALSE(!nsCSSProps::IsShorthand(prop), "out of range");
        return prop;
    }

    void CopyValueToIndex(PRUint32 i, nsCSSValue* aValue) {
        new (ValueAtIndex(i)) nsCSSValue(*aValue);
    }

    void RawCopyValueToIndex(PRUint32 i, nsCSSValue* aValue) {
        memcpy(ValueAtIndex(i), aValue, sizeof(nsCSSValue));
    }

    void SetPropertyAtIndex(PRUint32 i, nsCSSProperty aProperty) {
        NS_ABORT_IF_FALSE(i < mNumProps, "set property index out of range");
        CompressedProperties()[i] = (CompressedCSSProperty)aProperty;
    }

    void SetNumPropsToZero() {
        mNumProps = 0;
    }
};



MOZ_STATIC_ASSERT(sizeof(nsCSSCompressedDataBlock) == 8,
                  "nsCSSCompressedDataBlock's size has changed");
MOZ_STATIC_ASSERT(NS_ALIGNMENT_OF(nsCSSValue) == 4 || NS_ALIGNMENT_OF(nsCSSValue) == 8,
                  "nsCSSValue doesn't align with nsCSSCompressedDataBlock"); 
MOZ_STATIC_ASSERT(NS_ALIGNMENT_OF(nsCSSCompressedDataBlock::CompressedCSSProperty) == 2,
                  "CompressedCSSProperty doesn't align with nsCSSValue"); 


MOZ_STATIC_ASSERT(eCSSProperty_COUNT_no_shorthands <=
                  nsCSSCompressedDataBlock::MaxCompressedCSSProperty,
                  "nsCSSProperty doesn't fit in StoredSizeOfCSSProperty");

class nsCSSExpandedDataBlock {
    friend class nsCSSCompressedDataBlock;

public:
    nsCSSExpandedDataBlock();
    ~nsCSSExpandedDataBlock();

private:
    


    nsCSSValue mValues[eCSSProperty_COUNT_no_shorthands];

public:
    








    void Expand(nsCSSCompressedDataBlock *aNormalBlock,
                nsCSSCompressedDataBlock *aImportantBlock);

    







    void Compress(nsCSSCompressedDataBlock **aNormalBlock,
                  nsCSSCompressedDataBlock **aImportantBlock);

    



    void AddLonghandProperty(nsCSSProperty aProperty, const nsCSSValue& aValue);

    


    void Clear();

    



    void ClearProperty(nsCSSProperty aPropID);

    


    void ClearLonghandProperty(nsCSSProperty aPropID);

    










    bool TransferFromBlock(nsCSSExpandedDataBlock& aFromBlock,
                             nsCSSProperty aPropID,
                             bool aIsImportant,
                             bool aOverrideImportant,
                             bool aMustCallValueAppended,
                             mozilla::css::Declaration* aDeclaration);

    void AssertInitialState() {
#ifdef DEBUG
        DoAssertInitialState();
#endif
    }

private:
    



    void ComputeNumProps(PRUint32* aNumPropsNormal,
                         PRUint32* aNumPropsImportant);
    
    void DoExpand(nsCSSCompressedDataBlock *aBlock, bool aImportant);

    


    bool DoTransferFromBlock(nsCSSExpandedDataBlock& aFromBlock,
                               nsCSSProperty aPropID,
                               bool aIsImportant,
                               bool aOverrideImportant,
                               bool aMustCallValueAppended,
                               mozilla::css::Declaration* aDeclaration);

#ifdef DEBUG
    void DoAssertInitialState();
#endif

    





    nsCSSPropertySet mPropertiesSet;
    


    nsCSSPropertySet mPropertiesImportant;

    



    nsCSSValue* PropertyAt(nsCSSProperty aProperty) {
        NS_ABORT_IF_FALSE(0 <= aProperty &&
                          aProperty < eCSSProperty_COUNT_no_shorthands,
                          "property out of range");
        return &mValues[aProperty];
    }

    void SetPropertyBit(nsCSSProperty aProperty) {
        mPropertiesSet.AddProperty(aProperty);
    }

    void ClearPropertyBit(nsCSSProperty aProperty) {
        mPropertiesSet.RemoveProperty(aProperty);
    }

    bool HasPropertyBit(nsCSSProperty aProperty) {
        return mPropertiesSet.HasProperty(aProperty);
    }

    void SetImportantBit(nsCSSProperty aProperty) {
        mPropertiesImportant.AddProperty(aProperty);
    }

    void ClearImportantBit(nsCSSProperty aProperty) {
        mPropertiesImportant.RemoveProperty(aProperty);
    }

    bool HasImportantBit(nsCSSProperty aProperty) {
        return mPropertiesImportant.HasProperty(aProperty);
    }

    void ClearSets() {
        mPropertiesSet.Empty();
        mPropertiesImportant.Empty();
    }
};

#endif 
